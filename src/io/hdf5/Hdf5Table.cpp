#include "oor/io/hdf5/Hdf5Table.hpp"

#include "Hdf5Internal.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/io/hdf5/Hdf5GlobalHeapCheck.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <new>
#include <optional>

namespace oor::io
{

using detail::checked;
using detail::checkedProduct;
using detail::fail;
using detail::id;

std::string Hdf5Column::numpyString() const
{
    const char native = std::endian::native == std::endian::little ? '<' : '>';
    const std::string size = std::to_string(itemSize);
    switch (kind)
    {
    case Hdf5ColumnKind::Signed:
        return std::string(1, itemSize == 1 ? '|' : native) + "i" + size;
    case Hdf5ColumnKind::Unsigned:
        return std::string(1, itemSize == 1 ? '|' : native) + "u" + size;
    case Hdf5ColumnKind::Float:
        return std::string(1, native) + "f" + size;
    case Hdf5ColumnKind::FixedString:
        return "|S" + size;
    case Hdf5ColumnKind::Raw:
        return "|V" + size;
    case Hdf5ColumnKind::VarString:
        break;
    }
    return {};
}

const Hdf5Column* Hdf5Table::column(const std::string& name) const noexcept
{
    for (const Hdf5Column& candidate : columns)
        if (candidate.name == name)
            return &candidate;
    return nullptr;
}

namespace
{

constexpr const char* PLAIN_COLUMN = "values";
constexpr std::uint64_t BLOCK_BYTES = std::uint64_t{16} << 20;
constexpr std::size_t CHUNK_CACHE_BYTES = std::size_t{32} << 20;
constexpr std::uint64_t STRING_ESTIMATE = 32; // bytes counted for one variable-length string against the cap

// One value type of the table: a leaf of the compound tree (or the whole element of a plain dataset).
struct Leaf
{
    std::string name;
    std::vector<std::string> path; // member names from the top compound
    Hdf5Type fileType;             // the type of one value (the base type of an array member)
    Hdf5ColumnKind kind = Hdf5ColumnKind::Signed;
    std::size_t itemSize = 0;
    std::vector<std::uint64_t> arrayDims;
    std::uint64_t perElement = 1; // values in one element: the product of arrayDims
    std::string unsupported;      // non-empty: the type this table reader does not read
    Hdf5Handle memoryType;        // memory type of the leaf: the base, or the array of it
    std::size_t memoryBytes = 0;  // size of memoryType (what the leaf takes in a row buffer)
};

// Fills the classification and the memory type of a leaf from its file type.
void classifyLeaf(Leaf& leaf, hid_t type, const Hdf5Limits& limits)
{
    Hdf5Handle valueType;
    Hdf5Type description = detail::describeType(type, limits);
    hid_t base = type;
    if (description.typeClass == Hdf5TypeClass::Array && description.base)
    {
        leaf.arrayDims = description.dims;
        leaf.perElement = checkedProduct(description.dims, "an array member");
        valueType = checked(H5Tget_super(type), "cannot read the base of an array type");
        base = id(valueType);
        // copied out first: `base` belongs to `description`, and assigning from it in place freed it half way through
        // the copy (found by AddressSanitizer)
        Hdf5Type element = *description.base;
        description = std::move(element);
    }
    leaf.fileType = description;
    // The library converts integers and floats value by value, in a time that grows with the size of the type: a
    // damaged file can claim an integer of a megabyte. Nothing real is wider than 64 bits (integers) or 128 bits
    // (floats).
    const bool tooWide =
        (description.typeClass == Hdf5TypeClass::Integer || description.typeClass == Hdf5TypeClass::Bitfield ||
         description.typeClass == Hdf5TypeClass::Enum)
            ? description.size > 8
            : (description.typeClass == Hdf5TypeClass::Float && description.size > 16);
    if (tooWide)
    {
        leaf.unsupported = std::string(leaf.arrayDims.empty() ? "" : "array of ") + description.className() + " of " +
                           std::to_string(description.size * 8) + " bits";
        return;
    }
    Hdf5Handle memoryBase;
    switch (description.typeClass)
    {
    case Hdf5TypeClass::Integer:
    case Hdf5TypeClass::Bitfield:
        memoryBase = checked(H5Tget_native_type(base, H5T_DIR_ASCEND), "cannot get the native type of an integer");
        leaf.kind = description.isSigned && description.typeClass == Hdf5TypeClass::Integer ? Hdf5ColumnKind::Signed
                                                                                            : Hdf5ColumnKind::Unsigned;
        break;
    case Hdf5TypeClass::Enum:
    {
        const Hdf5Handle super = checked(H5Tget_super(base), "cannot get the base of an enum");
        memoryBase = checked(H5Tget_native_type(id(super), H5T_DIR_ASCEND), "cannot get the native type of an enum");
        leaf.kind = description.isSigned ? Hdf5ColumnKind::Signed : Hdf5ColumnKind::Unsigned;
        break;
    }
    case Hdf5TypeClass::Float:
        if (description.size == 2)
        {
            // half precision: NumPy's float16 where the library has the type, else widened to float32
#ifdef H5_HAVE__FLOAT16
            memoryBase = checked(H5Tcopy(H5T_NATIVE_FLOAT16), "cannot copy a native type");
#else
            memoryBase = checked(H5Tcopy(H5T_NATIVE_FLOAT), "cannot copy a native type");
#endif
        }
        else
        {
            memoryBase = checked(H5Tget_native_type(base, H5T_DIR_ASCEND), "cannot get the native type of a float");
        }
        if (H5Tget_size(id(memoryBase)) > 8)
            memoryBase = checked(H5Tcopy(H5T_NATIVE_DOUBLE), "cannot copy a native type");
        leaf.kind = Hdf5ColumnKind::Float;
        break;
    case Hdf5TypeClass::String:
        memoryBase = checked(H5Tcopy(base), "cannot copy a string type");
        leaf.kind = description.variableLength ? Hdf5ColumnKind::VarString : Hdf5ColumnKind::FixedString;
        break;
    case Hdf5TypeClass::Opaque:
    case Hdf5TypeClass::Reference:
        memoryBase = checked(H5Tcopy(base), "cannot copy a type");
        leaf.kind = Hdf5ColumnKind::Raw;
        break;
    default:
        leaf.unsupported = std::string(leaf.arrayDims.empty() ? "" : "array of ") + description.className();
        return;
    }
    leaf.memoryBytes = H5Tget_size(id(memoryBase));
    leaf.itemSize = leaf.kind == Hdf5ColumnKind::VarString ? 0 : leaf.memoryBytes;
    if (leaf.arrayDims.empty())
    {
        leaf.memoryType = std::move(memoryBase);
    }
    else
    {
        std::vector<hsize_t> dims(leaf.arrayDims.begin(), leaf.arrayDims.end());
        leaf.memoryType = checked(H5Tarray_create2(id(memoryBase), static_cast<unsigned>(dims.size()), dims.data()),
                                  "cannot create an array type");
        leaf.memoryBytes = H5Tget_size(id(leaf.memoryType));
    }
}

void collectLeaves(hid_t type, const Hdf5Type& description, std::vector<std::string>& path, std::vector<Leaf>& leaves,
                   const Hdf5Limits& limits)
{
    if (description.typeClass == Hdf5TypeClass::Compound)
    {
        for (std::size_t index = 0; index < description.members.size(); ++index)
        {
            const Hdf5Handle member = checked(H5Tget_member_type(type, static_cast<unsigned>(index)),
                                              "cannot read a member of a compound type");
            path.push_back(description.members[index].name);
            collectLeaves(id(member), description.members[index].type, path, leaves, limits);
            path.pop_back();
        }
        return;
    }
    Leaf leaf;
    leaf.path = path;
    for (std::size_t index = 0; index < path.size(); ++index)
        leaf.name += (index == 0 ? "" : ".") + path[index];
    if (path.empty())
        leaf.name = PLAIN_COLUMN;
    classifyLeaf(leaf, type, limits);
    leaves.push_back(std::move(leaf));
}

// The members of the memory compound type, merged by common prefix, for a set of leaves. Offsets are packed in tree
// order; `offsets[i]` is where leaf i lands in a row of the buffer.
struct Trie
{
    std::string name;
    std::vector<Trie> kids;
    int leaf = -1;
};

void insertLeaf(Trie& root, const std::vector<std::string>& path, int leafIndex)
{
    Trie* node = &root;
    for (const std::string& part : path)
    {
        auto found =
            std::find_if(node->kids.begin(), node->kids.end(), [&](const Trie& kid) { return kid.name == part; });
        if (found == node->kids.end())
        {
            node->kids.push_back(Trie{part, {}, -1});
            found = node->kids.end() - 1;
        }
        node = &*found;
    }
    node->leaf = leafIndex;
}

std::size_t trieSize(const Trie& node, const std::vector<Leaf>& leaves)
{
    if (node.leaf >= 0)
        return leaves[static_cast<std::size_t>(node.leaf)].memoryBytes;
    std::size_t size = 0;
    for (const Trie& kid : node.kids)
        size += trieSize(kid, leaves);
    return size;
}

Hdf5Handle buildCompound(const Trie& node, std::size_t base, const std::vector<Leaf>& leaves,
                         std::vector<std::size_t>& offsets)
{
    Hdf5Handle compound = checked(H5Tcreate(H5T_COMPOUND, trieSize(node, leaves)), "cannot create a compound type");
    std::size_t offset = 0;
    for (const Trie& kid : node.kids)
    {
        if (kid.leaf >= 0)
        {
            const Leaf& leaf = leaves[static_cast<std::size_t>(kid.leaf)];
            if (H5Tinsert(id(compound), kid.name.c_str(), offset, id(leaf.memoryType)) < 0)
                fail("cannot insert the member " + kid.name + " in a memory type");
            offsets[static_cast<std::size_t>(kid.leaf)] = base + offset;
            offset += leaf.memoryBytes;
        }
        else
        {
            const Hdf5Handle nested = buildCompound(kid, base + offset, leaves, offsets);
            if (H5Tinsert(id(compound), kid.name.c_str(), offset, id(nested)) < 0)
                fail("cannot insert the member " + kid.name + " in a memory type");
            offset += trieSize(kid, leaves);
        }
    }
    return compound;
}

// ------------------------------------------------------------------------------------------------ the dataset

struct OpenDataset
{
    Hdf5Handle file;
    Hdf5Handle dataset;
    Hdf5Type type;
    bool compound = false;
    bool isNull = false;
    std::vector<std::uint64_t> dims;  // the whole shape ([] for a scalar)
    std::uint64_t rows = 0;           // dims[0], 1 for a scalar, 0 for an empty dataspace
    std::vector<std::uint64_t> rest;  // dims after the first
    std::uint64_t elementsPerRow = 1; // product of rest
    std::uint64_t chunkRows = 0;      // rows of a chunk, 0 if not chunked
    std::vector<Leaf> leaves;
};

OpenDataset openDataset(const std::filesystem::path& path, const std::string& datasetPath, const Hdf5Limits& limits)
{
    OpenDataset result;
    result.file = detail::openFileReadOnly(base::pathToUtf8(path));
    detail::refuseExternalLinks(id(result.file), datasetPath);
    H5O_info2_t info;
    if (H5Oget_info_by_name3(id(result.file), datasetPath.c_str(), &info, H5O_INFO_BASIC, H5P_DEFAULT) < 0)
        fail("no object named " + datasetPath);
    if (info.type != H5O_TYPE_DATASET)
        throw base::IoError("HDF5: " + datasetPath + " is not a dataset");
    const Hdf5Handle accessList = checked(H5Pcreate(H5P_DATASET_ACCESS), "cannot create access properties");
    H5Pset_chunk_cache(id(accessList), H5D_CHUNK_CACHE_NSLOTS_DEFAULT, CHUNK_CACHE_BYTES, H5D_CHUNK_CACHE_W0_DEFAULT);
    result.dataset =
        checked(H5Dopen2(id(result.file), datasetPath.c_str(), id(accessList)), "cannot open " + datasetPath);
    const Hdf5Handle type = checked(H5Dget_type(id(result.dataset)), "cannot read the type of " + datasetPath);
    result.type = detail::describeType(id(type), limits);
    result.compound = result.type.typeClass == Hdf5TypeClass::Compound;
    const Hdf5Handle space = checked(H5Dget_space(id(result.dataset)), "cannot read the shape of " + datasetPath);
    const H5S_class_t spaceClass = H5Sget_simple_extent_type(id(space));
    if (spaceClass == H5S_NULL)
    {
        result.isNull = true;
        result.rows = 0;
    }
    else if (spaceClass == H5S_SCALAR)
    {
        result.rows = 1;
    }
    else
    {
        const int rank = H5Sget_simple_extent_ndims(id(space));
        if (rank < 0)
            fail("cannot read the rank of " + datasetPath);
        std::vector<hsize_t> dims(static_cast<std::size_t>(rank));
        if (rank > 0 && H5Sget_simple_extent_dims(id(space), dims.data(), nullptr) < 0)
            fail("cannot read the shape of " + datasetPath);
        result.dims.assign(dims.begin(), dims.end());
        result.rows = result.dims.empty() ? 1 : result.dims.front();
        if (result.dims.size() > 1)
            result.rest.assign(result.dims.begin() + 1, result.dims.end());
        result.elementsPerRow = checkedProduct(result.rest, "the shape of " + datasetPath);
    }
    // The chunk height, to read in blocks that cover whole chunks.
    const Hdf5Handle plist =
        checked(H5Dget_create_plist(id(result.dataset)), "cannot read the properties of " + datasetPath);
    if (H5Pget_layout(id(plist)) == H5D_CHUNKED && !result.dims.empty())
    {
        std::vector<hsize_t> chunks(result.dims.size() + 1);
        const int rank = H5Pget_chunk(id(plist), static_cast<int>(chunks.size()), chunks.data());
        if (rank > 0)
            result.chunkRows = chunks[0];
    }
    std::vector<std::string> names;
    collectLeaves(id(type), result.type, names, result.leaves, limits);
    return result;
}

[[noreturn]] void unsupportedColumn(const Leaf& leaf, const std::string& datasetPath)
{
    throw base::IoError("HDF5: the column " + leaf.name + " of " + datasetPath +
                        " has a type that read_h5_table does not read (" + leaf.unsupported +
                        "); leave it out of `columns`");
}

std::string availableColumns(const std::vector<Leaf>& leaves)
{
    std::string text;
    for (const Leaf& leaf : leaves)
        text += (text.empty() ? "" : ", ") + leaf.name;
    return text;
}

// The indices of the leaves a request selects, in the order of the request.
std::vector<std::size_t> selectLeaves(const OpenDataset& dataset, const std::vector<std::string>& requested,
                                      const std::string& datasetPath)
{
    std::vector<std::size_t> selected;
    if (requested.empty())
    {
        for (std::size_t index = 0; index < dataset.leaves.size(); ++index)
            selected.push_back(index);
        return selected;
    }
    for (const std::string& name : requested)
    {
        bool found = false;
        for (std::size_t index = 0; index < dataset.leaves.size(); ++index)
        {
            const std::string& leafName = dataset.leaves[index].name;
            const bool matches =
                leafName == name || (leafName.size() > name.size() && leafName.compare(0, name.size(), name) == 0 &&
                                     leafName[name.size()] == '.');
            if (!matches)
                continue;
            found = true;
            if (std::find(selected.begin(), selected.end(), index) == selected.end())
                selected.push_back(index);
        }
        if (!found)
            throw base::UsageError("HDF5: " + datasetPath + " has no column " + name +
                                   " (columns: " + availableColumns(dataset.leaves) + ")");
    }
    return selected;
}

Hdf5Column makeColumn(const Leaf& leaf, const OpenDataset& dataset)
{
    Hdf5Column column;
    column.name = leaf.name;
    column.kind = leaf.kind;
    column.itemSize = leaf.itemSize;
    column.shape = dataset.rest;
    column.shape.insert(column.shape.end(), leaf.arrayDims.begin(), leaf.arrayDims.end());
    column.fileType = leaf.fileType;
    return column;
}

// Everything that depends on the selected leaves: the memory type of a row, where each leaf lands in it.
struct ReadPlan
{
    std::vector<std::size_t> selected;
    Hdf5Handle memoryType;            // the compound (or the plain leaf type) of one element
    std::vector<std::size_t> offsets; // per selected leaf, in a row buffer
    std::size_t elementBytes = 0;     // bytes of one element in a row buffer
    bool direct = false;              // one leaf, no variable-length strings: read into the column
};

ReadPlan makePlan(const OpenDataset& dataset, std::vector<std::size_t> selected)
{
    ReadPlan plan;
    plan.selected = std::move(selected);
    plan.offsets.assign(plan.selected.size(), 0);
    if (!dataset.compound)
    {
        const Leaf& leaf = dataset.leaves[plan.selected.front()];
        plan.memoryType = checked(H5Tcopy(id(leaf.memoryType)), "cannot copy a type");
        plan.elementBytes = leaf.memoryBytes;
    }
    else
    {
        // The trie works on the leaves in the order of the selection, so the leaf indices are positions in `chosen`.
        std::vector<Leaf> chosen;
        chosen.reserve(plan.selected.size());
        Trie root;
        for (std::size_t position = 0; position < plan.selected.size(); ++position)
        {
            const Leaf& source = dataset.leaves[plan.selected[position]];
            Leaf copy;
            copy.path = source.path;
            copy.memoryBytes = source.memoryBytes;
            copy.memoryType = checked(H5Tcopy(id(source.memoryType)), "cannot copy a type");
            chosen.push_back(std::move(copy));
            insertLeaf(root, source.path, static_cast<int>(position));
        }
        plan.memoryType = buildCompound(root, 0, chosen, plan.offsets);
        plan.elementBytes = trieSize(root, chosen);
    }
    plan.direct = plan.selected.size() == 1 && dataset.leaves[plan.selected.front()].kind != Hdf5ColumnKind::VarString;
    return plan;
}

// Reads rows [first, first + count) of the dataset into `buffer` with the memory type; returns the memory space used
// (needed to reclaim variable-length data).
Hdf5Handle readBlock(const OpenDataset& dataset, hid_t memoryType, std::uint64_t first, std::uint64_t count,
                     void* buffer)
{
    if (dataset.dims.empty())
    {
        // a scalar dataspace: the one element
        Hdf5Handle space = checked(H5Screate(H5S_SCALAR), "cannot create a scalar dataspace");
        if (H5Dread(id(dataset.dataset), memoryType, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer) < 0)
            fail("cannot read a scalar dataset");
        return space;
    }
    const Hdf5Handle fileSpace = checked(H5Dget_space(id(dataset.dataset)), "cannot get the dataspace");
    std::vector<hsize_t> start(dataset.dims.size(), 0);
    std::vector<hsize_t> extent(dataset.dims.begin(), dataset.dims.end());
    start[0] = first;
    extent[0] = count;
    if (H5Sselect_hyperslab(id(fileSpace), H5S_SELECT_SET, start.data(), nullptr, extent.data(), nullptr) < 0)
        fail("cannot select rows of a dataset");
    Hdf5Handle memorySpace = checked(H5Screate_simple(static_cast<int>(extent.size()), extent.data(), nullptr),
                                     "cannot create a memory space");
    if (H5Dread(id(dataset.dataset), memoryType, id(memorySpace), id(fileSpace), H5P_DEFAULT, buffer) < 0)
        fail("cannot read rows [" + std::to_string(first) + ", " + std::to_string(first + count) + ") of a dataset");
    return memorySpace;
}

// Rows per block: about BLOCK_BYTES of row buffer, never fewer than a chunk, a multiple of the chunk height.
std::uint64_t blockRowsFor(const OpenDataset& dataset, std::uint64_t bytesPerRow)
{
    std::uint64_t rows = std::max<std::uint64_t>(1, BLOCK_BYTES / std::max<std::uint64_t>(bytesPerRow, 1));
    if (dataset.chunkRows > 0)
    {
        rows = std::max(rows, dataset.chunkRows);
        rows = rows / dataset.chunkRows * dataset.chunkRows;
    }
    return rows;
}

// End of the next block starting at `position`: blocks end on chunk boundaries so that a chunk is decoded once.
std::uint64_t blockEnd(const OpenDataset& dataset, std::uint64_t position, std::uint64_t end, std::uint64_t blockRows)
{
    std::uint64_t next = std::min(end, position + blockRows);
    if (dataset.chunkRows > 0 && next < end)
    {
        const std::uint64_t aligned = next / dataset.chunkRows * dataset.chunkRows;
        next = aligned > position ? aligned : std::min(end, (position / dataset.chunkRows + 1) * dataset.chunkRows);
    }
    return next;
}

using Runs = std::vector<std::pair<std::uint64_t, std::uint64_t>>;

// The runs of rows in [first, last) whose DOMAIN_ID equals `domain`: the member is read alone, block by block.
Runs scanDomain(const OpenDataset& dataset, std::size_t leafIndex, std::int64_t domain, std::uint64_t first,
                std::uint64_t last, const Hdf5ReadOptions& options)
{
    const Leaf& leaf = dataset.leaves[leafIndex];
    // The member read as int64 whatever its width: a memory type that holds only this member, wrapped once for each
    // level of nesting.
    Hdf5Handle memoryType = checked(H5Tcopy(H5T_NATIVE_INT64), "cannot copy a type");
    if (dataset.compound)
    {
        for (std::size_t depth = leaf.path.size(); depth-- > 0;)
        {
            Hdf5Handle wrapper =
                checked(H5Tcreate(H5T_COMPOUND, sizeof(std::int64_t)), "cannot create a compound type");
            if (H5Tinsert(id(wrapper), leaf.path[depth].c_str(), 0, id(memoryType)) < 0)
                fail("cannot insert the member " + leaf.path[depth]);
            memoryType = std::move(wrapper);
        }
    }
    Runs runs;
    const std::uint64_t blockRows =
        blockRowsFor(dataset, sizeof(std::int64_t) * std::max<std::uint64_t>(dataset.elementsPerRow, 1));
    std::vector<std::int64_t> block;
    std::uint64_t runStart = 0;
    bool inRun = false;
    for (std::uint64_t position = first; position < last;)
    {
        if (options.token != nullptr)
            options.token->throwIfCancelled();
        const std::uint64_t end = blockEnd(dataset, position, last, blockRows);
        const std::uint64_t count = end - position;
        block.resize(static_cast<std::size_t>(count));
        (void)readBlock(dataset, id(memoryType), position, count, block.data());
        for (std::uint64_t index = 0; index < count; ++index)
        {
            const bool match = block[static_cast<std::size_t>(index)] == domain;
            if (match && !inRun)
            {
                inRun = true;
                runStart = position + index;
            }
            else if (!match && inRun)
            {
                inRun = false;
                runs.emplace_back(runStart, position + index);
            }
        }
        if (options.progress != nullptr)
            options.progress->add(count);
        position = end;
    }
    if (inRun)
        runs.emplace_back(runStart, last);
    return runs;
}

std::string capMessage(std::uint64_t bytes, const Hdf5Limits& limits)
{
    return "HDF5: the table would take " + std::to_string(bytes) + " bytes, over the limit of " +
           std::to_string(limits.maxReadBytes) + " (select rows or columns)";
}

} // namespace

Hdf5TableLayout describeHdf5Table(const std::filesystem::path& file, const std::string& datasetPath,
                                  const Hdf5Limits& limits)
{
    return detail::cleanErrors(
        [&]
        {
            const std::lock_guard<std::mutex> lock(detail::hdf5Mutex());
            detail::ensureInitialised();
            const OpenDataset dataset = openDataset(file, datasetPath, limits);
            Hdf5TableLayout layout;
            layout.rowCount = dataset.rows;
            for (const Leaf& leaf : dataset.leaves)
            {
                if (!leaf.unsupported.empty())
                    continue;
                layout.columns.push_back(makeColumn(leaf, dataset));
            }
            return layout;
        });
}

Hdf5Table readHdf5Table(const std::filesystem::path& file, const std::string& datasetPath,
                        const Hdf5ReadOptions& options)
{
    return detail::cleanErrors(
        [&]
        {
            const std::lock_guard<std::mutex> lock(detail::hdf5Mutex());
            detail::ensureInitialised();
            const Hdf5Limits& limits = options.limits;
            OpenDataset dataset = openDataset(file, datasetPath, limits);

            const std::vector<std::size_t> selected = selectLeaves(dataset, options.columns, datasetPath);
            for (const std::size_t index : selected)
                if (!dataset.leaves[index].unsupported.empty())
                    unsupportedColumn(dataset.leaves[index], datasetPath);
            // variable-length strings come from the global heap, where a damaged collection makes libhdf5 loop
            // forever in H5Dread (Hdf5GlobalHeapCheck.hpp): the heap is checked first
            const bool variable = std::any_of(selected.begin(), selected.end(), [&](std::size_t index)
                                              { return dataset.leaves[index].kind == Hdf5ColumnKind::VarString; });
            if (variable)
                if (const std::optional<std::string> problem = findGlobalHeapProblem(file))
                    throw base::IoError("HDF5: " + datasetPath + " has variable-length strings and " + *problem +
                                        ": not read (libhdf5 would never return from it); leave those columns out of "
                                        "columns");

            // The rows asked for.
            std::uint64_t first = 0;
            std::uint64_t last = dataset.rows;
            if (options.rows.has_value())
            {
                if (options.rows->second < options.rows->first)
                    throw base::UsageError("HDF5: rows (" + std::to_string(options.rows->first) + ", " +
                                           std::to_string(options.rows->second) + ") end before they start");
                first = std::min(options.rows->first, dataset.rows);
                last = std::min(options.rows->second, dataset.rows);
            }
            if (options.progress != nullptr)
                options.progress->beginStage("read_h5_table", (last - first) * (options.domainId.has_value() ? 2 : 1));

            Runs runs;
            if (options.domainId.has_value())
            {
                std::size_t domainLeaf = dataset.leaves.size();
                for (std::size_t index = 0; index < dataset.leaves.size(); ++index)
                    if (dataset.leaves[index].name == options.domainColumn)
                        domainLeaf = index;
                if (domainLeaf == dataset.leaves.size())
                    throw base::UsageError("HDF5: " + datasetPath + " has no column " + options.domainColumn +
                                           " (columns: " + availableColumns(dataset.leaves) + ")");
                const Leaf& leaf = dataset.leaves[domainLeaf];
                const bool integer = leaf.kind == Hdf5ColumnKind::Signed || leaf.kind == Hdf5ColumnKind::Unsigned;
                if (!integer || leaf.perElement != 1 || dataset.elementsPerRow != 1 || !dataset.compound)
                    throw base::UsageError("HDF5: the column " + options.domainColumn +
                                           " is not a plain integer column of a one-dimensional compound dataset");
                runs = scanDomain(dataset, domainLeaf, *options.domainId, first, last, options);
            }
            else if (last > first)
            {
                runs.emplace_back(first, last);
            }
            std::uint64_t outRows = 0;
            for (const auto& run : runs)
                outRows += run.second - run.first;

            // Caps, before any allocation.
            if (outRows > limits.maxReadRows)
                throw base::LimitExceededError("HDF5: the table has " + std::to_string(outRows) +
                                               " rows, over the limit of " + std::to_string(limits.maxReadRows));
            Hdf5Table table;
            table.rowCount = outRows;
            table.sourceRuns = runs;
            std::uint64_t totalBytes = 0;
            for (const std::size_t index : selected)
            {
                const Leaf& leaf = dataset.leaves[index];
                const std::uint64_t values = checkedProduct(
                    checkedProduct(outRows, dataset.elementsPerRow, "the table"), leaf.perElement, "the table");
                const std::uint64_t bytes = checkedProduct(
                    values, leaf.kind == Hdf5ColumnKind::VarString ? STRING_ESTIMATE : leaf.itemSize, "the table");
                totalBytes += bytes;
                if (totalBytes > limits.maxReadBytes)
                    throw base::LimitExceededError(capMessage(totalBytes, limits));
                table.columns.push_back(makeColumn(leaf, dataset));
            }
            if (outRows == 0 || dataset.elementsPerRow == 0)
                return table;

            try
            {
                for (std::size_t position = 0; position < selected.size(); ++position)
                {
                    const Leaf& leaf = dataset.leaves[selected[position]];
                    Hdf5Column& column = table.columns[position];
                    const std::uint64_t values = outRows * dataset.elementsPerRow * leaf.perElement;
                    if (leaf.kind == Hdf5ColumnKind::VarString)
                        column.strings.resize(static_cast<std::size_t>(values));
                    else
                        column.data = Hdf5Buffer(static_cast<std::size_t>(values * leaf.itemSize));
                }
            }
            catch (const std::bad_alloc&)
            {
                throw base::LimitExceededError("HDF5: not enough memory for the table (select rows or columns)");
            }

            const ReadPlan plan = makePlan(dataset, selected);
            const std::uint64_t bytesPerRow = static_cast<std::uint64_t>(plan.elementBytes) * dataset.elementsPerRow;
            const std::uint64_t blockRows = blockRowsFor(dataset, bytesPerRow);
            Hdf5Buffer rowBuffer;
            if (!plan.direct)
                rowBuffer = Hdf5Buffer(static_cast<std::size_t>(std::min(blockRows, outRows) * bytesPerRow));
            std::uint64_t stringBytes = 0;

            std::uint64_t outRow = 0;
            for (const auto& run : runs)
            {
                for (std::uint64_t position = run.first; position < run.second;)
                {
                    if (options.token != nullptr)
                        options.token->throwIfCancelled();
                    const std::uint64_t end = blockEnd(dataset, position, run.second, blockRows);
                    const std::uint64_t count = end - position;
                    const std::uint64_t elements = count * dataset.elementsPerRow;
                    if (plan.direct)
                    {
                        const Leaf& leaf = dataset.leaves[selected.front()];
                        Hdf5Column& column = table.columns.front();
                        const std::size_t elementBytes = leaf.itemSize * static_cast<std::size_t>(leaf.perElement);
                        (void)readBlock(dataset, id(plan.memoryType), position, count,
                                        column.data.data() +
                                            static_cast<std::size_t>(outRow * dataset.elementsPerRow) * elementBytes);
                    }
                    else
                    {
                        const Hdf5Handle memorySpace =
                            readBlock(dataset, id(plan.memoryType), position, count, rowBuffer.data());
                        for (std::size_t slot = 0; slot < selected.size(); ++slot)
                        {
                            const Leaf& leaf = dataset.leaves[selected[slot]];
                            Hdf5Column& column = table.columns[slot];
                            const std::size_t offset = plan.offsets[slot];
                            if (leaf.kind == Hdf5ColumnKind::VarString)
                            {
                                const std::size_t perElement = static_cast<std::size_t>(leaf.perElement);
                                for (std::uint64_t element = 0; element < elements; ++element)
                                {
                                    const std::byte* source = rowBuffer.data() + element * plan.elementBytes + offset;
                                    for (std::size_t value = 0; value < perElement; ++value)
                                    {
                                        const char* pointer = nullptr;
                                        std::memcpy(&pointer, source + value * sizeof(char*), sizeof(char*));
                                        std::string& target = column.strings[static_cast<std::size_t>(
                                            (outRow * dataset.elementsPerRow + element) * perElement + value)];
                                        if (pointer != nullptr)
                                        {
                                            const std::size_t length =
                                                ::strnlen(pointer, static_cast<std::size_t>(limits.maxStringBytes));
                                            if (length >= limits.maxStringBytes)
                                                throw base::LimitExceededError("HDF5: a string is longer than " +
                                                                               std::to_string(limits.maxStringBytes) +
                                                                               " bytes");
                                            stringBytes += length;
                                            target.assign(pointer, length);
                                        }
                                    }
                                }
                                if (stringBytes > limits.maxReadBytes)
                                    throw base::LimitExceededError(capMessage(stringBytes, limits));
                                continue;
                            }
                            const std::size_t elementBytes = leaf.itemSize * static_cast<std::size_t>(leaf.perElement);
                            std::byte* target =
                                column.data.data() +
                                static_cast<std::size_t>(outRow * dataset.elementsPerRow) * elementBytes;
                            const std::byte* source = rowBuffer.data() + offset;
                            switch (elementBytes)
                            {
                            case 8:
                                for (std::uint64_t element = 0; element < elements;
                                     ++element, source += plan.elementBytes, target += 8)
                                    std::memcpy(target, source, 8);
                                break;
                            case 4:
                                for (std::uint64_t element = 0; element < elements;
                                     ++element, source += plan.elementBytes, target += 4)
                                    std::memcpy(target, source, 4);
                                break;
                            default:
                                for (std::uint64_t element = 0; element < elements;
                                     ++element, source += plan.elementBytes, target += elementBytes)
                                    std::memcpy(target, source, elementBytes);
                                break;
                            }
                        }
                        // free what the library allocated for variable-length strings
                        bool hasVariable = false;
                        for (const std::size_t index : selected)
                            hasVariable = hasVariable || dataset.leaves[index].kind == Hdf5ColumnKind::VarString;
                        if (hasVariable)
                            H5Treclaim(id(plan.memoryType), id(memorySpace), H5P_DEFAULT, rowBuffer.data());
                    }
                    if (options.progress != nullptr)
                        options.progress->add(count);
                    outRow += count;
                    position = end;
                }
            }
            return table;
        });
}

} // namespace oor::io
