#include "oor/api/CardEdits.hpp"
#include "oor/api/ControlEdits.hpp"
#include "oor/api/Deck.hpp"
#include "oor/api/Queries.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/model/core/ModelFileWriter.hpp"
#include "support/TestFiles.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <limits>

using namespace oor;

namespace
{
model::FieldValue integer(std::int64_t value)
{
    return {model::FieldValue::Kind::Integer, value, 0, {}};
}
model::FieldValue real(double value)
{
    return {model::FieldValue::Kind::Real, 0, value, {}};
}
api::CardInput node(int id, double x)
{
    return {"Node", {integer(id), {}, real(x), real(0), real(0)}, {}};
}
api::ControlRecord command(std::string name, std::string value)
{
    return {std::move(name), {std::move(value)}, {}};
}
} // namespace

TEST_CASE("an empty model is constructed without a file and checks forward references explicitly", "[api][builder]")
{
    const auto deck = api::Deck::createModel();
    CHECK(deck->cardCount() == 0);
    api::setSolution(*deck, "LinearStatics");
    const auto global = std::as_const(*deck->model()).control().caseControl.global().key;
    const std::array commands{command("TitleText", "A model; from memory"), command("DisplacementOutput", "All")};
    api::setCaseCommands(*deck, global, commands);
    const std::array subcase{command("StaticLoadSelection", "10")};
    api::addCaseSection(*deck, "subcase", 1, 0, subcase);
    CHECK_FALSE(deck->checkReferences().empty());
    const std::array cards{
        node(1, 2.5),
        api::CardInput{"PointForceLoad", {integer(10), integer(1), {}, real(5), real(1), real(0), real(0)}, {}}};
    api::insertCards(*deck, cards);
    CHECK(deck->checkReferences().empty());
    CHECK(api::findCard(*deck, "nodes", 1)->values.size() > 0);
    CHECK(deck->toModelText().find("TitleText;\"A model; from memory\"") != std::string::npos);
}

TEST_CASE("column construction is atomic and card keys survive renumbering, refusals and compaction", "[api][builder]")
{
    const auto deck = api::Deck::createModel();
    const auto empty = deck->toModelText();
    CHECK_THROWS(api::insertCards(*deck, std::array{node(1, 0), node(1, 1)}));
    CHECK(deck->toModelText() == empty);
    const auto added = api::insertCardBatch(
        *deck, "Node", 2000, [](std::size_t row) { return node(static_cast<int>(row + 1), row * 0.25); });
    REQUIRE(added.size() == 2000);
    const auto key = added[0].key;
    api::updateCardByKey(*deck, "Node", key, node(3000, -3.5));
    CHECK(api::findCardByKey(*deck, "Node", key)->id == 3000);
    const auto before = deck->toModelText();
    CHECK_THROWS(api::updateCardByKey(*deck, "Node", key, node(2, 9)));
    CHECK(deck->toModelText() == before);
    // A refused replacement must not append an out-of-order key to the row book.
    CHECK(api::findCardByKey(*deck, "Node", added.back().key)->id == 2000);
    CHECK_THROWS(api::insertCardBatch(
        *deck, "Node", 3, [](std::size_t row) { return node(row == 2 ? 2 : static_cast<int>(4000 + row), 0); }));
    CHECK(deck->toModelText() == before);
    CHECK_FALSE(api::findCard(*deck, "nodes", 4000));
    CHECK(api::removeCardByKey(*deck, "Node", added[10].key));
    deck->model()->compact();
    CHECK_FALSE(api::findCardByKey(*deck, "Node", added[10].key));
    CHECK(api::findCardByKey(*deck, "Node", key)->id == 3000);
    CHECK(api::findCardByKey(*deck, "Node", added.back().key)->id == 2000);
    auto bad = node(5000, std::numeric_limits<double>::infinity());
    CHECK_THROWS(api::insertCards(*deck, std::array{bad}));
}

TEST_CASE("set cards with the same ID are edited independently by key", "[api][builder]")
{
    const auto deck = api::Deck::createModel();
    const std::array cards{api::CardInput{"SinglePointConstrList", {integer(7), integer(123), integer(1)}, "first"},
                           api::CardInput{"SinglePointConstrList", {integer(7), integer(456), integer(2)}, "second"}};
    const auto added = api::insertCards(*deck, cards);
    REQUIRE(added.size() == 2);
    api::setCardValueByKey(*deck, added[1].type, added[1].key, "components", "12");
    CHECK(api::findCardByKey(*deck, added[0].type, added[0].key)->fields[1] == "123");
    CHECK(api::findCardByKey(*deck, added[1].type, added[1].key)->fields[1] == "12");
    CHECK(api::findCardByKey(*deck, added[1].type, added[1].key)->comment == "second");
    api::setCardCommentByKey(*deck, added[1].type, added[1].key, "changed");
    CHECK(api::findCardByKey(*deck, added[1].type, added[1].key)->comment == "changed");
}

TEST_CASE("case editing preserves inheritance, output variants, identity and atomic failure",
          "[api][builder][controls]")
{
    const auto deck = api::Deck::createModel();
    const auto global = std::as_const(*deck->model()).control().caseControl.global().key;
    api::setCaseCommands(*deck, global,
                         std::array{command("SinglePointConstrSelection", "7"), command("DisplacementOutput", "All")});
    const auto first = api::addCaseSection(*deck, "subcase", 1, 0, std::array{command("StaticLoadSelection", "10")});
    const auto second = api::addCaseSection(*deck, "subcase", 2, 0);
    const auto step = api::addCaseSection(*deck, "step", 1, first);
    const auto& control = std::as_const(*deck->model()).control().caseControl;
    CHECK(control.effectiveCommand(step, "SinglePointConstrSelection")->value.integer == 7);
    CHECK(control.effectiveCommand(step, "StaticLoadSelection")->value.integer == 10);
    const auto before = deck->toModelText();
    CHECK_THROWS(api::setCaseCommands(*deck, first,
                                      std::array{command("StaticLoadSelection", "20"), command("NotACommand", "1")}));
    CHECK(deck->toModelText() == before);
    CHECK_THROWS(api::addCaseSection(*deck, "subcase", 1, 0));
    CHECK_THROWS(api::addCaseSection(*deck, "step", 2, global));
    CHECK_THROWS(api::renumberCaseSection(*deck, second, 1));
    const auto keys =
        api::setCaseCommands(*deck, first,
                             std::array{api::ControlRecord{"DisplacementOutput", {"3", "PrintListing"}, {}},
                                        api::ControlRecord{"SetDefinition", {"3", "1", "Through", "5"}, {}},
                                        api::ControlRecord{"SubcaseParameter", {"PostprocessingFile", "-1"}, {}}});
    CHECK(keys.size() == 3);
    CHECK(control.effectiveOutputs(step, "DisplacementOutput").size() == 1);
    CHECK(control.effectiveOutputs(step, "DisplacementOutput")[0]->value.integer == 3);
    CHECK(api::removeCaseEntry(*deck, keys[0]));
    CHECK(control.effectiveOutputs(step, "DisplacementOutput")[0]->value.text == "All");
    api::renumberCaseSection(*deck, first, 4);
    CHECK(api::caseSection(*deck, first).id == 4);
    CHECK(api::removeCaseSection(*deck, first));
    CHECK_THROWS(api::caseSection(*deck, step));
}

TEST_CASE("executive replacement validates before changing the model", "[api][builder][controls]")
{
    const auto deck = api::Deck::createModel();
    api::setExecutive(*deck, std::array{command("SolutionSequence", "LinearStatics"),
                                        api::ControlRecord{"FileAssignment", {"Unit=7", "Path=external.data"}, {}}});
    REQUIRE(api::executiveRecords(*deck).size() == 2);
    const auto before = deck->toModelText();
    CHECK_THROWS(api::setExecutive(*deck, std::array{command("UnknownStatement", "1")}));
    CHECK(deck->toModelText() == before);
}

TEST_CASE("constructed includes and parts survive both file formats and part ownership", "[api][builder]")
{
    const testing::TestFiles files;
    auto deck = api::Deck::createModel();
    api::appendInclude(*deck, "mesh/nodes.oos", std::array{node(1, 1), node(2, 2)});
    auto part = deck->addPart({"SuperelementPart", 5, 0, false});
    api::insertCards(*part, std::array{node(1, 99)});
    CHECK_THROWS(deck->addPart({"SuperelementPart", 5, 0, false}));
    const auto output = files.root() / "model.oos";
    CHECK(deck->writeModel(output.string(), {true, {}}).empty());
    CHECK(std::filesystem::exists(files.root() / "mesh/nodes.oos"));
    auto read = api::Deck::readModel(output.string(), {true});
    CHECK(read->toModelText() == deck->toModelText());
    const auto archive = files.root() / "model.ooh5";
    CHECK(deck->writeArchive(archive.string()).finished);
    auto restored = api::Deck::readModel(archive.string());
    CHECK(restored->toModelText() == deck->toModelText());
    CHECK(restored->part("SuperelementPart", 5)->cardCount() == 1);
    deck.reset();
    CHECK(part->cardCount() == 1);
    CHECK(api::findCard(*part, "nodes", 1)->fields[2] == "99.");
}

TEST_CASE("singular insertion and model copies retain their identity and independence", "[api][builder]")
{
    const auto deck = api::Deck::createModel();
    const api::CardInput points{"ScalarPoint", {integer(1), integer(2)}, {}};
    const auto empty = deck->toModelText();
    CHECK_THROWS(api::insertCard(*deck, points));
    CHECK(deck->toModelText() == empty);
    const auto added = api::insertCard(*deck, node(1, 1));
    const auto global = std::as_const(*deck->model()).control().caseControl.global().key;
    api::setCaseCommands(*deck, global, std::array{command("TitleText", "Original")});
    const auto copy = deck->copy();
    api::setCardValuesByKey(*copy, added.type, added.key, std::array{std::pair{std::string("position1"), real(2)}});
    api::setCaseCommands(*copy, global, std::array{command("TitleText", "Copy")});
    CHECK(api::findCardByKey(*deck, added.type, added.key)->fields[2] == "1.");
    CHECK(api::findCardByKey(*copy, added.type, added.key)->fields[2] == "2.");
    CHECK(api::caseSection(*deck, global).entries[0].value.text == "Original");
    CHECK(api::caseSection(*copy, global).entries[0].value.text == "Copy");
}

namespace
{
void exerciseStoredCards(const model::Model& source)
{
    for (std::uint16_t type = 1; type < model::Model::typeCount(); ++type)
    {
        const auto* table = source.findTable(type);
        if (!table)
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            INFO(table->info().name << " row " << row);
            const auto record = model::ModelFileWriter::cardToRecord(source, {type, row});
            const auto deck = api::Deck::createModel();
            const auto added = api::addCards(*deck, record);
            const auto before = deck->toModelText();
            for (const auto& card : added)
            {
                const auto view = api::findCardByKey(*deck, card.type, card.key);
                REQUIRE(view);
                api::CardInput replacement;
                replacement.type = card.type;
                for (const auto& field : view->fields)
                    replacement.fields.push_back({model::FieldValue::Kind::Text, 0, 0, field});
                api::updateCardByKey(*deck, card.type, card.key, replacement);
            }
            CHECK(deck->toModelText() == before);
        }
    }
    for (const auto& part : source.parts())
        exerciseStoredCards(*part);
}

void exerciseValidationBatch(unsigned batch)
{
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(OOR_VALIDATION_DIR))
        if (entry.is_regular_file() && entry.path().extension() == ".oos")
            files.push_back(entry.path());
    std::sort(files.begin(), files.end());
    for (std::size_t i = batch; i < files.size(); i += 8)
    {
        INFO(files[i].filename().string());
        const auto deck = api::Deck::readModel(files[i].string());
        exerciseStoredCards(*deck->model());
    }
}
} // namespace

#define MODEL_EDITING_BATCH(index)                                                                         \
    TEST_CASE("structured editing preserves validation cards batch " #index, "[api][builder][validation]") \
    {                                                                                                      \
        exerciseValidationBatch(index);                                                                    \
    }
MODEL_EDITING_BATCH(0)
MODEL_EDITING_BATCH(1)
MODEL_EDITING_BATCH(2)
MODEL_EDITING_BATCH(3)
MODEL_EDITING_BATCH(4)
MODEL_EDITING_BATCH(5)
MODEL_EDITING_BATCH(6)
MODEL_EDITING_BATCH(7)
#undef MODEL_EDITING_BATCH
