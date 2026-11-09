#pragma once

#include "oor/io/binary/EndianReader.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace oor::io
{

// How the records of a Fortran unformatted sequential file are laid out: each record is
//   [marker][data][marker]
// where the marker is the length of the data in bytes, 4 or 8 bytes wide, in the file's byte order.
struct FortranLayout
{
    ByteOrder byteOrder = ByteOrder::Little;
    int markerWidth = 4; // 4 or 8
    [[nodiscard]] bool operator==(const FortranLayout&) const noexcept = default;
};

// Where the data of one record is, markers excluded.
struct RecordSpan
{
    std::uint64_t dataOffset = 0;
    std::uint64_t length = 0;
};

// Walks the records of a Fortran unformatted file held in memory. Both markers of every
// record are checked against each other and against the file size, so a corrupt or truncated file raises IoError at
// the faulty record instead of reading out of bounds.
class FortranRecordReader
{
public:
    FortranRecordReader(std::span<const std::byte> file, FortranLayout layout) noexcept : m_file(file), m_layout(layout)
    {
    }

    // Guesses the layout from the first record: the layout for which the leading marker equals the trailing marker
    // and the record fits in the file. Tries little then big endian, 4-byte then 8-byte markers. Empty when no
    // layout is consistent (not a Fortran unformatted file).
    [[nodiscard]] static std::optional<FortranLayout> detectLayout(std::span<const std::byte> file) noexcept;

    [[nodiscard]] const FortranLayout& layout() const noexcept { return m_layout; }
    [[nodiscard]] std::uint64_t position() const noexcept { return m_position; }
    [[nodiscard]] bool atEnd() const noexcept { return m_position >= m_file.size(); }

    // Moves to the start (leading marker) of a record, for instance one recorded earlier in an index.
    void seek(std::uint64_t position);

    // Reads the next record and moves past it. Throws IoError for a truncated record or markers that disagree.
    [[nodiscard]] RecordSpan nextRecord();

    // The bytes of a record returned by nextRecord().
    [[nodiscard]] std::span<const std::byte> dataOf(const RecordSpan& record) const noexcept
    {
        return m_file.subspan(static_cast<std::size_t>(record.dataOffset), static_cast<std::size_t>(record.length));
    }

private:
    std::span<const std::byte> m_file;
    FortranLayout m_layout;
    std::uint64_t m_position = 0;
};

} // namespace oor::io
