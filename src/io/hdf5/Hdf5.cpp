#include "Hdf5Internal.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/io/hdf5/Hdf5Dataset.hpp"
#include "oor/io/hdf5/Hdf5File.hpp"
#include "oor/io/hdf5/Hdf5Group.hpp"
#include "oor/io/hdf5/Hdf5Handle.hpp"

#include <algorithm>
#include <numeric>
#include <string>

namespace oor::io
{

static_assert(sizeof(hid_t) == sizeof(std::int64_t), "Hdf5Handle stores hid_t as a 64-bit integer");

namespace
{

using detail::checked;
using detail::fail;
using detail::id;

std::vector<hsize_t> toDims(const std::vector<std::uint64_t>& shape)
{
    return std::vector<hsize_t>(shape.begin(), shape.end());
}

std::uint64_t product(const std::vector<std::uint64_t>& shape)
{
    return std::accumulate(shape.begin(), shape.end(), std::uint64_t{1}, std::multiplies<>());
}

std::string readStringAttributeFrom(hid_t object, const std::string& name)
{
    if (H5Aexists(object, name.c_str()) <= 0)
        fail("no attribute named " + name);
    Hdf5Handle attribute = checked(H5Aopen(object, name.c_str(), H5P_DEFAULT), "cannot open attribute " + name);
    Hdf5Handle type = checked(H5Aget_type(id(attribute)), "cannot get the type of attribute " + name);
    // a fixed-length string is read here; a variable-length one would be a pointer into the global heap (where a
    // damaged file can make libhdf5 loop: Hdf5GlobalHeapCheck.hpp), and the the solver files have none
    if (H5Tget_class(id(type)) != H5T_STRING || H5Tis_variable_str(id(type)) != 0)
        fail("the attribute " + name + " is not a fixed-length string");
    const size_t size = H5Tget_size(id(type));
    std::string value(size, '\0');
    if (H5Aread(id(attribute), id(type), value.data()) < 0)
        fail("cannot read attribute " + name);
    value.erase(std::find(value.begin(), value.end(), '\0'), value.end());
    return value;
}

} // namespace

void Hdf5Handle::reset() noexcept
{
    if (m_identifier >= 0)
        H5Idec_ref(static_cast<hid_t>(m_identifier));
    m_identifier = INVALID;
}

std::int64_t openHdf5ObjectCount() noexcept
{
    const ssize_t count = H5Fget_obj_count(static_cast<hid_t>(H5F_OBJ_ALL), H5F_OBJ_ALL);
    return count < 0 ? 0 : static_cast<std::int64_t>(count);
}

// ---------------------------------------------------------------- Hdf5Dataset

std::vector<std::uint64_t> Hdf5Dataset::shape() const
{
    Hdf5Handle space = checked(H5Dget_space(id(m_handle)), "cannot get the dataspace");
    const int rank = H5Sget_simple_extent_ndims(id(space));
    if (rank < 0)
        fail("cannot get the rank of a dataset");
    std::vector<hsize_t> dims(static_cast<std::size_t>(rank));
    if (rank > 0 && H5Sget_simple_extent_dims(id(space), dims.data(), nullptr) < 0)
        fail("cannot get the dimensions of a dataset");
    return std::vector<std::uint64_t>(dims.begin(), dims.end());
}

std::uint64_t Hdf5Dataset::rowCount() const
{
    const auto dims = shape();
    return dims.empty() ? 1 : dims.front();
}

std::uint64_t Hdf5Dataset::valuesPerRow() const
{
    const auto dims = shape();
    if (dims.size() <= 1)
        return 1;
    return product(std::vector<std::uint64_t>(dims.begin() + 1, dims.end()));
}

Hdf5TypeClass Hdf5Dataset::typeClass() const
{
    Hdf5Handle type = checked(H5Dget_type(id(m_handle)), "cannot get the type of a dataset");
    switch (H5Tget_class(id(type)))
    {
    case H5T_INTEGER:
        return Hdf5TypeClass::Integer;
    case H5T_FLOAT:
        return Hdf5TypeClass::Float;
    case H5T_STRING:
        return Hdf5TypeClass::String;
    case H5T_COMPOUND:
        return Hdf5TypeClass::Compound;
    default:
        return Hdf5TypeClass::Other;
    }
}

std::size_t Hdf5Dataset::elementSize() const
{
    Hdf5Handle type = checked(H5Dget_type(id(m_handle)), "cannot get the type of a dataset");
    return H5Tget_size(id(type));
}

namespace
{

// The bytes a read of `rows` rows would allocate, checked against the default cap before anything is allocated (a
// hostile file can claim any shape).
std::size_t guardedBytes(std::uint64_t rows, std::uint64_t valuesPerRow, std::size_t elementBytes)
{
    const Hdf5Limits limits;
    const std::uint64_t bytes =
        detail::checkedProduct(detail::checkedProduct(rows, valuesPerRow, "HDF5 read"), elementBytes, "HDF5 read");
    if (bytes > limits.maxReadBytes)
        throw base::LimitExceededError("HDF5: a read of " + std::to_string(bytes) + " bytes is over the limit of " +
                                       std::to_string(limits.maxReadBytes));
    return static_cast<std::size_t>(bytes);
}

// Reads rows [first, first + count) of a dataset into `buffer` as memory type `memoryType`.
void readRows(hid_t dataset, std::uint64_t firstRow, std::uint64_t& count, hid_t memoryType, std::uint64_t valuesPerRow,
              void* buffer)
{
    (void)valuesPerRow;
    Hdf5Handle space = checked(H5Dget_space(dataset), "cannot get the dataspace");
    const int rank = H5Sget_simple_extent_ndims(id(space));
    std::vector<hsize_t> dims(static_cast<std::size_t>(std::max(rank, 0)));
    if (rank > 0)
        H5Sget_simple_extent_dims(id(space), dims.data(), nullptr);
    const std::uint64_t rows = dims.empty() ? 1 : dims.front();
    if (firstRow >= rows)
    {
        count = 0;
        return;
    }
    count = std::min<std::uint64_t>(count, rows - firstRow);
    if (rank == 0)
    {
        if (H5Dread(dataset, memoryType, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer) < 0)
            fail("cannot read a scalar dataset");
        return;
    }
    std::vector<hsize_t> start(dims.size(), 0);
    std::vector<hsize_t> extent = dims;
    start[0] = firstRow;
    extent[0] = count;
    if (H5Sselect_hyperslab(id(space), H5S_SELECT_SET, start.data(), nullptr, extent.data(), nullptr) < 0)
        fail("cannot select rows of a dataset");
    Hdf5Handle memorySpace = checked(H5Screate_simple(rank, extent.data(), nullptr), "cannot create a memory space");
    if (H5Dread(dataset, memoryType, id(memorySpace), id(space), H5P_DEFAULT, buffer) < 0)
        fail("cannot read rows of a dataset");
}

} // namespace

std::vector<double> Hdf5Dataset::readDoubles(std::uint64_t firstRow, std::uint64_t rowCount) const
{
    const std::uint64_t perRow = valuesPerRow();
    std::uint64_t count = rowCount;
    std::vector<double> values(
        guardedBytes(std::min(rowCount, std::max<std::uint64_t>(this->rowCount(), 1)), perRow, sizeof(double)) /
        sizeof(double));
    readRows(id(m_handle), firstRow, count, H5T_NATIVE_DOUBLE, perRow, values.data());
    values.resize(static_cast<std::size_t>(count * perRow));
    return values;
}

std::vector<std::int64_t> Hdf5Dataset::readInt64(std::uint64_t firstRow, std::uint64_t rowCount) const
{
    const std::uint64_t perRow = valuesPerRow();
    std::uint64_t count = rowCount;
    std::vector<std::int64_t> values(
        guardedBytes(std::min(rowCount, std::max<std::uint64_t>(this->rowCount(), 1)), perRow, sizeof(std::int64_t)) /
        sizeof(std::int64_t));
    readRows(id(m_handle), firstRow, count, H5T_NATIVE_INT64, perRow, values.data());
    values.resize(static_cast<std::size_t>(count * perRow));
    return values;
}

std::vector<std::byte> Hdf5Dataset::readBytes(std::uint64_t firstRow, std::uint64_t rowCount) const
{
    Hdf5Handle fileType = checked(H5Dget_type(id(m_handle)), "cannot get the type of a dataset");
    Hdf5Handle memoryType = checked(H5Tget_native_type(id(fileType), H5T_DIR_ASCEND), "cannot get the native type");
    const std::size_t elementBytes = H5Tget_size(id(memoryType));
    const std::uint64_t perRow = valuesPerRow();
    std::uint64_t count = rowCount;
    std::vector<std::byte> bytes(
        guardedBytes(std::min(rowCount, std::max<std::uint64_t>(this->rowCount(), 1)), perRow, elementBytes));
    readRows(id(m_handle), firstRow, count, id(memoryType), perRow, bytes.data());
    bytes.resize(static_cast<std::size_t>(count * perRow * elementBytes));
    return bytes;
}

bool Hdf5Dataset::hasAttribute(const std::string& name) const
{
    return H5Aexists(id(m_handle), name.c_str()) > 0;
}

std::string Hdf5Dataset::readStringAttribute(const std::string& name) const
{
    return readStringAttributeFrom(id(m_handle), name);
}

// ------------------------------------------------------------------ Hdf5Group

namespace
{

struct NameCollector
{
    std::vector<std::string> names;
};

herr_t collectName(hid_t, const char* name, const H5L_info_t*, void* collector)
{
    static_cast<NameCollector*>(collector)->names.emplace_back(name);
    return 0;
}

} // namespace

std::vector<std::string> Hdf5Group::childNames() const
{
    NameCollector collector;
    hsize_t index = 0;
    if (H5Literate2(id(m_handle), H5_INDEX_NAME, H5_ITER_INC, &index, collectName, &collector) < 0)
        fail("cannot list the children of a group");
    return std::move(collector.names);
}

bool Hdf5Group::hasChild(const std::string& path) const
{
    detail::refuseExternalLinks(id(m_handle), path);
    // H5Lexists fails when an intermediate group is missing: check the path one component at a time.
    std::string prefix;
    std::size_t start = 0;
    while (start <= path.size())
    {
        const std::size_t slash = path.find('/', start);
        const std::string part = path.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
        if (part.empty())
            return false;
        prefix += (prefix.empty() ? "" : "/") + part;
        if (H5Lexists(id(m_handle), prefix.c_str(), H5P_DEFAULT) <= 0)
            return false;
        if (slash == std::string::npos)
            return true;
        start = slash + 1;
    }
    return false;
}

bool Hdf5Group::isGroup(const std::string& path) const
{
    if (!hasChild(path))
        return false;
    H5O_info2_t info;
    if (H5Oget_info_by_name3(id(m_handle), path.c_str(), &info, H5O_INFO_BASIC, H5P_DEFAULT) < 0)
        return false;
    return info.type == H5O_TYPE_GROUP;
}

Hdf5Group Hdf5Group::openGroup(const std::string& path) const
{
    detail::refuseExternalLinks(id(m_handle), path);
    return Hdf5Group(checked(H5Gopen2(id(m_handle), path.c_str(), H5P_DEFAULT), "no group named " + path));
}

Hdf5Dataset Hdf5Group::openDataset(const std::string& path) const
{
    detail::refuseExternalLinks(id(m_handle), path);
    return Hdf5Dataset(checked(H5Dopen2(id(m_handle), path.c_str(), H5P_DEFAULT), "no dataset named " + path));
}

Hdf5Group Hdf5Group::createGroup(const std::string& name)
{
    return Hdf5Group(checked(H5Gcreate2(id(m_handle), name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT),
                             "cannot create group " + name));
}

void Hdf5Group::writeDoubles(const std::string& name, const std::vector<std::uint64_t>& shape,
                             const std::vector<double>& values)
{
    if (product(shape) != values.size())
        throw base::UsageError("the shape of dataset " + name + " does not match the number of values");
    const std::vector<hsize_t> dims = toDims(shape);
    Hdf5Handle space =
        checked(H5Screate_simple(static_cast<int>(dims.size()), dims.data(), nullptr), "cannot create a dataspace");
    Hdf5Handle dataset = checked(
        H5Dcreate2(id(m_handle), name.c_str(), H5T_IEEE_F64LE, id(space), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT),
        "cannot create dataset " + name);
    if (H5Dwrite(id(dataset), H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, values.data()) < 0)
        fail("cannot write dataset " + name);
}

void Hdf5Group::writeInt64(const std::string& name, const std::vector<std::uint64_t>& shape,
                           const std::vector<std::int64_t>& values)
{
    if (product(shape) != values.size())
        throw base::UsageError("the shape of dataset " + name + " does not match the number of values");
    const std::vector<hsize_t> dims = toDims(shape);
    Hdf5Handle space =
        checked(H5Screate_simple(static_cast<int>(dims.size()), dims.data(), nullptr), "cannot create a dataspace");
    Hdf5Handle dataset =
        checked(H5Dcreate2(id(m_handle), name.c_str(), H5T_STD_I64LE, id(space), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT),
                "cannot create dataset " + name);
    if (H5Dwrite(id(dataset), H5T_NATIVE_INT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, values.data()) < 0)
        fail("cannot write dataset " + name);
}

void Hdf5Group::setStringAttribute(const std::string& name, const std::string& value)
{
    Hdf5Handle type = checked(H5Tcopy(H5T_C_S1), "cannot create a string type");
    if (H5Tset_size(id(type), value.size() + 1) < 0)
        fail("cannot size a string type");
    Hdf5Handle space = checked(H5Screate(H5S_SCALAR), "cannot create a scalar dataspace");
    Hdf5Handle attribute =
        checked(H5Acreate2(id(m_handle), name.c_str(), id(type), id(space), H5P_DEFAULT, H5P_DEFAULT),
                "cannot create attribute " + name);
    if (H5Awrite(id(attribute), id(type), value.c_str()) < 0)
        fail("cannot write attribute " + name);
}

std::string Hdf5Group::readStringAttribute(const std::string& name) const
{
    return readStringAttributeFrom(id(m_handle), name);
}

// ------------------------------------------------------------------ Hdf5File

Hdf5File Hdf5File::openReadOnly(const std::filesystem::path& path)
{
    const std::string shown = base::pathToUtf8(path);
    Hdf5Handle file = detail::openFileReadOnly(shown);
    Hdf5Handle root = checked(H5Gopen2(id(file), "/", H5P_DEFAULT), "cannot open the root group of " + shown);
    return Hdf5File(std::move(file), std::move(root));
}

Hdf5File Hdf5File::create(const std::filesystem::path& path)
{
    detail::ensureInitialised();
    const std::string shown = base::pathToUtf8(path);
    Hdf5Handle file =
        checked(H5Fcreate(shown.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT), "cannot create HDF5 file: " + shown);
    Hdf5Handle root = checked(H5Gopen2(id(file), "/", H5P_DEFAULT), "cannot open the root group of " + shown);
    return Hdf5File(std::move(file), std::move(root));
}

} // namespace oor::io
