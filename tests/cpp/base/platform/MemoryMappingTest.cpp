#include "oor/base/errors/Errors.hpp"
#include "oor/base/platform/MemoryMapping.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

using namespace oor::base;

namespace
{

// A file in the temporary directory that is removed when the object goes away.
class TemporaryFile
{
public:
    explicit TemporaryFile(const std::string& content, const std::string& name = "oor_mapping_test.bin")
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

} // namespace

TEST_CASE("a file is mapped with its exact bytes", "[base][platform]")
{
    const std::string content = "Node;1;;0.;0.;0.\r\nNode;2;;1.;0.;0.\n";
    const TemporaryFile file(content);
    const MemoryMapping mapping = MemoryMapping::openReadOnly(file.path());
    REQUIRE(mapping.size() == content.size());
    REQUIRE(std::memcmp(mapping.data(), content.data(), content.size()) == 0);
    REQUIRE(mapping.bytes().size() == content.size());
}

TEST_CASE("an empty file gives an empty mapping", "[base][platform]")
{
    const TemporaryFile file("", "oor_mapping_empty.bin");
    const MemoryMapping mapping = MemoryMapping::openReadOnly(file.path());
    REQUIRE(mapping.size() == 0);
    REQUIRE(mapping.data() == nullptr);
}

TEST_CASE("a missing file throws IoError naming the path", "[base][platform]")
{
    const auto missing = std::filesystem::temp_directory_path() / "oor_no_such_file.oos";
    try
    {
        (void)MemoryMapping::openReadOnly(missing);
        FAIL("expected an IoError");
    }
    catch (const IoError& error)
    {
        REQUIRE(std::string(error.what()).find("oor_no_such_file.oos") != std::string::npos);
    }
}

TEST_CASE("a mapping can be moved", "[base][platform]")
{
    const TemporaryFile file("abc", "oor_mapping_move.bin");
    MemoryMapping first = MemoryMapping::openReadOnly(file.path());
    MemoryMapping second = std::move(first);
    REQUIRE(second.size() == 3);
    REQUIRE(first.size() == 0);
    MemoryMapping third;
    third = std::move(second);
    REQUIRE(third.size() == 3);
}
