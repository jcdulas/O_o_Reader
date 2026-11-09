#pragma once

#include "oor/io/hdf5/Hdf5Group.hpp"

#include <filesystem>

namespace oor::io
{

// An HDF5 file; also its root group. The only entry to libhdf5 for the rest of the project (the HDF5 headers are
// included only by src/io/hdf5). The HDF5 error stack is silenced: failures are reported as IoError with a message.
class Hdf5File final : public Hdf5Group
{
public:
    // Throws IoError when the file does not exist or is not an HDF5 file.
    [[nodiscard]] static Hdf5File openReadOnly(const std::filesystem::path& path);
    // Creates the file, replacing an existing one.
    [[nodiscard]] static Hdf5File create(const std::filesystem::path& path);

private:
    explicit Hdf5File(Hdf5Handle file, Hdf5Handle root) : Hdf5Group(std::move(root)), m_file(std::move(file)) {}

    Hdf5Handle m_file;
};

} // namespace oor::io
