#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

namespace oor::io
{

// Byte order of a binary file. Detected from the file, never assumed from the host.
enum class ByteOrder
{
    Little,
    Big
};

[[nodiscard]] constexpr ByteOrder hostByteOrder() noexcept
{
    return std::endian::native == std::endian::little ? ByteOrder::Little : ByteOrder::Big;
}

// Loads an integer or float from `bytes` (which must hold enough bytes) in the given byte order.
[[nodiscard]] std::uint32_t loadUint32(const std::byte* bytes, ByteOrder order) noexcept;
[[nodiscard]] std::uint64_t loadUint64(const std::byte* bytes, ByteOrder order) noexcept;

// Reads numbers sequentially from a block of bytes, in a fixed byte order. Every read is bounds-checked and a read
// past the end throws IoError ("unexpected end of data"), so a truncated file never reads out of bounds.
class EndianReader
{
public:
    EndianReader(std::span<const std::byte> data, ByteOrder order) noexcept : m_data(data), m_order(order) {}

    [[nodiscard]] ByteOrder byteOrder() const noexcept { return m_order; }
    [[nodiscard]] std::size_t position() const noexcept { return m_position; }
    [[nodiscard]] std::size_t size() const noexcept { return m_data.size(); }
    [[nodiscard]] std::size_t remaining() const noexcept { return m_data.size() - m_position; }

    void seek(std::size_t position);
    void skip(std::size_t count);

    [[nodiscard]] std::int32_t readInt32();
    [[nodiscard]] std::int64_t readInt64();
    [[nodiscard]] std::uint32_t readUint32();
    [[nodiscard]] std::uint64_t readUint64();
    [[nodiscard]] float readFloat32();
    [[nodiscard]] double readFloat64();
    // A signed integer of 4 or 8 bytes (the integer width of the file).
    [[nodiscard]] std::int64_t readInteger(int widthInBytes);
    [[nodiscard]] std::span<const std::byte> readBytes(std::size_t count);

private:
    void require(std::size_t count) const;

    std::span<const std::byte> m_data;
    ByteOrder m_order;
    std::size_t m_position = 0;
};

} // namespace oor::io
