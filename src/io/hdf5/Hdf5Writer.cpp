#include "oor/io/hdf5/Hdf5Writer.hpp"

#include "Hdf5Internal.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/io/hdf5/Hdf5StreamWriter.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <functional>
#include <set>
#include <system_error>
#include <thread>

namespace oor::io
{

using detail::checked;
using detail::checkedProduct;
using detail::fail;
using detail::id;

std::size_t Hdf5WriteArray::itemSize() const noexcept
{
    switch (kind)
    {
    case Hdf5ValueKind::Int8:
    case Hdf5ValueKind::UInt8:
    case Hdf5ValueKind::Bool:
        return 1;
    case Hdf5ValueKind::Int16:
    case Hdf5ValueKind::UInt16:
        return 2;
    case Hdf5ValueKind::Int32:
    case Hdf5ValueKind::UInt32:
    case Hdf5ValueKind::Float32:
        return 4;
    case Hdf5ValueKind::Int64:
    case Hdf5ValueKind::UInt64:
    case Hdf5ValueKind::Float64:
        return 8;
    case Hdf5ValueKind::FixedString:
        return stringBytes;
    case Hdf5ValueKind::VarString:
        break;
    }
    return sizeof(const char*);
}

std::uint64_t Hdf5WriteArray::elementCount() const noexcept
{
    std::uint64_t count = 1;
    for (const std::uint64_t extent : shape)
        count *= extent;
    return count;
}

namespace
{

constexpr std::uint64_t BLOCK_BYTES = std::uint64_t{8} << 20;
constexpr std::uint64_t AUTO_CHUNK_BYTES = std::uint64_t{1} << 20;

// ------------------------------------------------------------------------------------------------ validation

void validateArray(const Hdf5WriteArray& array, const std::string& what)
{
    const std::uint64_t count = checkedProduct(array.shape, what);
    if (array.kind == Hdf5ValueKind::FixedString && array.stringBytes == 0)
        throw base::UsageError("HDF5: " + what + " is a fixed-width string array with no width");
    if (array.kind == Hdf5ValueKind::VarString)
    {
        if (array.strings.size() != count)
            throw base::UsageError("HDF5: " + what + " has " + std::to_string(array.strings.size()) +
                                   " strings for a shape of " + std::to_string(count) + " values");
        return;
    }
    if (array.data == nullptr && count > 0)
        throw base::UsageError("HDF5: " + what + " has no data");
}

void validatePath(const std::string& path, const Hdf5Limits& limits, bool allowRoot)
{
    if (path.size() > limits.maxPathBytes)
        throw base::LimitExceededError("HDF5: a path is longer than " + std::to_string(limits.maxPathBytes) + " bytes");
    if (path == "/" && allowRoot)
        return;
    if (path.empty() || path.front() != '/' || path.back() == '/')
        throw base::UsageError("HDF5: the path '" + path + "' must be absolute (start with '/') and not end with '/'");
    std::uint32_t depth = 0;
    std::size_t start = 1;
    while (start <= path.size())
    {
        const std::size_t slash = path.find('/', start);
        const std::string part = path.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
        if (part.empty() || part == "." || part == "..")
            throw base::UsageError("HDF5: the path '" + path + "' has an empty, '.' or '..' component");
        ++depth;
        if (slash == std::string::npos)
            break;
        start = slash + 1;
    }
    if (depth > limits.maxDepth)
        throw base::LimitExceededError("HDF5: the path '" + path + "' nests deeper than " +
                                       std::to_string(limits.maxDepth));
}

void validateAttributes(const std::vector<Hdf5WriteAttribute>& attributes, const std::string& owner,
                        const Hdf5Limits& limits)
{
    if (attributes.size() > limits.maxAttributesPerObject)
        throw base::LimitExceededError("HDF5: " + owner + " has " + std::to_string(attributes.size()) +
                                       " attributes (the limit is " + std::to_string(limits.maxAttributesPerObject) +
                                       ")");
    std::set<std::string> names;
    for (const Hdf5WriteAttribute& attribute : attributes)
    {
        const std::string what = "attribute " + attribute.name + " of " + owner;
        if (attribute.name.empty())
            throw base::UsageError("HDF5: an attribute of " + owner + " has no name");
        if (!names.insert(attribute.name).second)
            throw base::UsageError("HDF5: " + owner + " has two attributes named " + attribute.name);
        validateArray(attribute.array, what);
        std::uint64_t bytes = checkedProduct(attribute.array.elementCount(), attribute.array.itemSize(), what);
        if (attribute.array.kind == Hdf5ValueKind::VarString)
        {
            bytes = 0;
            for (const std::string& text : attribute.array.strings)
                bytes += text.size() + sizeof(void*);
        }
        if (bytes > limits.maxWriteAttributeBytes)
            throw base::LimitExceededError(
                "HDF5: " + what + " is " + std::to_string(bytes) + " bytes; the format stores at most " +
                std::to_string(limits.maxWriteAttributeBytes) + " bytes in an attribute (use a dataset)");
    }
}

void validateColumns(const std::vector<Hdf5WriteColumn>& columns, std::uint64_t rows, const std::string& owner,
                     const Hdf5Limits& limits, std::uint32_t depth)
{
    if (depth > limits.maxTypeDepth)
        throw base::LimitExceededError("HDF5: compound members of " + owner + " nest deeper than " +
                                       std::to_string(limits.maxTypeDepth));
    if (columns.empty())
        throw base::UsageError("HDF5: a compound member of " + owner + " has no members");
    if (columns.size() > limits.maxCompoundMembers)
        throw base::LimitExceededError("HDF5: " + owner + " has " + std::to_string(columns.size()) +
                                       " columns (the limit is " + std::to_string(limits.maxCompoundMembers) + ")");
    std::set<std::string> names;
    for (const Hdf5WriteColumn& column : columns)
    {
        if (column.name.empty())
            throw base::UsageError("HDF5: a column of " + owner + " has no name");
        if (!names.insert(column.name).second)
            throw base::UsageError("HDF5: " + owner + " has two columns named " + column.name);
        if (!column.members.empty())
        {
            validateColumns(column.members, rows, owner + "." + column.name, limits, depth + 1);
            continue;
        }
        const std::string what = "column " + column.name + " of " + owner;
        validateArray(column.array, what);
        if (column.array.shape.empty() || column.array.shape.front() != rows)
            throw base::UsageError("HDF5: " + what + " has " +
                                   (column.array.shape.empty() ? std::string("no rows")
                                                               : std::to_string(column.array.shape.front()) + " rows") +
                                   ", the other columns have " + std::to_string(rows));
    }
}

std::uint64_t columnRows(const std::vector<Hdf5WriteColumn>& columns)
{
    const Hdf5WriteColumn& first = columns.front();
    if (!first.members.empty())
        return columnRows(first.members);
    return first.array.shape.empty() ? 0 : first.array.shape.front();
}

// ------------------------------------------------------------------------------------------------ types

struct TypePair
{
    Hdf5Handle file;
    Hdf5Handle memory;
};

TypePair elementTypes(const Hdf5WriteArray& array)
{
    TypePair pair;
    const auto both = [&pair](hid_t file, hid_t memory)
    {
        pair.file = checked(H5Tcopy(file), "cannot copy a type");
        pair.memory = checked(H5Tcopy(memory), "cannot copy a type");
    };
    switch (array.kind)
    {
    case Hdf5ValueKind::Int8:
        both(H5T_STD_I8LE, H5T_NATIVE_INT8);
        break;
    case Hdf5ValueKind::Int16:
        both(H5T_STD_I16LE, H5T_NATIVE_INT16);
        break;
    case Hdf5ValueKind::Int32:
        both(H5T_STD_I32LE, H5T_NATIVE_INT32);
        break;
    case Hdf5ValueKind::Int64:
        both(H5T_STD_I64LE, H5T_NATIVE_INT64);
        break;
    case Hdf5ValueKind::UInt8:
        both(H5T_STD_U8LE, H5T_NATIVE_UINT8);
        break;
    case Hdf5ValueKind::UInt16:
        both(H5T_STD_U16LE, H5T_NATIVE_UINT16);
        break;
    case Hdf5ValueKind::UInt32:
        both(H5T_STD_U32LE, H5T_NATIVE_UINT32);
        break;
    case Hdf5ValueKind::UInt64:
        both(H5T_STD_U64LE, H5T_NATIVE_UINT64);
        break;
    case Hdf5ValueKind::Float32:
        both(H5T_IEEE_F32LE, H5T_NATIVE_FLOAT);
        break;
    case Hdf5ValueKind::Float64:
        both(H5T_IEEE_F64LE, H5T_NATIVE_DOUBLE);
        break;
    case Hdf5ValueKind::Bool:
    {
        pair.file = checked(H5Tenum_create(H5T_STD_I8LE), "cannot create an enum type");
        const std::int8_t falseValue = 0;
        const std::int8_t trueValue = 1;
        if (H5Tenum_insert(id(pair.file), "FALSE", &falseValue) < 0 ||
            H5Tenum_insert(id(pair.file), "TRUE", &trueValue) < 0)
            fail("cannot fill an enum type");
        pair.memory = checked(H5Tcopy(id(pair.file)), "cannot copy a type");
        break;
    }
    case Hdf5ValueKind::FixedString:
    {
        pair.file = checked(H5Tcopy(H5T_C_S1), "cannot create a string type");
        if (H5Tset_size(id(pair.file), array.stringBytes) < 0 || H5Tset_strpad(id(pair.file), H5T_STR_NULLPAD) < 0 ||
            H5Tset_cset(id(pair.file), array.utf8 ? H5T_CSET_UTF8 : H5T_CSET_ASCII) < 0)
            fail("cannot size a string type");
        pair.memory = checked(H5Tcopy(id(pair.file)), "cannot copy a type");
        break;
    }
    case Hdf5ValueKind::VarString:
    {
        pair.file = checked(H5Tcopy(H5T_C_S1), "cannot create a string type");
        if (H5Tset_size(id(pair.file), H5T_VARIABLE) < 0 || H5Tset_cset(id(pair.file), H5T_CSET_UTF8) < 0)
            fail("cannot make a variable-length string type");
        pair.memory = checked(H5Tcopy(id(pair.file)), "cannot copy a type");
        break;
    }
    }
    return pair;
}

// The type of a member: the element type, as an array type when the member has dimensions after the rows.
TypePair memberTypes(const Hdf5WriteArray& array)
{
    TypePair pair = elementTypes(array);
    if (array.shape.size() > 1)
    {
        std::vector<hsize_t> dims(array.shape.begin() + 1, array.shape.end());
        const unsigned rank = static_cast<unsigned>(dims.size());
        pair.file = checked(H5Tarray_create2(id(pair.file), rank, dims.data()), "cannot create an array type");
        pair.memory = checked(H5Tarray_create2(id(pair.memory), rank, dims.data()), "cannot create an array type");
    }
    return pair;
}

struct WriteLeaf
{
    const Hdf5WriteArray* array = nullptr;
    std::size_t offset = 0;      // in a row of the memory compound
    std::size_t bytesPerRow = 0; // of this leaf
    std::uint64_t valuesPerRow = 0;
};

struct BuiltCompound
{
    TypePair types;
    std::size_t size = 0;
    std::size_t alignment = 1;     // the largest alignment of a member (used when the layout is aligned)
    std::vector<WriteLeaf> leaves; // offsets relative to the start of this compound
};

// The alignment of one value of a member, as a C compiler lays out a struct: the size of a number (at most 8), 1 for
// text.
std::size_t elementAlignment(const Hdf5WriteArray& array)
{
    switch (array.kind)
    {
    case Hdf5ValueKind::Int16:
    case Hdf5ValueKind::UInt16:
        return 2;
    case Hdf5ValueKind::Int32:
    case Hdf5ValueKind::UInt32:
    case Hdf5ValueKind::Float32:
        return 4;
    case Hdf5ValueKind::Int64:
    case Hdf5ValueKind::UInt64:
    case Hdf5ValueKind::Float64:
    case Hdf5ValueKind::VarString:
        return 8;
    case Hdf5ValueKind::Int8:
    case Hdf5ValueKind::UInt8:
    case Hdf5ValueKind::Bool:
    case Hdf5ValueKind::FixedString:
        break;
    }
    return 1;
}

std::size_t alignedUp(std::size_t value, std::size_t alignment)
{
    return (value + alignment - 1) / alignment * alignment;
}

// `aligned`: each member starts at a multiple of its alignment and the compound's size is a multiple of the largest one
// (the layout of a C struct, which the the solver files have: a member after a 4-byte string starts at the next multiple of
// 8). Otherwise the members are packed.
BuiltCompound buildCompound(const std::vector<Hdf5WriteColumn>& columns, bool aligned)
{
    std::vector<TypePair> memberPairs;
    std::vector<std::size_t> sizes;
    std::vector<std::size_t> alignments;
    BuiltCompound built;
    std::vector<std::vector<WriteLeaf>> nestedLeaves;
    for (const Hdf5WriteColumn& column : columns)
    {
        if (!column.members.empty())
        {
            BuiltCompound nested = buildCompound(column.members, aligned);
            memberPairs.push_back(std::move(nested.types));
            sizes.push_back(nested.size);
            alignments.push_back(nested.alignment);
            nestedLeaves.push_back(std::move(nested.leaves));
        }
        else
        {
            memberPairs.push_back(memberTypes(column.array));
            sizes.push_back(H5Tget_size(id(memberPairs.back().memory)));
            alignments.push_back(elementAlignment(column.array));
            nestedLeaves.emplace_back();
        }
    }
    std::vector<std::size_t> offsets(columns.size());
    std::size_t total = 0;
    for (std::size_t index = 0; index < sizes.size(); ++index)
    {
        if (aligned)
        {
            total = alignedUp(total, alignments[index]);
            built.alignment = std::max(built.alignment, alignments[index]);
        }
        offsets[index] = total;
        total += sizes[index];
    }
    if (aligned)
        total = alignedUp(total, built.alignment);
    built.types.file = checked(H5Tcreate(H5T_COMPOUND, total), "cannot create a compound type");
    built.types.memory = checked(H5Tcreate(H5T_COMPOUND, total), "cannot create a compound type");
    std::size_t offset = 0;
    for (std::size_t index = 0; index < columns.size(); ++index)
    {
        offset = offsets[index];
        if (H5Tinsert(id(built.types.file), columns[index].name.c_str(), offset, id(memberPairs[index].file)) < 0 ||
            H5Tinsert(id(built.types.memory), columns[index].name.c_str(), offset, id(memberPairs[index].memory)) < 0)
            fail("cannot add the column " + columns[index].name + " to a compound type");
        if (columns[index].members.empty())
        {
            WriteLeaf leaf;
            leaf.array = &columns[index].array;
            leaf.offset = offset;
            leaf.bytesPerRow = sizes[index];
            leaf.valuesPerRow = 1;
            for (std::size_t dim = 1; dim < columns[index].array.shape.size(); ++dim)
                leaf.valuesPerRow *= columns[index].array.shape[dim];
            built.leaves.push_back(leaf);
        }
        else
        {
            for (WriteLeaf leaf : nestedLeaves[index])
            {
                leaf.offset += offset;
                built.leaves.push_back(leaf);
            }
        }
        offset += sizes[index];
    }
    built.size = total;
    return built;
}

// ------------------------------------------------------------------------------------------------ attributes

void writeAttribute(hid_t object, const Hdf5WriteAttribute& attribute)
{
    const TypePair types = elementTypes(attribute.array);
    Hdf5Handle space;
    if (attribute.array.shape.empty())
    {
        space = checked(H5Screate(H5S_SCALAR), "cannot create a scalar dataspace");
    }
    else
    {
        const std::vector<hsize_t> dims(attribute.array.shape.begin(), attribute.array.shape.end());
        space =
            checked(H5Screate_simple(static_cast<int>(dims.size()), dims.data(), nullptr), "cannot create a dataspace");
    }
    const Hdf5Handle created =
        checked(H5Acreate2(object, attribute.name.c_str(), id(types.file), id(space), H5P_DEFAULT, H5P_DEFAULT),
                "cannot create attribute " + attribute.name);
    const std::uint64_t count = attribute.array.elementCount();
    if (count == 0)
        return;
    herr_t status = 0;
    if (attribute.array.kind == Hdf5ValueKind::VarString)
    {
        std::vector<const char*> pointers;
        pointers.reserve(attribute.array.strings.size());
        for (const std::string& text : attribute.array.strings)
            pointers.push_back(text.c_str());
        status = H5Awrite(id(created), id(types.memory), pointers.data());
    }
    else
    {
        status = H5Awrite(id(created), id(types.memory), attribute.array.data);
    }
    if (status < 0)
        fail("cannot write attribute " + attribute.name);
}

void writeAttributes(hid_t object, const std::vector<Hdf5WriteAttribute>& attributes)
{
    for (const Hdf5WriteAttribute& attribute : attributes)
        writeAttribute(object, attribute);
}

// ------------------------------------------------------------------------------------------------ datasets

class FileWriter
{
public:
    FileWriter(hid_t file, const base::CancellationToken* token, base::ProgressCounter* progress)
        : m_file(file), m_token(token), m_progress(progress)
    {
        m_createParents = checked(H5Pcreate(H5P_LINK_CREATE), "cannot create link properties");
        if (H5Pset_create_intermediate_group(id(m_createParents), 1) < 0)
            fail("cannot set link properties");
        // The times of creation and modification are not recorded: the same request gives the same bytes.
        m_groupProperties = checked(H5Pcreate(H5P_GROUP_CREATE), "cannot create group properties");
        if (H5Pset_obj_track_times(id(m_groupProperties), 0) < 0)
            fail("cannot set group properties");
    }

    // Creates the groups on the path of an object that do not exist yet, without times (the groups the library would
    // make on its own record them).
    void makeParents(const std::string& path)
    {
        std::string prefix;
        std::size_t start = 1;
        while (true)
        {
            const std::size_t slash = path.find('/', start);
            if (slash == std::string::npos)
                return;
            prefix = path.substr(0, slash);
            if (!prefix.empty() && !exists(prefix))
            {
                const Hdf5Handle created =
                    checked(H5Gcreate2(m_file, prefix.c_str(), H5P_DEFAULT, id(m_groupProperties), H5P_DEFAULT),
                            "cannot create the group " + prefix);
                static_cast<void>(created);
            }
            start = slash + 1;
        }
    }

    void group(const Hdf5WriteGroup& request)
    {
        Hdf5Handle opened;
        if (request.path == "/")
        {
            opened = checked(H5Gopen2(m_file, "/", H5P_DEFAULT), "cannot open the root group");
        }
        else if (exists(request.path))
        {
            opened =
                checked(H5Gopen2(m_file, request.path.c_str(), H5P_DEFAULT), "cannot open the group " + request.path);
        }
        else
        {
            makeParents(request.path);
            opened = checked(
                H5Gcreate2(m_file, request.path.c_str(), id(m_createParents), id(m_groupProperties), H5P_DEFAULT),
                "cannot create the group " + request.path);
        }
        writeAttributes(id(opened), request.attributes);
    }

    void dataset(const Hdf5WriteDataset& request)
    {
        if (m_token != nullptr)
            m_token->throwIfCancelled();
        const bool compound = !request.columns.empty();
        BuiltCompound built;
        TypePair plainTypes;
        std::uint64_t rows = 0;
        std::vector<std::uint64_t> dims;
        std::size_t rowBytes = 0;
        if (compound)
        {
            rows = columnRows(request.columns);
            built = buildCompound(request.columns, request.alignMembers);
            dims = {rows};
            rowBytes = built.size;
        }
        else
        {
            plainTypes = elementTypes(request.array);
            dims = request.array.shape;
            rows = dims.empty() ? 1 : dims.front();
            std::uint64_t values = 1;
            for (std::size_t dim = 1; dim < dims.size(); ++dim)
                values *= dims[dim];
            rowBytes = static_cast<std::size_t>(values) * request.array.itemSize();
        }
        const hid_t fileType = compound ? id(built.types.file) : id(plainTypes.file);
        const hid_t memoryType = compound ? id(built.types.memory) : id(plainTypes.memory);

        Hdf5Handle space;
        if (dims.empty())
        {
            space = checked(H5Screate(H5S_SCALAR), "cannot create a scalar dataspace");
        }
        else
        {
            const std::vector<hsize_t> extent(dims.begin(), dims.end());
            std::vector<hsize_t> maxExtent = extent;
            if (request.resizable)
                maxExtent[0] = H5S_UNLIMITED;
            space = checked(H5Screate_simple(static_cast<int>(extent.size()), extent.data(), maxExtent.data()),
                            "cannot create a dataspace");
        }
        makeParents(request.path);
        const Hdf5Handle properties = checked(H5Pcreate(H5P_DATASET_CREATE), "cannot create dataset properties");
        if (H5Pset_obj_track_times(id(properties), 0) < 0)
            fail("cannot set the dataset properties of " + request.path);
        std::uint64_t chunkRows = 0;
        const bool wantsChunks = request.chunkRows > 0 || request.deflate > 0 || request.shuffle || request.resizable;
        const bool nonEmpty =
            !dims.empty() && std::all_of(dims.begin(), dims.end(), [](std::uint64_t extent) { return extent > 0; });
        if (wantsChunks && nonEmpty)
        {
            chunkRows = request.chunkRows > 0
                            ? request.chunkRows
                            : std::max<std::uint64_t>(1, AUTO_CHUNK_BYTES / std::max<std::size_t>(rowBytes, 1));
            // A resizable dataset may have a chunk larger than its present extent (as the the solver files do).
            if (!request.resizable)
                chunkRows = std::min(chunkRows, rows);
            std::vector<hsize_t> chunk(dims.begin(), dims.end());
            chunk[0] = chunkRows;
            if (H5Pset_chunk(id(properties), static_cast<int>(chunk.size()), chunk.data()) < 0)
                fail("cannot set the chunk shape of " + request.path);
            if (request.shuffle && H5Pset_shuffle(id(properties)) < 0)
                fail("cannot set the shuffle filter of " + request.path);
            if (request.deflate > 0 && H5Pset_deflate(id(properties), static_cast<unsigned>(request.deflate)) < 0)
                fail("cannot set the deflate filter of " + request.path);
        }
        const Hdf5Handle created = checked(H5Dcreate2(m_file, request.path.c_str(), fileType, id(space),
                                                      id(m_createParents), id(properties), H5P_DEFAULT),
                                           "cannot create the dataset " + request.path);

        const bool anyData = nonEmpty || (dims.empty());
        if (anyData && (compound || request.array.elementCount() > 0))
        {
            if (compound)
                writeCompound(id(created), memoryType, built, rows, chunkRows, request.path);
            else
                writePlain(id(created), memoryType, request.array, dims, chunkRows, rowBytes, request.path);
        }
        writeAttributes(id(created), request.attributes);
    }

    void link(const Hdf5WriteLink& request)
    {
        makeParents(request.path);
        herr_t status = 0;
        if (!request.file.empty())
            status = H5Lcreate_external(request.file.c_str(), request.target.c_str(), m_file, request.path.c_str(),
                                        id(m_createParents), H5P_DEFAULT);
        else if (request.hard)
            status = H5Lcreate_hard(m_file, request.target.c_str(), m_file, request.path.c_str(), id(m_createParents),
                                    H5P_DEFAULT);
        else
            status =
                H5Lcreate_soft(request.target.c_str(), m_file, request.path.c_str(), id(m_createParents), H5P_DEFAULT);
        if (status < 0)
            fail("cannot create the link " + request.path);
    }

private:
    [[nodiscard]] bool exists(const std::string& path) const
    {
        std::string prefix;
        std::size_t start = 1;
        while (start <= path.size())
        {
            const std::size_t slash = path.find('/', start);
            prefix += "/" + path.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
            if (H5Lexists(m_file, prefix.c_str(), H5P_DEFAULT) <= 0)
                return false;
            if (slash == std::string::npos)
                return true;
            start = slash + 1;
        }
        return false;
    }

    [[nodiscard]] std::uint64_t blockRows(std::uint64_t chunkRows, std::size_t rowBytes) const
    {
        std::uint64_t rows = std::max<std::uint64_t>(1, BLOCK_BYTES / std::max<std::size_t>(rowBytes, 1));
        if (chunkRows > 0)
        {
            rows = std::max(rows, chunkRows);
            rows = rows / chunkRows * chunkRows;
        }
        return rows;
    }

    // Writes rows [first, first + count) from `buffer` (memory shaped [count, rest...]).
    void writeRows(hid_t dataset, hid_t memoryType, const std::vector<std::uint64_t>& dims, std::uint64_t first,
                   std::uint64_t count, const void* buffer, const std::string& path) const
    {
        const Hdf5Handle fileSpace = checked(H5Dget_space(dataset), "cannot get the dataspace of " + path);
        std::vector<hsize_t> start(dims.size(), 0);
        std::vector<hsize_t> extent(dims.begin(), dims.end());
        start[0] = first;
        extent[0] = count;
        if (H5Sselect_hyperslab(id(fileSpace), H5S_SELECT_SET, start.data(), nullptr, extent.data(), nullptr) < 0)
            fail("cannot select rows of " + path);
        const Hdf5Handle memorySpace = checked(
            H5Screate_simple(static_cast<int>(extent.size()), extent.data(), nullptr), "cannot create a memory space");
        if (H5Dwrite(dataset, memoryType, id(memorySpace), id(fileSpace), H5P_DEFAULT, buffer) < 0)
            fail("cannot write rows [" + std::to_string(first) + ", " + std::to_string(first + count) + ") of " + path);
    }

    void writeCompound(hid_t dataset, hid_t memoryType, const BuiltCompound& built, std::uint64_t rows,
                       std::uint64_t chunkRows, const std::string& path) const
    {
        const std::uint64_t block = blockRows(chunkRows, built.size);
        std::vector<std::byte> buffer(static_cast<std::size_t>(std::min(block, rows)) * built.size);
        const std::vector<std::uint64_t> dims{rows};
        for (std::uint64_t first = 0; first < rows; first += block)
        {
            if (m_token != nullptr)
                m_token->throwIfCancelled();
            const std::uint64_t count = std::min(block, rows - first);
            for (const WriteLeaf& leaf : built.leaves)
            {
                const bool variable = leaf.array->kind == Hdf5ValueKind::VarString;
                const std::size_t valueBytes = leaf.array->itemSize();
                for (std::uint64_t row = 0; row < count; ++row)
                {
                    std::byte* target = buffer.data() + static_cast<std::size_t>(row) * built.size + leaf.offset;
                    const std::uint64_t absolute = first + row;
                    if (variable)
                    {
                        for (std::uint64_t value = 0; value < leaf.valuesPerRow; ++value)
                        {
                            const char* pointer =
                                leaf.array->strings[static_cast<std::size_t>(absolute * leaf.valuesPerRow + value)]
                                    .c_str();
                            std::memcpy(target + value * sizeof(pointer), &pointer, sizeof(pointer));
                        }
                    }
                    else
                    {
                        const auto* source =
                            static_cast<const std::byte*>(leaf.array->data) + absolute * leaf.valuesPerRow * valueBytes;
                        std::memcpy(target, source, static_cast<std::size_t>(leaf.valuesPerRow) * valueBytes);
                    }
                }
            }
            writeRows(dataset, memoryType, dims, first, count, buffer.data(), path);
            if (m_progress != nullptr)
                m_progress->add(count);
        }
    }

    void writePlain(hid_t dataset, hid_t memoryType, const Hdf5WriteArray& array,
                    const std::vector<std::uint64_t>& dims, std::uint64_t chunkRows, std::size_t rowBytes,
                    const std::string& path) const
    {
        const bool variable = array.kind == Hdf5ValueKind::VarString;
        const auto pointersOf = [&array](std::uint64_t firstValue, std::uint64_t count)
        {
            std::vector<const char*> pointers;
            pointers.reserve(static_cast<std::size_t>(count));
            for (std::uint64_t index = 0; index < count; ++index)
                pointers.push_back(array.strings[static_cast<std::size_t>(firstValue + index)].c_str());
            return pointers;
        };
        if (dims.empty())
        {
            herr_t status = 0;
            if (variable)
            {
                const std::vector<const char*> pointers = pointersOf(0, 1);
                status = H5Dwrite(dataset, memoryType, H5S_ALL, H5S_ALL, H5P_DEFAULT, pointers.data());
            }
            else
            {
                status = H5Dwrite(dataset, memoryType, H5S_ALL, H5S_ALL, H5P_DEFAULT, array.data);
            }
            if (status < 0)
                fail("cannot write the dataset " + path);
            return;
        }
        const std::uint64_t rows = dims.front();
        const std::uint64_t valuesPerRow = rows == 0 ? 0 : array.elementCount() / rows;
        const std::uint64_t block = blockRows(chunkRows, rowBytes);
        for (std::uint64_t first = 0; first < rows; first += block)
        {
            if (m_token != nullptr)
                m_token->throwIfCancelled();
            const std::uint64_t count = std::min(block, rows - first);
            if (variable)
            {
                const std::vector<const char*> pointers = pointersOf(first * valuesPerRow, count * valuesPerRow);
                writeRows(dataset, memoryType, dims, first, count, pointers.data(), path);
            }
            else
            {
                const auto* source =
                    static_cast<const std::byte*>(array.data) + first * valuesPerRow * array.itemSize();
                writeRows(dataset, memoryType, dims, first, count, source, path);
            }
            if (m_progress != nullptr)
                m_progress->add(count);
        }
    }

    hid_t m_file;
    const base::CancellationToken* m_token;
    base::ProgressCounter* m_progress;
    Hdf5Handle m_createParents;
    Hdf5Handle m_groupProperties;
};

std::filesystem::path temporaryPathFor(const std::filesystem::path& target)
{
    static std::atomic<std::uint64_t> counter{0};
    const std::uint64_t stamp = static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    const std::uint64_t unique =
        stamp ^ (std::hash<std::thread::id>{}(std::this_thread::get_id()) << 1) ^ (counter.fetch_add(1) << 40);
    std::filesystem::path temporary = target;
    temporary += ".oor_h5_" + std::to_string(unique) + ".tmp";
    return temporary;
}

} // namespace

void writeHdf5(const std::filesystem::path& target, const Hdf5WriteRequest& request, const Hdf5Limits& limits,
               const base::CancellationToken* token, base::ProgressCounter* progress)
{
    detail::cleanErrors(
        [&]
        {
            // Everything that can be checked without the disk is checked first.
            const std::uint64_t objects = request.groups.size() + request.datasets.size() + request.links.size();
            if (objects > limits.maxWriteObjects)
                throw base::LimitExceededError("HDF5: " + std::to_string(objects) + " objects to write (the limit is " +
                                               std::to_string(limits.maxWriteObjects) + ")");
            std::set<std::string> paths;
            for (const Hdf5WriteGroup& group : request.groups)
            {
                validatePath(group.path, limits, true);
                validateAttributes(group.attributes, "group " + group.path, limits);
            }
            std::uint64_t totalRows = 0;
            for (const Hdf5WriteDataset& dataset : request.datasets)
            {
                validatePath(dataset.path, limits, false);
                if (!paths.insert(dataset.path).second)
                    throw base::UsageError("HDF5: two datasets are written at " + dataset.path);
                if (dataset.deflate < 0 || dataset.deflate > 9)
                    throw base::UsageError("HDF5: the deflate level of " + dataset.path + " is " +
                                           std::to_string(dataset.deflate) + ", it must be 0 to 9");
                validateAttributes(dataset.attributes, "dataset " + dataset.path, limits);
                if (dataset.columns.empty())
                {
                    validateArray(dataset.array, "dataset " + dataset.path);
                    totalRows += dataset.array.shape.empty() ? 1 : dataset.array.shape.front();
                }
                else
                {
                    const std::uint64_t rows = columnRows(dataset.columns);
                    validateColumns(dataset.columns, rows, "dataset " + dataset.path, limits, 0);
                    totalRows += rows;
                }
            }
            for (const Hdf5WriteLink& link : request.links)
            {
                validatePath(link.path, limits, false);
                if (!paths.insert(link.path).second)
                    throw base::UsageError("HDF5: two objects are written at " + link.path);
                if (link.target.empty())
                    throw base::UsageError("HDF5: the link " + link.path + " has no target");
            }

            const std::lock_guard<std::mutex> lock(detail::hdf5Mutex());
            detail::ensureInitialised();
            if (progress != nullptr)
                progress->beginStage("write_h5", totalRows);
            if (!target.parent_path().empty())
            {
                std::error_code ignored;
                std::filesystem::create_directories(target.parent_path(), ignored);
            }
            const std::filesystem::path temporary = temporaryPathFor(target);
            const auto discard = [&temporary]
            {
                std::error_code ignored;
                std::filesystem::remove(temporary, ignored);
            };
            try
            {
                {
                    const Hdf5Handle accessList = checked(H5Pcreate(H5P_FILE_ACCESS), "cannot create file properties");
                    if (H5Pset_fclose_degree(id(accessList), H5F_CLOSE_STRONG) < 0)
                        fail("cannot set the file close degree");
                    const std::string shown = base::pathToUtf8(temporary);
                    const Hdf5Handle file =
                        checked(H5Fcreate(shown.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, id(accessList)),
                                "cannot create HDF5 file: " + shown);
                    FileWriter writer(id(file), token, progress);
                    for (const Hdf5WriteGroup& group : request.groups)
                        writer.group(group);
                    for (const Hdf5WriteDataset& dataset : request.datasets)
                        writer.dataset(dataset);
                    for (const Hdf5WriteLink& link : request.links)
                        writer.link(link);
                    if (H5Fflush(id(file), H5F_SCOPE_GLOBAL) < 0)
                        fail("cannot flush the file to disk");
                }
                if (token != nullptr)
                    token->throwIfCancelled();
                std::error_code error;
                std::filesystem::rename(temporary, target, error);
                if (error)
                    throw base::IoError("cannot replace file: " + base::pathToUtf8(target) + " (" + error.message() +
                                        ")");
            }
            catch (...)
            {
                discard();
                throw;
            }
        });
}

void appendHdf5(const std::filesystem::path& path, const Hdf5WriteRequest& request)
{
    detail::cleanErrors(
        [&]
        {
            const Hdf5Limits limits;
            for (const Hdf5WriteDataset& dataset : request.datasets)
            {
                validatePath(dataset.path, limits, false);
                if (!dataset.columns.empty())
                    validateColumns(dataset.columns, columnRows(dataset.columns), "dataset " + dataset.path, limits, 0);
            }
            const std::lock_guard<std::mutex> lock(detail::hdf5Mutex());
            detail::ensureInitialised();
            const Hdf5Handle accessList = checked(H5Pcreate(H5P_FILE_ACCESS), "cannot create file properties");
            if (H5Pset_fclose_degree(id(accessList), H5F_CLOSE_STRONG) < 0)
                fail("cannot set the file close degree");
            const std::string shown = base::pathToUtf8(path);
            const Hdf5Handle file =
                checked(H5Fopen(shown.c_str(), H5F_ACC_RDWR, id(accessList)), "cannot open HDF5 file: " + shown);
            FileWriter writer(id(file), nullptr, nullptr);
            for (const Hdf5WriteGroup& group : request.groups)
                writer.group(group);
            for (const Hdf5WriteDataset& dataset : request.datasets)
                writer.dataset(dataset);
            for (const Hdf5WriteLink& link : request.links)
                writer.link(link);
            if (H5Fflush(id(file), H5F_SCOPE_GLOBAL) < 0)
                fail("cannot flush the file to disk");
        });
}

} // namespace oor::io
