#include "oor/base/errors/Errors.hpp"
#include "oor/io/zip/Zip.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace oor;

TEST_CASE("a ZIP archive gives back its files, compressed or stored", "[io][zip]")
{
    const std::string repeated(10000, 'a');
    for (const int level : {0, 1, 6, 9})
    {
        io::ZipWriter writer(level);
        writer.addFile("[Content_Types].xml", "<Types/>");
        writer.addFile("xl/worksheets/sheet1.xml", repeated);
        writer.addFile("empty.txt", "");
        writer.addFile("caf\xc3\xa9.txt", "UTF-8 name");
        const std::string bytes = writer.finish();
        CHECK(bytes.substr(0, 2) == "PK");
        if (level > 0)
            CHECK(bytes.size() < repeated.size() / 10); // the repeated text compresses well

        const std::vector<io::ZipFile> files = io::readZip(bytes);
        REQUIRE(files.size() == 4);
        CHECK(files[0].name == "[Content_Types].xml");
        CHECK(files[0].content == "<Types/>");
        CHECK(files[1].content == repeated);
        CHECK(files[2].content.empty());
        CHECK(files[3].name == "caf\xc3\xa9.txt");
    }
}

TEST_CASE("a damaged ZIP archive is refused", "[io][zip]")
{
    io::ZipWriter writer;
    writer.addFile("a.txt", "some text to compress, some text to compress");
    std::string bytes = writer.finish();
    CHECK_THROWS_AS(io::readZip(bytes.substr(0, 10)), base::IoError);
    bytes[40] = static_cast<char>(bytes[40] ^ 0x55); // inside the compressed data
    CHECK_THROWS_AS(io::readZip(bytes), base::IoError);
    CHECK_THROWS_AS(io::ZipWriter(10), base::UsageError);
}
