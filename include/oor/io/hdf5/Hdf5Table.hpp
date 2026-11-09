#pragma once

#include "oor/base/threading/CancellationToken.hpp"
#include "oor/base/threading/ProgressCounter.hpp"
#include "oor/io/hdf5/Hdf5Limits.hpp"
#include "oor/io/hdf5/Hdf5Type.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace oor::io
{

// A block of bytes that the caller can take over: the memory of a column, handed to NumPy without a copy. Allocated
// uninitialised (a table of gigabytes is not zeroed before it is filled).
class Hdf5Buffer
{
public:
    Hdf5Buffer() = default;
    explicit Hdf5Buffer(std::size_t size) : m_data(size == 0 ? nullptr : new std::byte[size]), m_size(size) {}

    [[nodiscard]] std::byte* data() noexcept { return m_data.get(); }
    [[nodiscard]] const std::byte* data() const noexcept { return m_data.get(); }
    [[nodiscard]] std::size_t size() const noexcept { return m_size; }

private:
    std::unique_ptr<std::byte[]> m_data;
    std::size_t m_size = 0;
};

enum class Hdf5ColumnKind
{
    Signed,
    Unsigned,
    Float,
    FixedString, // bytes, `itemSize` per value, padded as the file pads
    VarString,   // variable-length strings, in `strings`
    Raw          // opaque bytes: `itemSize` per value (opaque types and references)
};

// One column of a table: the values of one member of a compound dataset, or of a plain dataset. The values of a row
// are `shape` (empty: one value); the buffer holds rows * product(shape) values of `itemSize` bytes in the byte order
// of this machine, row after row (C order).
struct Hdf5Column
{
    std::string name; // "DOMAIN_ID", "a.b" for a member of a nested compound, "values" for a plain dataset
    Hdf5ColumnKind kind = Hdf5ColumnKind::Signed;
    std::size_t itemSize = 0;
    std::vector<std::uint64_t> shape; // the dimensions of one row's value; for a dataset of rank > 1 they start with
                                      // the dimensions after the first
    Hdf5Buffer data;                  // not used by a VarString column
    std::vector<std::string> strings; // VarString columns: rows * product(shape) strings
    Hdf5Type fileType;                // the type in the file (an enum keeps its names here)

    // The NumPy type string of the buffer: "<i8", "<u2", "<f4", "|S8", "|V16"; empty for a VarString column.
    [[nodiscard]] std::string numpyString() const;
};

struct Hdf5Table
{
    std::uint64_t rowCount = 0;
    std::vector<Hdf5Column> columns;
    // Rows of the dataset the table holds, as runs [first, last) in increasing order (a table read whole or by row
    // range is one run; a read by DOMAIN_ID holds the runs where the identifier matched).
    std::vector<std::pair<std::uint64_t, std::uint64_t>> sourceRuns;

    [[nodiscard]] const Hdf5Column* column(const std::string& name) const noexcept;
};

struct Hdf5ReadOptions
{
    // Rows [first, last) of the dataset; last is clamped to the end. Nothing: all rows.
    std::optional<std::pair<std::uint64_t, std::uint64_t>> rows;
    // Columns to read, in the order of the result: member names (a compound member that is itself a compound stands for
    // all its leaves, "a" for "a.x" and "a.y"). Nothing: every column.
    std::vector<std::string> columns;
    // Only the rows whose DOMAIN_ID member equals this value (within `rows`). The member is scanned alone, then only
    // the runs of matching rows are read.
    std::optional<std::int64_t> domainId;
    std::string domainColumn = "DOMAIN_ID";
    Hdf5Limits limits;
    const base::CancellationToken* token = nullptr;
    base::ProgressCounter* progress = nullptr;
};

// The structure of a table without reading its data: the columns it would give (names, types, shapes of one row) and
// the rows of the dataset.
struct Hdf5TableLayout
{
    std::uint64_t rowCount = 0;
    std::vector<Hdf5Column> columns; // empty data
};

// Reads a dataset as columns. `dataset` is the path of the dataset in the file. Throws IoError (not a dataset, a type
// that is not read, a damaged file, an external link on the path), LimitExceededError (the result would exceed a cap),
// UsageError (an unknown column, a DOMAIN_ID column that is not an integer), OperationCancelled.
[[nodiscard]] Hdf5Table readHdf5Table(const std::filesystem::path& file, const std::string& dataset,
                                      const Hdf5ReadOptions& options = {});

[[nodiscard]] Hdf5TableLayout describeHdf5Table(const std::filesystem::path& file, const std::string& dataset,
                                                const Hdf5Limits& limits = {});

} // namespace oor::io
