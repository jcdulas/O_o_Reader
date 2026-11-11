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

constexpr std::size_t ARCHIVE_MODEL_CASE_COUNT = 256;

void checkModel(std::size_t index)
{
    const auto models = testing::validationModels(OOR_VALIDATION_DIR);
    REQUIRE(models.size() > 200);
    REQUIRE(models.size() <= ARCHIVE_MODEL_CASE_COUNT);
    if (index >= models.size())
        return;
    const testing::TestFiles files;
    INFO(models[index].string());
    auto source = read(models[index]);
    results::ArchiveWriteOptions settings;
    settings.model = source;
    const auto path = files.root() / "model.ooh5";
    REQUIRE(results::writeArchive(path, {}, settings).finished);
    const auto back = results::ArchiveResultFile::open(path)->readModel();
    CHECK(testing::cardTablesText(*back) == testing::cardTablesText(*source));
    CHECK(model::ModelFileWriter::toString(*back) == model::ModelFileWriter::toString(*source));
    CHECK(back->files().size() == source->files().size());
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
    const auto path = files.root() / "native.ooh5";
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
    const auto path = files.root() / "empty.ooh5";
    REQUIRE(results::writeArchive(path, {}, options).finished);
    const auto back = results::ArchiveResultFile::open(path)->readModel();
    CHECK(back->cardCount() == 0);
    CHECK(back->files().empty());
}

TEST_CASE("an unrepresentable model aborts the archive without replacing the target", "[results][archive-model]")
{
    const testing::TestFiles files;
    const auto path = files.write("existing.ooh5", "previous file");
    auto source = std::make_shared<model::Model>();
    model::ControlStatement raw;
    raw.kind = model::ControlKind::Raw;
    raw.text = "unsupported control statement";
    source->control().executive.entries().push_back(raw);
    results::ArchiveWriteOptions options;
    options.model = source;
    CHECK_THROWS_AS(results::writeArchive(path, {}, options), base::UsageError);
    CHECK(files.read("existing.ooh5") == "previous file");
    CHECK_FALSE(files.exists("existing.ooh5.oor_tmp"));
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
    const auto path = files.root() / "large.ooh5";
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

// One model per test keeps the archive round trip below the one-second test limit.
#define OOR_ARCHIVE_MODEL_CASE(index) \
    TEST_CASE("native archive validation model " #index, "[results][archive-model][validation]") \
    { \
        checkModel(index); \
    }

OOR_ARCHIVE_MODEL_CASE(0)
OOR_ARCHIVE_MODEL_CASE(1)
OOR_ARCHIVE_MODEL_CASE(2)
OOR_ARCHIVE_MODEL_CASE(3)
OOR_ARCHIVE_MODEL_CASE(4)
OOR_ARCHIVE_MODEL_CASE(5)
OOR_ARCHIVE_MODEL_CASE(6)
OOR_ARCHIVE_MODEL_CASE(7)
OOR_ARCHIVE_MODEL_CASE(8)
OOR_ARCHIVE_MODEL_CASE(9)
OOR_ARCHIVE_MODEL_CASE(10)
OOR_ARCHIVE_MODEL_CASE(11)
OOR_ARCHIVE_MODEL_CASE(12)
OOR_ARCHIVE_MODEL_CASE(13)
OOR_ARCHIVE_MODEL_CASE(14)
OOR_ARCHIVE_MODEL_CASE(15)
OOR_ARCHIVE_MODEL_CASE(16)
OOR_ARCHIVE_MODEL_CASE(17)
OOR_ARCHIVE_MODEL_CASE(18)
OOR_ARCHIVE_MODEL_CASE(19)
OOR_ARCHIVE_MODEL_CASE(20)
OOR_ARCHIVE_MODEL_CASE(21)
OOR_ARCHIVE_MODEL_CASE(22)
OOR_ARCHIVE_MODEL_CASE(23)
OOR_ARCHIVE_MODEL_CASE(24)
OOR_ARCHIVE_MODEL_CASE(25)
OOR_ARCHIVE_MODEL_CASE(26)
OOR_ARCHIVE_MODEL_CASE(27)
OOR_ARCHIVE_MODEL_CASE(28)
OOR_ARCHIVE_MODEL_CASE(29)
OOR_ARCHIVE_MODEL_CASE(30)
OOR_ARCHIVE_MODEL_CASE(31)
OOR_ARCHIVE_MODEL_CASE(32)
OOR_ARCHIVE_MODEL_CASE(33)
OOR_ARCHIVE_MODEL_CASE(34)
OOR_ARCHIVE_MODEL_CASE(35)
OOR_ARCHIVE_MODEL_CASE(36)
OOR_ARCHIVE_MODEL_CASE(37)
OOR_ARCHIVE_MODEL_CASE(38)
OOR_ARCHIVE_MODEL_CASE(39)
OOR_ARCHIVE_MODEL_CASE(40)
OOR_ARCHIVE_MODEL_CASE(41)
OOR_ARCHIVE_MODEL_CASE(42)
OOR_ARCHIVE_MODEL_CASE(43)
OOR_ARCHIVE_MODEL_CASE(44)
OOR_ARCHIVE_MODEL_CASE(45)
OOR_ARCHIVE_MODEL_CASE(46)
OOR_ARCHIVE_MODEL_CASE(47)
OOR_ARCHIVE_MODEL_CASE(48)
OOR_ARCHIVE_MODEL_CASE(49)
OOR_ARCHIVE_MODEL_CASE(50)
OOR_ARCHIVE_MODEL_CASE(51)
OOR_ARCHIVE_MODEL_CASE(52)
OOR_ARCHIVE_MODEL_CASE(53)
OOR_ARCHIVE_MODEL_CASE(54)
OOR_ARCHIVE_MODEL_CASE(55)
OOR_ARCHIVE_MODEL_CASE(56)
OOR_ARCHIVE_MODEL_CASE(57)
OOR_ARCHIVE_MODEL_CASE(58)
OOR_ARCHIVE_MODEL_CASE(59)
OOR_ARCHIVE_MODEL_CASE(60)
OOR_ARCHIVE_MODEL_CASE(61)
OOR_ARCHIVE_MODEL_CASE(62)
OOR_ARCHIVE_MODEL_CASE(63)
OOR_ARCHIVE_MODEL_CASE(64)
OOR_ARCHIVE_MODEL_CASE(65)
OOR_ARCHIVE_MODEL_CASE(66)
OOR_ARCHIVE_MODEL_CASE(67)
OOR_ARCHIVE_MODEL_CASE(68)
OOR_ARCHIVE_MODEL_CASE(69)
OOR_ARCHIVE_MODEL_CASE(70)
OOR_ARCHIVE_MODEL_CASE(71)
OOR_ARCHIVE_MODEL_CASE(72)
OOR_ARCHIVE_MODEL_CASE(73)
OOR_ARCHIVE_MODEL_CASE(74)
OOR_ARCHIVE_MODEL_CASE(75)
OOR_ARCHIVE_MODEL_CASE(76)
OOR_ARCHIVE_MODEL_CASE(77)
OOR_ARCHIVE_MODEL_CASE(78)
OOR_ARCHIVE_MODEL_CASE(79)
OOR_ARCHIVE_MODEL_CASE(80)
OOR_ARCHIVE_MODEL_CASE(81)
OOR_ARCHIVE_MODEL_CASE(82)
OOR_ARCHIVE_MODEL_CASE(83)
OOR_ARCHIVE_MODEL_CASE(84)
OOR_ARCHIVE_MODEL_CASE(85)
OOR_ARCHIVE_MODEL_CASE(86)
OOR_ARCHIVE_MODEL_CASE(87)
OOR_ARCHIVE_MODEL_CASE(88)
OOR_ARCHIVE_MODEL_CASE(89)
OOR_ARCHIVE_MODEL_CASE(90)
OOR_ARCHIVE_MODEL_CASE(91)
OOR_ARCHIVE_MODEL_CASE(92)
OOR_ARCHIVE_MODEL_CASE(93)
OOR_ARCHIVE_MODEL_CASE(94)
OOR_ARCHIVE_MODEL_CASE(95)
OOR_ARCHIVE_MODEL_CASE(96)
OOR_ARCHIVE_MODEL_CASE(97)
OOR_ARCHIVE_MODEL_CASE(98)
OOR_ARCHIVE_MODEL_CASE(99)
OOR_ARCHIVE_MODEL_CASE(100)
OOR_ARCHIVE_MODEL_CASE(101)
OOR_ARCHIVE_MODEL_CASE(102)
OOR_ARCHIVE_MODEL_CASE(103)
OOR_ARCHIVE_MODEL_CASE(104)
OOR_ARCHIVE_MODEL_CASE(105)
OOR_ARCHIVE_MODEL_CASE(106)
OOR_ARCHIVE_MODEL_CASE(107)
OOR_ARCHIVE_MODEL_CASE(108)
OOR_ARCHIVE_MODEL_CASE(109)
OOR_ARCHIVE_MODEL_CASE(110)
OOR_ARCHIVE_MODEL_CASE(111)
OOR_ARCHIVE_MODEL_CASE(112)
OOR_ARCHIVE_MODEL_CASE(113)
OOR_ARCHIVE_MODEL_CASE(114)
OOR_ARCHIVE_MODEL_CASE(115)
OOR_ARCHIVE_MODEL_CASE(116)
OOR_ARCHIVE_MODEL_CASE(117)
OOR_ARCHIVE_MODEL_CASE(118)
OOR_ARCHIVE_MODEL_CASE(119)
OOR_ARCHIVE_MODEL_CASE(120)
OOR_ARCHIVE_MODEL_CASE(121)
OOR_ARCHIVE_MODEL_CASE(122)
OOR_ARCHIVE_MODEL_CASE(123)
OOR_ARCHIVE_MODEL_CASE(124)
OOR_ARCHIVE_MODEL_CASE(125)
OOR_ARCHIVE_MODEL_CASE(126)
OOR_ARCHIVE_MODEL_CASE(127)
OOR_ARCHIVE_MODEL_CASE(128)
OOR_ARCHIVE_MODEL_CASE(129)
OOR_ARCHIVE_MODEL_CASE(130)
OOR_ARCHIVE_MODEL_CASE(131)
OOR_ARCHIVE_MODEL_CASE(132)
OOR_ARCHIVE_MODEL_CASE(133)
OOR_ARCHIVE_MODEL_CASE(134)
OOR_ARCHIVE_MODEL_CASE(135)
OOR_ARCHIVE_MODEL_CASE(136)
OOR_ARCHIVE_MODEL_CASE(137)
OOR_ARCHIVE_MODEL_CASE(138)
OOR_ARCHIVE_MODEL_CASE(139)
OOR_ARCHIVE_MODEL_CASE(140)
OOR_ARCHIVE_MODEL_CASE(141)
OOR_ARCHIVE_MODEL_CASE(142)
OOR_ARCHIVE_MODEL_CASE(143)
OOR_ARCHIVE_MODEL_CASE(144)
OOR_ARCHIVE_MODEL_CASE(145)
OOR_ARCHIVE_MODEL_CASE(146)
OOR_ARCHIVE_MODEL_CASE(147)
OOR_ARCHIVE_MODEL_CASE(148)
OOR_ARCHIVE_MODEL_CASE(149)
OOR_ARCHIVE_MODEL_CASE(150)
OOR_ARCHIVE_MODEL_CASE(151)
OOR_ARCHIVE_MODEL_CASE(152)
OOR_ARCHIVE_MODEL_CASE(153)
OOR_ARCHIVE_MODEL_CASE(154)
OOR_ARCHIVE_MODEL_CASE(155)
OOR_ARCHIVE_MODEL_CASE(156)
OOR_ARCHIVE_MODEL_CASE(157)
OOR_ARCHIVE_MODEL_CASE(158)
OOR_ARCHIVE_MODEL_CASE(159)
OOR_ARCHIVE_MODEL_CASE(160)
OOR_ARCHIVE_MODEL_CASE(161)
OOR_ARCHIVE_MODEL_CASE(162)
OOR_ARCHIVE_MODEL_CASE(163)
OOR_ARCHIVE_MODEL_CASE(164)
OOR_ARCHIVE_MODEL_CASE(165)
OOR_ARCHIVE_MODEL_CASE(166)
OOR_ARCHIVE_MODEL_CASE(167)
OOR_ARCHIVE_MODEL_CASE(168)
OOR_ARCHIVE_MODEL_CASE(169)
OOR_ARCHIVE_MODEL_CASE(170)
OOR_ARCHIVE_MODEL_CASE(171)
OOR_ARCHIVE_MODEL_CASE(172)
OOR_ARCHIVE_MODEL_CASE(173)
OOR_ARCHIVE_MODEL_CASE(174)
OOR_ARCHIVE_MODEL_CASE(175)
OOR_ARCHIVE_MODEL_CASE(176)
OOR_ARCHIVE_MODEL_CASE(177)
OOR_ARCHIVE_MODEL_CASE(178)
OOR_ARCHIVE_MODEL_CASE(179)
OOR_ARCHIVE_MODEL_CASE(180)
OOR_ARCHIVE_MODEL_CASE(181)
OOR_ARCHIVE_MODEL_CASE(182)
OOR_ARCHIVE_MODEL_CASE(183)
OOR_ARCHIVE_MODEL_CASE(184)
OOR_ARCHIVE_MODEL_CASE(185)
OOR_ARCHIVE_MODEL_CASE(186)
OOR_ARCHIVE_MODEL_CASE(187)
OOR_ARCHIVE_MODEL_CASE(188)
OOR_ARCHIVE_MODEL_CASE(189)
OOR_ARCHIVE_MODEL_CASE(190)
OOR_ARCHIVE_MODEL_CASE(191)
OOR_ARCHIVE_MODEL_CASE(192)
OOR_ARCHIVE_MODEL_CASE(193)
OOR_ARCHIVE_MODEL_CASE(194)
OOR_ARCHIVE_MODEL_CASE(195)
OOR_ARCHIVE_MODEL_CASE(196)
OOR_ARCHIVE_MODEL_CASE(197)
OOR_ARCHIVE_MODEL_CASE(198)
OOR_ARCHIVE_MODEL_CASE(199)
OOR_ARCHIVE_MODEL_CASE(200)
OOR_ARCHIVE_MODEL_CASE(201)
OOR_ARCHIVE_MODEL_CASE(202)
OOR_ARCHIVE_MODEL_CASE(203)
OOR_ARCHIVE_MODEL_CASE(204)
OOR_ARCHIVE_MODEL_CASE(205)
OOR_ARCHIVE_MODEL_CASE(206)
OOR_ARCHIVE_MODEL_CASE(207)
OOR_ARCHIVE_MODEL_CASE(208)
OOR_ARCHIVE_MODEL_CASE(209)
OOR_ARCHIVE_MODEL_CASE(210)
OOR_ARCHIVE_MODEL_CASE(211)
OOR_ARCHIVE_MODEL_CASE(212)
OOR_ARCHIVE_MODEL_CASE(213)
OOR_ARCHIVE_MODEL_CASE(214)
OOR_ARCHIVE_MODEL_CASE(215)
OOR_ARCHIVE_MODEL_CASE(216)
OOR_ARCHIVE_MODEL_CASE(217)
OOR_ARCHIVE_MODEL_CASE(218)
OOR_ARCHIVE_MODEL_CASE(219)
OOR_ARCHIVE_MODEL_CASE(220)
OOR_ARCHIVE_MODEL_CASE(221)
OOR_ARCHIVE_MODEL_CASE(222)
OOR_ARCHIVE_MODEL_CASE(223)
OOR_ARCHIVE_MODEL_CASE(224)
OOR_ARCHIVE_MODEL_CASE(225)
OOR_ARCHIVE_MODEL_CASE(226)
OOR_ARCHIVE_MODEL_CASE(227)
OOR_ARCHIVE_MODEL_CASE(228)
OOR_ARCHIVE_MODEL_CASE(229)
OOR_ARCHIVE_MODEL_CASE(230)
OOR_ARCHIVE_MODEL_CASE(231)
OOR_ARCHIVE_MODEL_CASE(232)
OOR_ARCHIVE_MODEL_CASE(233)
OOR_ARCHIVE_MODEL_CASE(234)
OOR_ARCHIVE_MODEL_CASE(235)
OOR_ARCHIVE_MODEL_CASE(236)
OOR_ARCHIVE_MODEL_CASE(237)
OOR_ARCHIVE_MODEL_CASE(238)
OOR_ARCHIVE_MODEL_CASE(239)
OOR_ARCHIVE_MODEL_CASE(240)
OOR_ARCHIVE_MODEL_CASE(241)
OOR_ARCHIVE_MODEL_CASE(242)
OOR_ARCHIVE_MODEL_CASE(243)
OOR_ARCHIVE_MODEL_CASE(244)
OOR_ARCHIVE_MODEL_CASE(245)
OOR_ARCHIVE_MODEL_CASE(246)
OOR_ARCHIVE_MODEL_CASE(247)
OOR_ARCHIVE_MODEL_CASE(248)
OOR_ARCHIVE_MODEL_CASE(249)
OOR_ARCHIVE_MODEL_CASE(250)
OOR_ARCHIVE_MODEL_CASE(251)
OOR_ARCHIVE_MODEL_CASE(252)
OOR_ARCHIVE_MODEL_CASE(253)
OOR_ARCHIVE_MODEL_CASE(254)
OOR_ARCHIVE_MODEL_CASE(255)

#undef OOR_ARCHIVE_MODEL_CASE
