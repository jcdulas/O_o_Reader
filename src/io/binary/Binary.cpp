#include "oor/base/errors/Errors.hpp"
#include "oor/io/binary/EndianReader.hpp"
#include "oor/io/binary/FortranRecordReader.hpp"

#include <string>

namespace oor::io
{

namespace
{

constexpr std::uint32_t swapBytes(std::uint32_t value) noexcept
{
    return ((value & 0x000000FFu) << 24) | ((value & 0x0000FF00u) << 8) | ((value & 0x00FF0000u) >> 8) |
           ((value & 0xFF000000u) >> 24);
}

constexpr std::uint64_t swapBytes(std::uint64_t value) noexcept
{
    return (static_cast<std::uint64_t>(swapBytes(static_cast<std::uint32_t>(value))) << 32) |
           swapBytes(static_cast<std::uint32_t>(value >> 32));
}

} // namespace

std::uint32_t loadUint32(const std::byte* bytes, ByteOrder order) noexcept
{
    std::uint32_t value;
    std::memcpy(&value, bytes, sizeof(value));
    return order == hostByteOrder() ? value : swapBytes(value);
}

std::uint64_t loadUint64(const std::byte* bytes, ByteOrder order) noexcept
{
    std::uint64_t value;
    std::memcpy(&value, bytes, sizeof(value));
    return order == hostByteOrder() ? value : swapBytes(value);
}

void EndianReader::require(std::size_t count) const
{
    if (count > remaining())
        throw base::IoError("unexpected end of data at offset " + std::to_string(m_position) + ": needed " +
                            std::to_string(count) + " bytes, " + std::to_string(remaining()) + " left");
}

void EndianReader::seek(std::size_t position)
{
    if (position > m_data.size())
        throw base::IoError("seek past the end of data: offset " + std::to_string(position));
    m_position = position;
}

void EndianReader::skip(std::size_t count)
{
    require(count);
    m_position += count;
}

std::uint32_t EndianReader::readUint32()
{
    require(4);
    const std::uint32_t value = loadUint32(m_data.data() + m_position, m_order);
    m_position += 4;
    return value;
}

std::uint64_t EndianReader::readUint64()
{
    require(8);
    const std::uint64_t value = loadUint64(m_data.data() + m_position, m_order);
    m_position += 8;
    return value;
}

std::int32_t EndianReader::readInt32()
{
    return static_cast<std::int32_t>(readUint32());
}

std::int64_t EndianReader::readInt64()
{
    return static_cast<std::int64_t>(readUint64());
}

float EndianReader::readFloat32()
{
    return std::bit_cast<float>(readUint32());
}

double EndianReader::readFloat64()
{
    return std::bit_cast<double>(readUint64());
}

std::int64_t EndianReader::readInteger(int widthInBytes)
{
    if (widthInBytes == 4)
        return readInt32();
    if (widthInBytes == 8)
        return readInt64();
    throw base::UsageError("integer width must be 4 or 8 bytes, not " + std::to_string(widthInBytes));
}

std::span<const std::byte> EndianReader::readBytes(std::size_t count)
{
    require(count);
    const std::span<const std::byte> bytes = m_data.subspan(m_position, count);
    m_position += count;
    return bytes;
}

std::optional<FortranLayout> FortranRecordReader::detectLayout(std::span<const std::byte> file) noexcept
{
    for (ByteOrder order : {ByteOrder::Little, ByteOrder::Big})
    {
        for (int width : {4, 8})
        {
            const std::size_t w = static_cast<std::size_t>(width);
            if (file.size() < 2 * w)
                continue;
            const std::uint64_t length = width == 4 ? loadUint32(file.data(), order) : loadUint64(file.data(), order);
            if (length > file.size() - 2 * w)
                continue;
            const std::byte* trailing = file.data() + w + length;
            const std::uint64_t check = width == 4 ? loadUint32(trailing, order) : loadUint64(trailing, order);
            if (check == length)
                return FortranLayout{order, width};
        }
    }
    return std::nullopt;
}

void FortranRecordReader::seek(std::uint64_t position)
{
    if (position > m_file.size())
        throw base::IoError("seek past the end of file: offset " + std::to_string(position));
    m_position = position;
}

RecordSpan FortranRecordReader::nextRecord()
{
    const std::size_t markerWidth = static_cast<std::size_t>(m_layout.markerWidth);
    const std::uint64_t start = m_position;
    const std::uint64_t fileSize = m_file.size();
    if (start > fileSize || fileSize - start < 2 * markerWidth)
        throw base::IoError("truncated record marker at offset " + std::to_string(start));
    const std::byte* leadingPosition = m_file.data() + start;
    const std::uint64_t length = markerWidth == 4 ? loadUint32(leadingPosition, m_layout.byteOrder)
                                                  : loadUint64(leadingPosition, m_layout.byteOrder);
    if (length > fileSize - start - 2 * markerWidth)
        throw base::IoError("record at offset " + std::to_string(start) + " claims " + std::to_string(length) +
                            " bytes but the file ends first");
    const std::byte* trailingPosition = leadingPosition + markerWidth + length;
    const std::uint64_t trailing = markerWidth == 4 ? loadUint32(trailingPosition, m_layout.byteOrder)
                                                    : loadUint64(trailingPosition, m_layout.byteOrder);
    if (trailing != length)
        throw base::IoError("record at offset " + std::to_string(start) + ": leading marker " + std::to_string(length) +
                            " and trailing marker " + std::to_string(trailing) + " differ");
    m_position = start + 2 * markerWidth + length;
    return RecordSpan{start + markerWidth, length};
}

} // namespace oor::io
