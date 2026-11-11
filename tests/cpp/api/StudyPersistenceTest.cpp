#include "oor/api/Deck.hpp"
#include "oor/api/Results.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/results/core/ResultLayout.hpp"
#include "support/TestFiles.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>

using namespace oor;

TEST_CASE("application documents follow model copies and archives without changing analysis versions", "[api][archive]")
{
    auto deck = api::Deck::createModel();
    const auto content = deck->model()->contentVersion();
    const auto structure = deck->model()->structureVersion();
    deck->setDocument("oos.optimization.study", "{\"version\":1,\"name\":\"study\"}");
    REQUIRE(deck->model()->contentVersion() == content);
    REQUIRE(deck->model()->structureVersion() == structure);
    auto copy = deck->copy();
    copy->setDocument("oos.optimization.study", "copy");
    REQUIRE(deck->document("oos.optimization.study") != copy->document("oos.optimization.study"));
    REQUIRE_FALSE(deck->document("absent"));
    REQUIRE_THROWS_AS(deck->setDocument("", "invalid"), base::UsageError);
    const testing::TestFiles files;
    auto part = deck->addPart({"auxiliary", 8, 0, false});
    part->setDocument("basis", "a shape basis");
    deck->writeArchive(base::pathToUtf8(files.root() / "study.ooh5"));
    auto loaded = api::Deck::readModel(base::pathToUtf8(files.root() / "study.ooh5"));
    REQUIRE(loaded->document("oos.optimization.study") == deck->document("oos.optimization.study"));
    REQUIRE(loaded->part("auxiliary", 8)->document("basis") == "a shape basis");
    REQUIRE(copy->removeDocument("oos.optimization.study"));
    REQUIRE_FALSE(copy->removeDocument("oos.optimization.study"));
    REQUIRE(deck->documentNames().size() == 1);
    REQUIRE(deck->writeModel(base::pathToUtf8(files.root() / "study.oos")).size() == 2);
}

TEST_CASE("named aerodynamic arrays are validated and preserved with an optimization model", "[api][archive]")
{
    auto set = api::Results::fromTables("study", {});
    const std::array<std::int32_t, 1> integer{1};
    const std::array<std::int32_t, 1> fixed{3};
    const std::array<double, 1> value{0.125};
    const std::vector<results::NamedArrayInput> arrays{
        {"variableType", results::ArrayType::Int32, {1}, std::as_bytes(std::span(integer))},
        {"statusCode", results::ArrayType::Int32, {1}, std::as_bytes(std::span(fixed))},
        {"isFixed", results::ArrayType::Int32, {1}, std::as_bytes(std::span(integer))},
        {"value", results::ArrayType::Float64, {1}, std::as_bytes(std::span(value))}};
    results::NamedResultOrigin origin;
    origin.analysisCode = 144;
    origin.subcase = 7;
    origin.attributes = {{"mach", "0.2"}};
    set->addNamed("aeroTrimVariables", origin, arrays, {{"variable", {"AngleOfAttack"}}});
    REQUIRE(set->metadata().namedCount == 1);
    REQUIRE_THROWS_AS(set->addNamed("aeroTrimVariables", origin, {}, {}), base::UsageError);
    REQUIRE(set->named().size() == 1);
    REQUIRE_THROWS_AS(set->readNamed(1), base::UsageError);
    auto deck = api::Deck::createModel();
    deck->setDocument("oos.optimization.study", "{\"version\":1}");
    set->attach(deck);
    const testing::TestFiles files;
    set->writeArchive(base::pathToUtf8(files.root() / "accepted.ooh5"));
    auto loaded = api::Results::open(base::pathToUtf8(files.root() / "accepted.ooh5"));
    REQUIRE(loaded->metadata().namedCount == 1);
    REQUIRE(loaded->named().front().analysisCode == 144);
    REQUIRE(loaded->named().front().subcase == 7);
    REQUIRE(loaded->readNamed(0)->find("value")->values<double>().front() == 0.125);
    REQUIRE(loaded->model()->document("oos.optimization.study") == "{\"version\":1}");
}

TEST_CASE("package solution codes identify static modal buckling and aeroelastic results", "[api][results]")
{
    REQUIRE(results::analysisTypeName(101) == "static");
    REQUIRE(results::analysisTypeName(103) == "modes");
    REQUIRE(results::analysisTypeName(105) == "buckling");
    REQUIRE(results::analysisTypeName(144) == "aeroelastic");
    REQUIRE(results::stepKindOf(103) == results::StepKind::Mode);
    REQUIRE(results::stepKindOf(105) == results::StepKind::Mode);
    REQUIRE(results::analysisTypeName(1) == "static");
}
