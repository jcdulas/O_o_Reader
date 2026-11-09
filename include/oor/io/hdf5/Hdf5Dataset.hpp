#pragma once

#include "oor/io/hdf5/Hdf5Handle.hpp"
#include "oor/io/hdf5/Hdf5Type.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace oor::io
{

// A dataset of an HDF5 file: an N-dimensional array. Reading is by rows (the first dimension), so that a large table
// is never read whole unless asked. Errors throw IoError.
class Hdf5Dataset
{
public:
    explicit Hdf5Dataset(Hdf5Handle handle) noexcept : m_handle(std::move(handle)) {}

    [[nodiscard]] std::vector<std::uint64_t> shape() const;
    [[nodiscard]] std::uint64_t rowCount() const;
    [[nodiscard]] Hdf5TypeClass typeClass() const;
    // Size in bytes of one element (one row of a compound table is one element).
    [[nodiscard]] std::size_t elementSize() const;
    // Number of values in one row: the product of the dimensions after the first.
    [[nodiscard]] std::uint64_t valuesPerRow() const;

    // Rows [firstRow, firstRow + rowCount) converted to double / int64; the count is clamped to the end.
    [[nodiscard]] std::vector<double> readDoubles(std::uint64_t firstRow, std::uint64_t rowCount) const;
    [[nodiscard]] std::vector<std::int64_t> readInt64(std::uint64_t firstRow, std::uint64_t rowCount) const;
    // Raw bytes in the file's native memory layout (what a compound table needs).
    [[nodiscard]] std::vector<std::byte> readBytes(std::uint64_t firstRow, std::uint64_t rowCount) const;

    [[nodiscard]] bool hasAttribute(const std::string& name) const;
    [[nodiscard]] std::string readStringAttribute(const std::string& name) const;

private:
    Hdf5Handle m_handle;
};

} // namespace oor::io
