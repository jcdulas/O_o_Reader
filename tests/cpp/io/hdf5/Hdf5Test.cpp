#include "oor/base/errors/Errors.hpp"
#include "oor/io/hdf5/Hdf5File.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>

using namespace oor::io;
using oor::base::IoError;

namespace
{

class TemporaryPath
{
public:
    explicit TemporaryPath(const std::string& name) : m_path(std::filesystem::temp_directory_path() / name)
    {
        std::error_code ignored;
        std::filesystem::remove(m_path, ignored);
    }
    ~TemporaryPath()
    {
        std::error_code ignored;
        std::filesystem::remove(m_path, ignored);
    }
    [[nodiscard]] const std::filesystem::path& path() const { return m_path; }

private:
    std::filesystem::path m_path;
};

} // namespace

TEST_CASE("a written file is read back", "[io][hdf5]")
{
    const TemporaryPath file("oor_io_roundtrip.h5");
    {
        Hdf5File writer = Hdf5File::create(file.path());
        Hdf5Group results = writer.createGroup("MODEL");
        Hdf5Group table = results.createGroup("RESULT");
        table.writeDoubles("DISPLACEMENT", {3, 2}, {1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
        table.writeInt64("IDS", {3}, {10, 20, 30});
        table.setStringAttribute("TITLE", "plate");
    }
    REQUIRE(openHdf5ObjectCount() == 0);

    const Hdf5File reader = Hdf5File::openReadOnly(file.path());
    REQUIRE(reader.childNames() == std::vector<std::string>{"MODEL"});
    REQUIRE(reader.hasChild("MODEL/RESULT/DISPLACEMENT"));
    REQUIRE_FALSE(reader.hasChild("MODEL/RESULT/MISSING"));
    REQUIRE_FALSE(reader.hasChild("NOPE/RESULT"));
    REQUIRE(reader.isGroup("MODEL/RESULT"));
    REQUIRE_FALSE(reader.isGroup("MODEL/RESULT/IDS"));

    const Hdf5Group table = reader.openGroup("MODEL/RESULT");
    REQUIRE(table.readStringAttribute("TITLE") == "plate");
    const Hdf5Dataset displacement = table.openDataset("DISPLACEMENT");
    REQUIRE(displacement.shape() == std::vector<std::uint64_t>{3, 2});
    REQUIRE(displacement.rowCount() == 3);
    REQUIRE(displacement.valuesPerRow() == 2);
    REQUIRE(displacement.typeClass() == Hdf5TypeClass::Float);
    REQUIRE(displacement.elementSize() == 8);
    REQUIRE(displacement.readDoubles(0, 3) == std::vector<double>{1, 2, 3, 4, 5, 6});
    REQUIRE(displacement.readDoubles(1, 1) == std::vector<double>{3, 4});
    REQUIRE(displacement.readDoubles(2, 100) == std::vector<double>{5, 6}); // clamped to the end
    REQUIRE(displacement.readDoubles(3, 1).empty());
    const Hdf5Dataset ids = table.openDataset("IDS");
    REQUIRE(ids.typeClass() == Hdf5TypeClass::Integer);
    REQUIRE(ids.readInt64(0, 3) == std::vector<std::int64_t>{10, 20, 30});
    REQUIRE(ids.readDoubles(0, 3) == std::vector<double>{10, 20, 30}); // converted
    REQUIRE(ids.readBytes(0, 3).size() == 24);
}

TEST_CASE("errors are IoError, with no HDF5 noise and no leaked object", "[io][hdf5]")
{
    const TemporaryPath missing("oor_io_missing.h5");
    REQUIRE_THROWS_AS(Hdf5File::openReadOnly(missing.path()), IoError);

    const TemporaryPath notHdf5("oor_io_text.h5");
    {
        std::ofstream(notHdf5.path()) << "this is not an HDF5 file";
    }
    REQUIRE_THROWS_AS(Hdf5File::openReadOnly(notHdf5.path()), IoError);

    const TemporaryPath file("oor_io_errors.h5");
    {
        Hdf5File writer = Hdf5File::create(file.path());
        writer.writeDoubles("data", {2}, {1.0, 2.0});
        REQUIRE_THROWS_AS(writer.writeDoubles("data", {2}, {1.0, 2.0}), IoError); // exists already
        REQUIRE_THROWS_AS(writer.writeDoubles("other", {3}, {1.0, 2.0}), oor::base::UsageError);
    }
    {
        const Hdf5File reader = Hdf5File::openReadOnly(file.path());
        REQUIRE_THROWS_AS(reader.openDataset("nope"), IoError);
        REQUIRE_THROWS_AS(reader.openGroup("data"), IoError);
        REQUIRE_THROWS_AS(reader.readStringAttribute("absent"), IoError);
        const Hdf5Dataset dataset = reader.openDataset("data");
        REQUIRE_FALSE(dataset.hasAttribute("absent"));
        REQUIRE_THROWS_AS(dataset.readStringAttribute("absent"), IoError);
    }
    REQUIRE(openHdf5ObjectCount() == 0);
}
