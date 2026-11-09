#include "oor/base/text/TextUtilities.hpp"
#include "oor/io/hdf5/Hdf5File.hpp"
#include "oor/io/hdf5/Hdf5Tree.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/core/ModelFileReader.hpp"
#include "oor/model/core/ModelFileWriter.hpp"
#include "oor/results/archive/ArchiveResultFile.hpp"
#include "oor/results/archive/ArchiveWriter.hpp"
#include "support/ModelFileRoundTrip.hpp"
#include "support/TestFiles.hpp"
#include "support/ValidationModels.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <bit>
#include <filesystem>

using namespace oor;

namespace
{

std::shared_ptr<model::Model> read(const std::filesystem::path& file)
{
    return std::make_shared<model::Model>(model::ModelFileReader::read(file, {}));
}

void checkModels(std::size_t first)
{
    const auto models = testing::validationModels(OOR_VALIDATION_DIR);
    REQUIRE(models.size() > 200);
    const testing::TestFiles files;
    for (std::size_t i = first; i < models.size(); i += 48)
    {
        INFO(models[i].string());
        auto source = read(models[i]);
        results::ArchiveWriteOptions settings;
        settings.model = source;
        const auto path = files.root() / "model.oorh5";
        REQUIRE(results::writeArchive(path, {}, settings).finished);
        const auto back = results::ArchiveResultFile::open(path)->readModel();
        CHECK(testing::cardTablesText(*back) == testing::cardTablesText(*source));
        CHECK(model::ModelFileWriter::toString(*back) == model::ModelFileWriter::toString(*source));
        CHECK(back->files().size() == source->files().size());
    }
}

} // namespace

TEST_CASE("native model columns preserve controls, includes, parts, keys and exact numbers", "[results][archive-model]")
{
    const testing::TestFiles files;
    files.write("nodes.oos", "# précision\nNode;1;;-0.;1.2345678901234567;0.\nNode;2;;1.;0.;0.\n");
    const auto source = read(files.write("model.oos",
        "[executive]\nSolutionSequence;101\n[case]\n# case title\nTitleText;\"Pièce; α\"\n"
        "SubcaseStart;1\nDisplacementOutput;All\n[model]\nInclude;nodes.oos\n"
        "ShellQuad4;10;1;1;2;3;4;;7\nShellQuad4;11;1;1;2;3;4;30.\n"
        "LoadCombination;20;1.;0.5;10;2.;11\nFutureCard;payload;;\"a;b\"\n"
        "[model;SuperelementPart;5;massId=300;append]\nNode;100;;5.;0.;0.\n"));
    REQUIRE_FALSE(source->diagnostics().hasErrors());
    // Keep a hole in the creation keys; the next added card must not reuse a persisted key.
    source->remove(*source->locate(base::IdSpace::Node, 2));
    results::ArchiveWriteOptions options;
    options.model = source;
    const auto path = files.root() / "native.oorh5";
    REQUIRE(results::writeArchive(path, {}, options).finished);
    // Sources are descriptors only. Reading the archive must not reopen them.
    std::filesystem::remove(files.root() / "nodes.oos");
    const auto archive = results::ArchiveResultFile::open(path);
    REQUIRE(archive->hasModel());
    const auto back = archive->readModel();
    CHECK(testing::cardTablesText(*back) == testing::cardTablesText(*source));
    CHECK(model::ModelFileWriter::toString(*back) == model::ModelFileWriter::toString(*source));
    REQUIRE(back->parts().size() == 1);
    CHECK(back->parts().front()->partInfo()->massId == 300);
    CHECK(back->parts().front()->partInfo()->appends);
    CHECK(back->nextKeyValue() == source->nextKeyValue());
    const auto* nodes = back->findTable<model::NodeTable>();
    REQUIRE(nodes != nullptr);
    CHECK(std::bit_cast<std::uint64_t>(nodes->xyz(0).x) == std::bit_cast<std::uint64_t>(-0.0));
    CHECK(nodes->xyz(0).y == 1.2345678901234567);
    const auto* originalNodes = source->findTable<model::NodeTable>();
    CHECK(nodes->card(0).blankFields == originalNodes->card(0).blankFields);
    CHECK(std::ranges::equal(nodes->rows().significantDigits(0), originalNodes->rows().significantDigits(0)));
    const auto next = back->newKey();
    CHECK(next.value == source->nextKeyValue());
    const auto tree = io::Hdf5Tree::read(path);
    CHECK(tree.find("/model/oos") == nullptr);
    CHECK(tree.find("/model/parts/0/cards/0/columns/value_offsets") != nullptr);
}

TEST_CASE("a native archive preserves an empty model", "[results][archive-model]")
{
    const testing::TestFiles files;
    results::ArchiveWriteOptions options;
    options.model = std::make_shared<model::Model>();
    const auto path = files.root() / "empty.oorh5";
    REQUIRE(results::writeArchive(path, {}, options).finished);
    const auto back = results::ArchiveResultFile::open(path)->readModel();
    CHECK(back->cardCount() == 0);
    CHECK(back->files().empty());
}

TEST_CASE("an unrepresentable model aborts the archive without replacing the target", "[results][archive-model]")
{
    const testing::TestFiles files;
    const auto path = files.write("existing.oorh5", "previous file");
    auto source = std::make_shared<model::Model>();
    model::ControlStatement raw;
    raw.kind = model::ControlKind::Raw;
    raw.text = "unsupported control statement";
    source->control().executive.entries().push_back(raw);
    results::ArchiveWriteOptions options;
    options.model = source;
    CHECK_THROWS_AS(results::writeArchive(path, {}, options), base::UsageError);
    CHECK(files.read("existing.oorh5") == "previous file");
    CHECK_FALSE(files.exists("existing.oorh5.oor_tmp"));
}

TEST_CASE("large native columns are compressed and loaded over several batches", "[results][archive-model]")
{
    const testing::TestFiles files;
    auto model = std::make_shared<model::Model>();
    for (std::int32_t id = 1; id <= 10000; ++id)
    {
        model::NodeCard node;
        node.id = id;
        node.xyz = {static_cast<double>(id), -0.0, 0.0};
        model->add<model::NodeTable>(node);
    }
    results::ArchiveWriteOptions options;
    options.model = model;
    const auto path = files.root() / "large.oorh5";
    REQUIRE(results::writeArchive(path, {}, options).finished);
    const auto tree = io::Hdf5Tree::read(path);
    const auto* reals = tree.find("/model/parts/0/cards/0/reals");
    REQUIRE(reals != nullptr);
    CHECK(reals->layout == "chunked");
    CHECK(reals->storageBytes < 10000 * 3 * sizeof(double));
    const auto back = results::ArchiveResultFile::open(path)->readModel();
    REQUIRE(back->cardCount() == 10000);
    CHECK(back->findTable<model::NodeTable>()->xyz(9999).x == 10000.0);
}

// Keep each corpus batch below the per-test timeout, including on busy Windows hosts.
#define OOR_ARCHIVE_MODEL_BATCH(index) \
    TEST_CASE("native archive validation batch " #index, "[results][archive-model][validation]") \
    { \
        checkModels(index); \
    }

OOR_ARCHIVE_MODEL_BATCH(0)
OOR_ARCHIVE_MODEL_BATCH(1)
OOR_ARCHIVE_MODEL_BATCH(2)
OOR_ARCHIVE_MODEL_BATCH(3)
OOR_ARCHIVE_MODEL_BATCH(4)
OOR_ARCHIVE_MODEL_BATCH(5)
OOR_ARCHIVE_MODEL_BATCH(6)
OOR_ARCHIVE_MODEL_BATCH(7)
OOR_ARCHIVE_MODEL_BATCH(8)
OOR_ARCHIVE_MODEL_BATCH(9)
OOR_ARCHIVE_MODEL_BATCH(10)
OOR_ARCHIVE_MODEL_BATCH(11)
OOR_ARCHIVE_MODEL_BATCH(12)
OOR_ARCHIVE_MODEL_BATCH(13)
OOR_ARCHIVE_MODEL_BATCH(14)
OOR_ARCHIVE_MODEL_BATCH(15)
OOR_ARCHIVE_MODEL_BATCH(16)
OOR_ARCHIVE_MODEL_BATCH(17)
OOR_ARCHIVE_MODEL_BATCH(18)
OOR_ARCHIVE_MODEL_BATCH(19)
OOR_ARCHIVE_MODEL_BATCH(20)
OOR_ARCHIVE_MODEL_BATCH(21)
OOR_ARCHIVE_MODEL_BATCH(22)
OOR_ARCHIVE_MODEL_BATCH(23)
OOR_ARCHIVE_MODEL_BATCH(24)
OOR_ARCHIVE_MODEL_BATCH(25)
OOR_ARCHIVE_MODEL_BATCH(26)
OOR_ARCHIVE_MODEL_BATCH(27)
OOR_ARCHIVE_MODEL_BATCH(28)
OOR_ARCHIVE_MODEL_BATCH(29)
OOR_ARCHIVE_MODEL_BATCH(30)
OOR_ARCHIVE_MODEL_BATCH(31)
OOR_ARCHIVE_MODEL_BATCH(32)
OOR_ARCHIVE_MODEL_BATCH(33)
OOR_ARCHIVE_MODEL_BATCH(34)
OOR_ARCHIVE_MODEL_BATCH(35)
OOR_ARCHIVE_MODEL_BATCH(36)
OOR_ARCHIVE_MODEL_BATCH(37)
OOR_ARCHIVE_MODEL_BATCH(38)
OOR_ARCHIVE_MODEL_BATCH(39)
OOR_ARCHIVE_MODEL_BATCH(40)
OOR_ARCHIVE_MODEL_BATCH(41)
OOR_ARCHIVE_MODEL_BATCH(42)
OOR_ARCHIVE_MODEL_BATCH(43)
OOR_ARCHIVE_MODEL_BATCH(44)
OOR_ARCHIVE_MODEL_BATCH(45)
OOR_ARCHIVE_MODEL_BATCH(46)
OOR_ARCHIVE_MODEL_BATCH(47)

#undef OOR_ARCHIVE_MODEL_BATCH
