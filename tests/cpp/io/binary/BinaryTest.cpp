#include "oor/base/errors/Errors.hpp"
#include "oor/io/binary/EndianReader.hpp"
#include "oor/io/binary/FortranRecordReader.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <bit>
#include <cstring>
#include <vector>

using namespace oor::io;
using oor::base::IoError;

namespace
{

// Builds files byte by byte, in a chosen byte order and marker width, so that the tests do not depend on the host.
class ByteWriter
{
public:
    explicit ByteWriter(ByteOrder order) : m_order(order) {}

    void u32(std::uint32_t value) { put(value, 4); }
    void u64(std::uint64_t value) { put(value, 8); }
    void f32(float value) { put(std::bit_cast<std::uint32_t>(value), 4); }
    void f64(double value) { put(std::bit_cast<std::uint64_t>(value), 8); }
    void raw(const std::vector<std::byte>& bytes) { m_bytes.insert(m_bytes.end(), bytes.begin(), bytes.end()); }

    void record(const std::vector<std::byte>& data, int markerWidth)
    {
        put(data.size(), markerWidth);
        raw(data);
        put(data.size(), markerWidth);
    }

    [[nodiscard]] std::span<const std::byte> bytes() const { return m_bytes; }
    [[nodiscard]] std::vector<std::byte>& storage() { return m_bytes; }

private:
    void put(std::uint64_t value, int width)
    {
        for (int index = 0; index < width; ++index)
        {
            const int shift = (m_order == ByteOrder::Little ? index : width - 1 - index) * 8;
            m_bytes.push_back(static_cast<std::byte>((value >> shift) & 0xFF));
        }
    }

    ByteOrder m_order;
    std::vector<std::byte> m_bytes;
};

std::vector<std::byte> bytesOf(std::initializer_list<int> values)
{
    std::vector<std::byte> bytes;
    for (int value : values)
        bytes.push_back(static_cast<std::byte>(value));
    return bytes;
}

} // namespace

TEST_CASE("numbers are read in both byte orders", "[io][binary]")
{
    for (ByteOrder order : {ByteOrder::Little, ByteOrder::Big})
    {
        ByteWriter writer(order);
        writer.u32(0xFFFFFFFEu); // -2 as int32
        writer.u64(0xFFFFFFFFFFFFFFFDULL);
        writer.f32(1.5f);
        writer.f64(-2.25);
        EndianReader reader(writer.bytes(), order);
        REQUIRE(reader.readInt32() == -2);
        REQUIRE(reader.readInt64() == -3);
        REQUIRE(reader.readFloat32() == 1.5f);
        REQUIRE(reader.readFloat64() == -2.25);
        REQUIRE(reader.remaining() == 0);
    }
}

TEST_CASE("the integer width is 4 or 8 bytes", "[io][binary]")
{
    ByteWriter writer(ByteOrder::Big);
    writer.u32(7);
    writer.u64(9);
    EndianReader reader(writer.bytes(), ByteOrder::Big);
    REQUIRE(reader.readInteger(4) == 7);
    REQUIRE(reader.readInteger(8) == 9);
    REQUIRE_THROWS_AS(reader.readInteger(2), oor::base::UsageError);
}

TEST_CASE("a read past the end throws instead of reading out of bounds", "[io][binary]")
{
    const auto bytes = bytesOf({1, 2, 3});
    EndianReader reader(bytes, ByteOrder::Little);
    REQUIRE_THROWS_AS(reader.readInt32(), IoError);
    REQUIRE(reader.position() == 0);
    REQUIRE_THROWS_AS(reader.readBytes(4), IoError);
    REQUIRE_THROWS_AS(reader.skip(4), IoError);
    REQUIRE_THROWS_AS(reader.seek(4), IoError);
    reader.seek(3);
    REQUIRE(reader.remaining() == 0);
}

TEST_CASE("records are read with 4 and 8 byte markers in both byte orders", "[io][binary]")
{
    for (ByteOrder order : {ByteOrder::Little, ByteOrder::Big})
    {
        for (int width : {4, 8})
        {
            ByteWriter writer(order);
            writer.record(bytesOf({1, 2, 3, 4}), width);
            writer.record({}, width); // an empty record is valid
            writer.record(bytesOf({9, 8, 7, 6, 5, 4, 3, 2}), width);
            const auto layout = FortranRecordReader::detectLayout(writer.bytes());
            REQUIRE(layout.has_value());
            REQUIRE(layout->byteOrder == order);
            REQUIRE(layout->markerWidth == width);

            FortranRecordReader reader(writer.bytes(), *layout);
            const RecordSpan first = reader.nextRecord();
            REQUIRE(first.length == 4);
            REQUIRE(reader.dataOf(first)[3] == std::byte{4});
            REQUIRE(reader.nextRecord().length == 0);
            const RecordSpan third = reader.nextRecord();
            REQUIRE(third.length == 8);
            REQUIRE(reader.dataOf(third)[0] == std::byte{9});
            REQUIRE(reader.atEnd());
        }
    }
}

TEST_CASE("a corrupt or truncated record raises IoError at that record", "[io][binary]")
{
    ByteWriter writer(ByteOrder::Little);
    writer.record(bytesOf({1, 2, 3, 4}), 4);
    writer.record(bytesOf({5, 6, 7, 8}), 4);
    const FortranLayout layout{ByteOrder::Little, 4};

    SECTION("trailing marker differs")
    {
        writer.storage()[8 + 4 + 4 + 4 + 4 - 4] = std::byte{99}; // second record's trailing marker
        FortranRecordReader reader(writer.bytes(), layout);
        (void)reader.nextRecord();
        REQUIRE_THROWS_AS(reader.nextRecord(), IoError);
    }
    SECTION("the file ends inside a record")
    {
        writer.storage().resize(writer.storage().size() - 3);
        FortranRecordReader reader(writer.bytes(), layout);
        (void)reader.nextRecord();
        REQUIRE_THROWS_AS(reader.nextRecord(), IoError);
    }
    SECTION("a huge length marker is refused without allocating")
    {
        writer.storage()[0] = std::byte{0xFF};
        writer.storage()[1] = std::byte{0xFF};
        writer.storage()[2] = std::byte{0xFF};
        writer.storage()[3] = std::byte{0x7F};
        FortranRecordReader reader(writer.bytes(), layout);
        REQUIRE_THROWS_AS(reader.nextRecord(), IoError);
    }
    SECTION("seeking back to a recorded offset")
    {
        FortranRecordReader reader(writer.bytes(), layout);
        const RecordSpan first = reader.nextRecord();
        const std::uint64_t secondStart = reader.position();
        (void)reader.nextRecord();
        reader.seek(secondStart);
        REQUIRE(reader.dataOf(reader.nextRecord())[0] == std::byte{5});
        reader.seek(first.dataOffset - 4);
        REQUIRE(reader.nextRecord().dataOffset == first.dataOffset);
        REQUIRE_THROWS_AS(reader.seek(1000), IoError);
    }
}

TEST_CASE("text that is not a Fortran file has no layout", "[io][binary]")
{
    const std::string text = "[executive]\nSolutionSequence;101\n[model]\n";
    std::vector<std::byte> bytes(text.size());
    std::memcpy(bytes.data(), text.data(), text.size());
    REQUIRE_FALSE(FortranRecordReader::detectLayout(bytes).has_value());
    REQUIRE_FALSE(FortranRecordReader::detectLayout({}).has_value());
}
