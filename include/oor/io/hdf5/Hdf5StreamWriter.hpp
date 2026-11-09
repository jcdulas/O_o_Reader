#pragma once

#include "oor/io/hdf5/Hdf5Writer.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace oor::io
{

// A member of a table written by rows: an integer, a real or a fixed string, alone or as an array of `count` values.
struct Hdf5StreamMember
{
    std::string name;
    Hdf5ValueKind kind = Hdf5ValueKind::Int64; // Int64, Float64 or FixedString
    std::size_t stringBytes = 0;               // FixedString: the bytes of one string (NUL padded)
    std::uint64_t count = 1;                   // the values of the member in a row (an array member when above 1)
};

// Where each member is in a row of the buffers given to `append`: the members laid out as a C struct (each at a
// multiple of its alignment, the row a multiple of the largest), the layout of the the solver files.
struct Hdf5StreamLayout
{
    std::size_t rowBytes = 0;
    std::vector<std::size_t> offsets; // one per member, in bytes from the start of a row
};

// An HDF5 file written by pieces: tables of rows appended as they come (one-dimensional
// compound datasets, chunked, resizable, deflate 1 with shuffle, as the the solver files have them), small datasets and
// attributes written whole, then the file closed and renamed over the target. Until `finish` the file is a temporary
// one next to the target; `abort` (or the destruction of an unfinished writer) removes it. The library's lock is held
// during each call only; the memory held is the chunk cache of the tables (a small one each: bounded by the number of
// tables, not by the rows appended).
class Hdf5StreamWriter
{
public:
    explicit Hdf5StreamWriter(std::filesystem::path target);
    ~Hdf5StreamWriter();
    Hdf5StreamWriter(const Hdf5StreamWriter&) = delete;
    Hdf5StreamWriter& operator=(const Hdf5StreamWriter&) = delete;

    // Creates the table at `path` (absolute, its groups made as needed). Throws UsageError when it exists.
    const Hdf5StreamLayout& createTable(const std::string& path, const std::vector<Hdf5StreamMember>& members,
                                        std::uint64_t chunkRows);
    [[nodiscard]] bool hasTable(const std::string& path) const noexcept;
    [[nodiscard]] const Hdf5StreamLayout& layoutOf(const std::string& path) const;
    // Appends `count` rows (`rows` holds them in the layout of the table); returns the position of the first.
    std::uint64_t append(const std::string& path, std::span<const std::byte> rows, std::uint64_t count);
    [[nodiscard]] std::uint64_t rowCount(const std::string& path) const;

    // A compound dataset written whole, contiguous (the DOMAINS and INDEX tables of the the solver files).
    void writeTable(const std::string& path, const std::vector<Hdf5StreamMember>& members,
                    std::span<const std::byte> rows, std::uint64_t count);
    // An attribute of a group (made when missing) or of a dataset: an int64 array of one value, or a fixed string.
    void setAttribute(const std::string& objectPath, const std::string& name, std::int64_t value);
    void setAttribute(const std::string& objectPath, const std::string& name, const std::string& value);

    // Closes the file (the temporary one: other writers may add to it, `appendHdf5`), then puts it over the target.
    void close();
    [[nodiscard]] const std::filesystem::path& temporaryPath() const noexcept;
    void commit();
    // Closes and removes the temporary file.
    void abort() noexcept;
    [[nodiscard]] bool isOpen() const noexcept;

private:
    struct State;
    std::unique_ptr<State> m_state;
};

// Writes the groups, datasets and links of `request` into the existing file `path` (as `writeHdf5` writes a new one).
void appendHdf5(const std::filesystem::path& path, const Hdf5WriteRequest& request);

} // namespace oor::io
