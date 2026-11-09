#include "oor/base/errors/Errors.hpp"
#include "oor/io/files/BufferedFile.hpp"
#include "oor/io/files/ByteSource.hpp"
#include "oor/io/files/MappedFile.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <vector>

using namespace oor::io;

namespace
{

class TemporaryFile
{
public:
    TemporaryFile(const std::string& name, const std::vector<char>& content)
        : m_path(std::filesystem::temp_directory_path() / name)
    {
        std::ofstream file(m_path, std::ios::binary);
        file.write(content.data(), static_cast<std::streamsize>(content.size()));
    }
    ~TemporaryFile()
    {
        std::error_code ignored;
        std::filesystem::remove(m_path, ignored);
    }
    [[nodiscard]] const std::filesystem::path& path() const { return m_path; }

private:
    std::filesystem::path m_path;
};

std::vector<char> randomBytes(std::size_t count)
{
    std::mt19937 generator(7);
    std::vector<char> bytes(count);
    for (char& byte : bytes)
        byte = static_cast<char>(generator() & 0xFF);
    return bytes;
}

bool sameBytes(std::span<const std::byte> span, const std::vector<char>& expected, std::size_t offset,
               std::size_t length)
{
    return span.size() == length && std::memcmp(span.data(), expected.data() + offset, length) == 0;
}

} // namespace

TEST_CASE("mapped and buffered sources return the same bytes", "[io][files]")
{
    const std::vector<char> content = randomBytes(10000);
    const TemporaryFile file("oor_io_same.bin", content);
    MappedFile mapped(file.path());
    BufferedFile buffered(file.path());
    REQUIRE(mapped.size() == content.size());
    REQUIRE(buffered.size() == content.size());
    for (std::size_t offset : {std::size_t{0}, std::size_t{1}, std::size_t{4999}, std::size_t{9990}})
    {
        const std::size_t length = std::min<std::size_t>(100, content.size() - offset);
        REQUIRE(sameBytes(mapped.read(offset, 100), content, offset, length));
        REQUIRE(sameBytes(buffered.read(offset, 100), content, offset, length));
    }
    REQUIRE(sameBytes(mapped.read(0, 1 << 20), content, 0, content.size()));
    REQUIRE(sameBytes(buffered.read(0, 1 << 20), content, 0, content.size()));
}

TEST_CASE("reads past the end give fewer bytes, then none", "[io][files]")
{
    const std::vector<char> content = randomBytes(100);
    const TemporaryFile file("oor_io_end.bin", content);
    MappedFile mapped(file.path());
    BufferedFile buffered(file.path());
    REQUIRE(mapped.read(90, 50).size() == 10);
    REQUIRE(buffered.read(90, 50).size() == 10);
    REQUIRE(mapped.read(100, 10).empty());
    REQUIRE(buffered.read(100, 10).empty());
    REQUIRE(mapped.read(5000, 10).empty());
    REQUIRE(buffered.read(5000, 10).empty());
}

TEST_CASE("an empty file is a valid empty source", "[io][files]")
{
    const TemporaryFile file("oor_io_empty.bin", {});
    MappedFile mapped(file.path());
    BufferedFile buffered(file.path());
    REQUIRE(mapped.size() == 0);
    REQUIRE(buffered.size() == 0);
    REQUIRE(mapped.read(0, 10).empty());
    REQUIRE(buffered.read(0, 10).empty());
}

TEST_CASE("a missing file throws IoError", "[io][files]")
{
    const auto missing = std::filesystem::temp_directory_path() / "oor_io_missing.bin";
    REQUIRE_THROWS_AS(MappedFile(missing), oor::base::IoError);
    REQUIRE_THROWS_AS(BufferedFile(missing), oor::base::IoError);
    REQUIRE_THROWS_AS(openByteSource(missing), oor::base::IoError);
}

TEST_CASE("openByteSource prefers a mapping and can be told not to", "[io][files]")
{
    const TemporaryFile file("oor_io_open.bin", randomBytes(64));
    REQUIRE(openByteSource(file.path())->isStable());
    REQUIRE_FALSE(openByteSource(file.path(), false)->isStable());
}
