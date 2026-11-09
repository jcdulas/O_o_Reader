#include "oor/base/errors/Errors.hpp"
#include "oor/io/hdf5/Hdf5StreamWriter.hpp"
#include "oor/io/hdf5/Hdf5Table.hpp"
#include "oor/io/hdf5/Hdf5Tree.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

// The HDF5 file written by pieces: rows appended to resizable tables, laid out as C structs, read back by
// the generic reader; nothing is at the target until the commit, and an abort leaves nothing.

using namespace oor::io;

namespace
{

std::filesystem::path target(const std::string& name)
{
    const std::filesystem::path folder = std::filesystem::temp_directory_path() / "oor_h5_stream_tests";
    std::filesystem::create_directories(folder);
    std::filesystem::remove(folder / name);
    return folder / name;
}

template <typename T>
std::vector<T> valuesOf(const Hdf5Column& column)
{
    std::vector<T> values(column.data.size() / sizeof(T));
    std::memcpy(values.data(), column.data.data(), column.data.size());
    return values;
}

} // namespace

TEST_CASE("rows appended by pieces read back in order, members laid out as a C struct", "[io][hdf5]")
{
    const std::filesystem::path path = target("stream.h5");
    {
        Hdf5StreamWriter writer(path);
        const std::vector<Hdf5StreamMember> members = {{"EID", Hdf5ValueKind::Int64, 0, 1},
                                                       {"TERM", Hdf5ValueKind::FixedString, 4, 1},
                                                       {"X", Hdf5ValueKind::Float64, 0, 3},
                                                       {"DOMAIN_ID", Hdf5ValueKind::Int64, 0, 1}};
        const Hdf5StreamLayout& layout = writer.createTable("/A/B/T", members, 10);
        // EID at 0, TERM at 8, X aligned at 16, DOMAIN_ID at 40: 48 bytes a row
        CHECK(layout.offsets == std::vector<std::size_t>{0, 8, 16, 40});
        CHECK(layout.rowBytes == 48);
        CHECK_THROWS_AS(writer.createTable("/A/B/T", members, 10), oor::base::UsageError);
        for (std::int64_t piece = 0; piece < 3; ++piece)
        {
            std::vector<std::byte> rows(7 * layout.rowBytes, std::byte{0});
            for (std::int64_t row = 0; row < 7; ++row)
            {
                std::byte* at = rows.data() + row * layout.rowBytes;
                const std::int64_t id = piece * 7 + row;
                const double x[3] = {0.5 * static_cast<double>(id), 1.0, -2.0};
                std::memcpy(at, &id, 8);
                std::memcpy(at + 8, "CEN/", 4);
                std::memcpy(at + 16, x, sizeof(x));
                std::memcpy(at + 40, &piece, 8);
            }
            CHECK(writer.append("/A/B/T", rows, 7) == static_cast<std::uint64_t>(piece * 7));
        }
        CHECK(writer.rowCount("/A/B/T") == 21);
        writer.setAttribute("/", "SCHEMA", std::int64_t{20200});
        writer.setAttribute("/A", "VERSION", std::string("v2020.0"));
        writer.close();
        CHECK_FALSE(std::filesystem::exists(path));
        writer.commit();
    }
    REQUIRE(std::filesystem::exists(path));
    const Hdf5Table table = readHdf5Table(path, "/A/B/T", {});
    REQUIRE(table.rowCount == 21);
    const std::vector<std::int64_t> ids = valuesOf<std::int64_t>(*table.column("EID"));
    CHECK(ids.front() == 0);
    CHECK(ids.back() == 20);
    const std::vector<double> x = valuesOf<double>(*table.column("X"));
    CHECK(x.size() == 63);
    CHECK(x[3 * 20] == 10.0);
    CHECK(valuesOf<std::int64_t>(*table.column("DOMAIN_ID"))[14] == 2);
    const Hdf5Tree tree = Hdf5Tree::read(path);
    REQUIRE(tree.find("/A") != nullptr);
    CHECK(tree.find("/A")->attribute("VERSION") != nullptr);
}

TEST_CASE("a stream writer that is aborted or dropped leaves no file", "[io][hdf5]")
{
    const std::filesystem::path path = target("dropped.h5");
    {
        Hdf5StreamWriter writer(path);
        (void)writer.createTable("/T", {{"ID", Hdf5ValueKind::Int64, 0, 1}}, 10);
    }
    CHECK_FALSE(std::filesystem::exists(path));
    Hdf5StreamWriter writer(path);
    writer.abort();
    CHECK_FALSE(writer.isOpen());
    CHECK_FALSE(std::filesystem::exists(path));
    for (const auto& entry : std::filesystem::directory_iterator(path.parent_path()))
        CHECK(entry.path().filename().string().find("dropped.h5.") == std::string::npos);
}
