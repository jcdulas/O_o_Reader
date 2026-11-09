#pragma once

#include "oor/io/hdf5/Hdf5Dataset.hpp"
#include "oor/io/hdf5/Hdf5Handle.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace oor::io
{

// A group of an HDF5 file (a folder of groups and datasets). Paths are "a/b/c" relative to the group. Errors throw
// IoError.
class Hdf5Group
{
public:
    explicit Hdf5Group(Hdf5Handle handle) noexcept : m_handle(std::move(handle)) {}
    virtual ~Hdf5Group() = default;
    Hdf5Group(Hdf5Group&&) noexcept = default;
    Hdf5Group& operator=(Hdf5Group&&) noexcept = default;

    [[nodiscard]] std::vector<std::string> childNames() const;
    [[nodiscard]] bool hasChild(const std::string& path) const;
    [[nodiscard]] bool isGroup(const std::string& path) const;

    [[nodiscard]] Hdf5Group openGroup(const std::string& path) const;
    [[nodiscard]] Hdf5Dataset openDataset(const std::string& path) const;

    // Writing (used by the writers of later steps and by the tests).
    Hdf5Group createGroup(const std::string& name);
    void writeDoubles(const std::string& name, const std::vector<std::uint64_t>& shape,
                      const std::vector<double>& values);
    void writeInt64(const std::string& name, const std::vector<std::uint64_t>& shape,
                    const std::vector<std::int64_t>& values);
    void setStringAttribute(const std::string& name, const std::string& value);
    [[nodiscard]] std::string readStringAttribute(const std::string& name) const;

protected:
    Hdf5Handle m_handle;
};

} // namespace oor::io
