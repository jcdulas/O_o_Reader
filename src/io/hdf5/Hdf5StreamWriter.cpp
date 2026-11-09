#include "oor/io/hdf5/Hdf5StreamWriter.hpp"

#include "Hdf5Internal.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <system_error>

namespace oor::io
{

using detail::checked;
using detail::fail;
using detail::id;

namespace
{

// The chunk cache of each table: one chunk or so (the rows are appended in order and never read back).
constexpr std::size_t CHUNK_CACHE_BYTES = std::size_t{1} << 20;

std::size_t alignmentOf(const Hdf5StreamMember& member) noexcept
{
    return member.kind == Hdf5ValueKind::FixedString ? 1 : 8;
}

std::size_t valueBytes(const Hdf5StreamMember& member)
{
    switch (member.kind)
    {
    case Hdf5ValueKind::Int64:
    case Hdf5ValueKind::Float64:
        return 8;
    case Hdf5ValueKind::FixedString:
        if (member.stringBytes == 0)
            throw base::UsageError("HDF5: the string member " + member.name + " has no width");
        return member.stringBytes;
    default:
        throw base::UsageError(
            "HDF5: the member " + member.name +
            " is not an int64, a float64 or a fixed string (the members of a table written by rows)");
    }
}

// The file type of one value of a member.
Hdf5Handle valueType(const Hdf5StreamMember& member)
{
    switch (member.kind)
    {
    case Hdf5ValueKind::Int64:
        return checked(H5Tcopy(H5T_STD_I64LE), "cannot copy a type");
    case Hdf5ValueKind::Float64:
        return checked(H5Tcopy(H5T_IEEE_F64LE), "cannot copy a type");
    default:
    {
        Hdf5Handle type = checked(H5Tcopy(H5T_C_S1), "cannot create a string type");
        if (H5Tset_size(id(type), member.stringBytes) < 0 || H5Tset_strpad(id(type), H5T_STR_NULLPAD) < 0)
            fail("cannot size a string type");
        return type;
    }
    }
}

// The compound type of a row (the same in memory and in the file: little endian on the machines this builds for) and
// the offsets of its members, laid out as a C struct.
Hdf5Handle rowType(const std::vector<Hdf5StreamMember>& members, Hdf5StreamLayout& layout)
{
    layout.offsets.clear();
    std::size_t total = 0;
    std::size_t largest = 1;
    for (const Hdf5StreamMember& member : members)
    {
        const std::size_t alignment = alignmentOf(member);
        largest = std::max(largest, alignment);
        total = (total + alignment - 1) / alignment * alignment;
        layout.offsets.push_back(total);
        total += valueBytes(member) * static_cast<std::size_t>(member.count);
    }
    total = (total + largest - 1) / largest * largest;
    layout.rowBytes = total;
    Hdf5Handle compound = checked(H5Tcreate(H5T_COMPOUND, total), "cannot create a compound type");
    for (std::size_t index = 0; index < members.size(); ++index)
    {
        const Hdf5StreamMember& member = members[index];
        Hdf5Handle type = valueType(member);
        if (member.count > 1)
        {
            const hsize_t dims[1] = {member.count};
            type = checked(H5Tarray_create2(id(type), 1, dims), "cannot create an array type");
        }
        if (H5Tinsert(id(compound), member.name.c_str(), layout.offsets[index], id(type)) < 0)
            fail("cannot add the member " + member.name + " to a compound type");
    }
    return compound;
}

std::uint64_t uniqueNumber()
{
    static std::atomic<std::uint64_t> counter{0};
    const auto stamp = static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    return stamp ^ (counter.fetch_add(1) << 40);
}

} // namespace

struct Hdf5StreamWriter::State
{
    struct Table
    {
        Hdf5Handle dataset;
        Hdf5Handle type;
        Hdf5StreamLayout layout;
        std::uint64_t rows = 0;
    };

    std::filesystem::path target;
    std::filesystem::path temporary;
    Hdf5Handle file;
    Hdf5Handle createParents;
    Hdf5Handle groupProperties;
    std::map<std::string, Table> tables;
    bool committed = false;

    void makeGroups(const std::string& path)
    {
        std::size_t start = 1;
        while (true)
        {
            const std::size_t slash = path.find('/', start);
            if (slash == std::string::npos)
                return;
            const std::string prefix = path.substr(0, slash);
            if (H5Lexists(id(file), prefix.c_str(), H5P_DEFAULT) <= 0)
            {
                const Hdf5Handle created =
                    checked(H5Gcreate2(id(file), prefix.c_str(), H5P_DEFAULT, id(groupProperties), H5P_DEFAULT),
                            "cannot create the group " + prefix);
                static_cast<void>(created);
            }
            start = slash + 1;
        }
    }

    // The object at `path`, a group made when nothing is there.
    Hdf5Handle openObject(const std::string& path)
    {
        if (path == "/")
            return checked(H5Oopen(id(file), "/", H5P_DEFAULT), "cannot open the root group");
        if (H5Lexists(id(file), path.c_str(), H5P_DEFAULT) <= 0)
        {
            makeGroups(path + "/");
        }
        return checked(H5Oopen(id(file), path.c_str(), H5P_DEFAULT), "cannot open " + path);
    }
};

Hdf5StreamWriter::Hdf5StreamWriter(std::filesystem::path target) : m_state(std::make_unique<State>())
{
    m_state->target = std::move(target);
    if (!m_state->target.parent_path().empty())
    {
        std::error_code ignored;
        std::filesystem::create_directories(m_state->target.parent_path(), ignored);
    }
    m_state->temporary = m_state->target;
    m_state->temporary += ".oor_h5_" + std::to_string(uniqueNumber()) + ".tmp";
    detail::cleanErrors(
        [&]
        {
            const std::lock_guard<std::mutex> lock(detail::hdf5Mutex());
            detail::ensureInitialised();
            const Hdf5Handle accessList = checked(H5Pcreate(H5P_FILE_ACCESS), "cannot create file properties");
            if (H5Pset_fclose_degree(id(accessList), H5F_CLOSE_STRONG) < 0)
                fail("cannot set the file close degree");
            const std::string shown = base::pathToUtf8(m_state->temporary);
            m_state->file = checked(H5Fcreate(shown.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, id(accessList)),
                                    "cannot create HDF5 file: " + shown);
            m_state->createParents = checked(H5Pcreate(H5P_LINK_CREATE), "cannot create link properties");
            if (H5Pset_create_intermediate_group(id(m_state->createParents), 1) < 0)
                fail("cannot set link properties");
            m_state->groupProperties = checked(H5Pcreate(H5P_GROUP_CREATE), "cannot create group properties");
            if (H5Pset_obj_track_times(id(m_state->groupProperties), 0) < 0)
                fail("cannot set group properties");
        });
}

Hdf5StreamWriter::~Hdf5StreamWriter()
{
    if (m_state && !m_state->committed)
        abort();
}

bool Hdf5StreamWriter::isOpen() const noexcept
{
    return m_state && m_state->file.isValid();
}

const Hdf5StreamLayout& Hdf5StreamWriter::createTable(const std::string& path,
                                                      const std::vector<Hdf5StreamMember>& members,
                                                      std::uint64_t chunkRows)
{
    return detail::cleanErrors(
        [&]() -> const Hdf5StreamLayout&
        {
            if (!isOpen())
                throw base::UsageError("HDF5: the stream writer is closed");
            if (m_state->tables.count(path) != 0)
                throw base::UsageError("HDF5: the table " + path + " exists already");
            const std::lock_guard<std::mutex> lock(detail::hdf5Mutex());
            State::Table table;
            table.type = rowType(members, table.layout);
            const hsize_t extent[1] = {0};
            const hsize_t maxExtent[1] = {H5S_UNLIMITED};
            const Hdf5Handle space = checked(H5Screate_simple(1, extent, maxExtent), "cannot create a dataspace");
            const Hdf5Handle properties = checked(H5Pcreate(H5P_DATASET_CREATE), "cannot create dataset properties");
            const hsize_t chunk[1] = {std::max<std::uint64_t>(1, chunkRows)};
            if (H5Pset_obj_track_times(id(properties), 0) < 0 || H5Pset_chunk(id(properties), 1, chunk) < 0 ||
                H5Pset_shuffle(id(properties)) < 0 || H5Pset_deflate(id(properties), 1) < 0)
                fail("cannot set the dataset properties of " + path);
            const Hdf5Handle access = checked(H5Pcreate(H5P_DATASET_ACCESS), "cannot create dataset access");
            if (H5Pset_chunk_cache(id(access), 521, CHUNK_CACHE_BYTES, 0.75) < 0)
                fail("cannot set the chunk cache of " + path);
            m_state->makeGroups(path);
            table.dataset = checked(H5Dcreate2(id(m_state->file), path.c_str(), id(table.type), id(space),
                                               id(m_state->createParents), id(properties), id(access)),
                                    "cannot create the dataset " + path);
            const auto [where, inserted] = m_state->tables.emplace(path, std::move(table));
            static_cast<void>(inserted);
            return where->second.layout;
        });
}

bool Hdf5StreamWriter::hasTable(const std::string& path) const noexcept
{
    return m_state && m_state->tables.count(path) != 0;
}

const Hdf5StreamLayout& Hdf5StreamWriter::layoutOf(const std::string& path) const
{
    const auto found = m_state->tables.find(path);
    if (found == m_state->tables.end())
        throw base::UsageError("HDF5: no table " + path + " was created");
    return found->second.layout;
}

std::uint64_t Hdf5StreamWriter::rowCount(const std::string& path) const
{
    const auto found = m_state->tables.find(path);
    return found == m_state->tables.end() ? 0 : found->second.rows;
}

std::uint64_t Hdf5StreamWriter::append(const std::string& path, std::span<const std::byte> rows, std::uint64_t count)
{
    return detail::cleanErrors(
        [&]
        {
            const auto found = m_state->tables.find(path);
            if (found == m_state->tables.end())
                throw base::UsageError("HDF5: no table " + path + " was created");
            State::Table& table = found->second;
            if (rows.size() != count * table.layout.rowBytes)
                throw base::UsageError("HDF5: " + std::to_string(rows.size()) + " bytes for " + std::to_string(count) +
                                       " rows of " + std::to_string(table.layout.rowBytes) + " bytes of " + path);
            const std::uint64_t first = table.rows;
            if (count == 0)
                return first;
            const std::lock_guard<std::mutex> lock(detail::hdf5Mutex());
            const hsize_t extent[1] = {first + count};
            if (H5Dset_extent(id(table.dataset), extent) < 0)
                fail("cannot extend the dataset " + path);
            const Hdf5Handle fileSpace =
                checked(H5Dget_space(id(table.dataset)), "cannot get the dataspace of " + path);
            const hsize_t start[1] = {first};
            const hsize_t size[1] = {count};
            if (H5Sselect_hyperslab(id(fileSpace), H5S_SELECT_SET, start, nullptr, size, nullptr) < 0)
                fail("cannot select rows of " + path);
            const Hdf5Handle memorySpace = checked(H5Screate_simple(1, size, nullptr), "cannot create a memory space");
            if (H5Dwrite(id(table.dataset), id(table.type), id(memorySpace), id(fileSpace), H5P_DEFAULT, rows.data()) <
                0)
                fail("cannot write rows of " + path);
            table.rows += count;
            return first;
        });
}

void Hdf5StreamWriter::writeTable(const std::string& path, const std::vector<Hdf5StreamMember>& members,
                                  std::span<const std::byte> rows, std::uint64_t count)
{
    detail::cleanErrors(
        [&]
        {
            if (!isOpen())
                throw base::UsageError("HDF5: the stream writer is closed");
            const std::lock_guard<std::mutex> lock(detail::hdf5Mutex());
            Hdf5StreamLayout layout;
            const Hdf5Handle type = rowType(members, layout);
            if (rows.size() != count * layout.rowBytes)
                throw base::UsageError("HDF5: " + std::to_string(rows.size()) + " bytes for " + std::to_string(count) +
                                       " rows of " + path);
            const hsize_t extent[1] = {count};
            const Hdf5Handle space = checked(H5Screate_simple(1, extent, nullptr), "cannot create a dataspace");
            const Hdf5Handle properties = checked(H5Pcreate(H5P_DATASET_CREATE), "cannot create dataset properties");
            if (H5Pset_obj_track_times(id(properties), 0) < 0)
                fail("cannot set the dataset properties of " + path);
            m_state->makeGroups(path);
            const Hdf5Handle dataset = checked(H5Dcreate2(id(m_state->file), path.c_str(), id(type), id(space),
                                                          id(m_state->createParents), id(properties), H5P_DEFAULT),
                                               "cannot create the dataset " + path);
            if (count > 0 && H5Dwrite(id(dataset), id(type), H5S_ALL, H5S_ALL, H5P_DEFAULT, rows.data()) < 0)
                fail("cannot write the dataset " + path);
        });
}

void Hdf5StreamWriter::setAttribute(const std::string& objectPath, const std::string& name, std::int64_t value)
{
    detail::cleanErrors(
        [&]
        {
            const std::lock_guard<std::mutex> lock(detail::hdf5Mutex());
            const Hdf5Handle object = m_state->openObject(objectPath);
            const hsize_t extent[1] = {1};
            const Hdf5Handle space = checked(H5Screate_simple(1, extent, nullptr), "cannot create a dataspace");
            const Hdf5Handle attribute =
                checked(H5Acreate2(id(object), name.c_str(), H5T_STD_I64LE, id(space), H5P_DEFAULT, H5P_DEFAULT),
                        "cannot create the attribute " + name);
            if (H5Awrite(id(attribute), H5T_NATIVE_INT64, &value) < 0)
                fail("cannot write the attribute " + name);
        });
}

void Hdf5StreamWriter::setAttribute(const std::string& objectPath, const std::string& name, const std::string& value)
{
    detail::cleanErrors(
        [&]
        {
            const std::lock_guard<std::mutex> lock(detail::hdf5Mutex());
            const Hdf5Handle object = m_state->openObject(objectPath);
            const Hdf5Handle type = checked(H5Tcopy(H5T_C_S1), "cannot create a string type");
            if (H5Tset_size(id(type), std::max<std::size_t>(1, value.size())) < 0 ||
                H5Tset_strpad(id(type), H5T_STR_NULLPAD) < 0)
                fail("cannot size a string type");
            const Hdf5Handle space = checked(H5Screate(H5S_SCALAR), "cannot create a scalar dataspace");
            const Hdf5Handle attribute =
                checked(H5Acreate2(id(object), name.c_str(), id(type), id(space), H5P_DEFAULT, H5P_DEFAULT),
                        "cannot create the attribute " + name);
            std::string padded = value;
            padded.resize(std::max<std::size_t>(1, value.size()), '\0');
            if (H5Awrite(id(attribute), id(type), padded.data()) < 0)
                fail("cannot write the attribute " + name);
        });
}

void Hdf5StreamWriter::close()
{
    if (!isOpen())
        throw base::UsageError("HDF5: the stream writer is closed");
    try
    {
        detail::cleanErrors(
            [&]
            {
                const std::lock_guard<std::mutex> lock(detail::hdf5Mutex());
                m_state->tables.clear();
                if (H5Fflush(id(m_state->file), H5F_SCOPE_GLOBAL) < 0)
                    fail("cannot flush the file to disk");
                m_state->groupProperties.reset();
                m_state->createParents.reset();
                m_state->file.reset();
            });
    }
    catch (...)
    {
        abort();
        throw;
    }
}

const std::filesystem::path& Hdf5StreamWriter::temporaryPath() const noexcept
{
    return m_state->temporary;
}

void Hdf5StreamWriter::commit()
{
    if (isOpen())
        close();
    std::error_code error;
    std::filesystem::rename(m_state->temporary, m_state->target, error);
    if (error)
    {
        abort();
        throw base::IoError("cannot replace file: " + base::pathToUtf8(m_state->target) + " (" + error.message() + ")");
    }
    m_state->committed = true;
}

void Hdf5StreamWriter::abort() noexcept
{
    if (!m_state)
        return;
    try
    {
        const std::lock_guard<std::mutex> lock(detail::hdf5Mutex());
        m_state->tables.clear();
        m_state->groupProperties.reset();
        m_state->createParents.reset();
        m_state->file.reset();
    }
    catch (...)
    {
    }
    std::error_code ignored;
    std::filesystem::remove(m_state->temporary, ignored);
}

} // namespace oor::io
