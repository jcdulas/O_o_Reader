#include "oor/api/CardEdits.hpp"
#include "oor/api/Deck.hpp"
#include "oor/api/Queries.hpp"
#include "oor/api/Results.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/io/hdf5/Hdf5Tree.hpp"
#include "oor/model/DataContract.hpp"
#include "oor/query/table/Hdf5Export.hpp"
#include "oor/query/table/PlainColumn.hpp"
#include "oor/results/core/MemoryResults.hpp"
#include "support/TestFiles.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cmath>
#include <limits>
#include <span>

using namespace oor;

namespace
{

// A plate of two ShellQuad4 on six nodes, written for these tests (a model file).
const char* const PLATE = R"([executive]
SolutionSequence;101
[case]
SubcaseStart;1
DisplacementOutput;All
[model]
Node;1;;0.;0.;0.
Node;2;;1.;0.;0.
Node;3;;2.;0.;0.
Node;4;;0.;1.;0.
Node;5;;1.;1.;0.
Node;6;;2.;1.;0.
ShellQuad4;10;1;1;2;5;4
ShellQuad4;11;1;2;3;6;5
ShellProp;1;1;0.1
IsoMat;1;7.E+4;;0.33;2.7E-9
)";

// The displacements of the six grid points, made in memory as a solver hands them over.
std::shared_ptr<api::Results> plateDisplacements()
{
    std::vector<double> ramp(6 * 6);
    for (std::size_t index = 0; index < ramp.size(); ++index)
        ramp[index] = 0.001 * static_cast<double>(index);
    static std::vector<std::vector<double>> kept; // the input borrows its values until the table is made
    kept.push_back(std::move(ramp));
    results::MemoryTableInput input;
    input.kind = results::ResultKind::Displacements;
    input.subcase = 1;
    input.steps = {std::numeric_limits<double>::quiet_NaN()};
    input.ids = {1, 2, 3, 4, 5, 6};
    input.values = std::as_bytes(std::span<const double>(kept.back()));
    std::vector<results::MemoryTableInput> inputs;
    inputs.push_back(std::move(input));
    return api::Results::fromTables("plate", std::move(inputs));
}

} // namespace

TEST_CASE("a model is read, counted, queried and written back", "[api]")
{
    const testing::TestFiles files;
    const auto path = files.write("plate.oos", PLATE);
    const auto deck = api::Deck::readModel(path.string());
    CHECK(deck->diagnostics().empty());
    const auto counts = deck->cardCounts();
    const auto countOf = [&](const std::string& name)
    {
        for (const auto& [card, count] : counts)
            if (card == name)
                return count;
        return std::size_t{0};
    };
    CHECK(countOf("Node") == 6);
    CHECK(countOf("ShellQuad4") == 2); // the neutral name of the card type

    const auto nodes = api::runQuery("nodesTable", deck, nullptr, {});
    CHECK(nodes->rowCount() == 6);
    const query::PlainColumn x = query::plainColumn(*nodes, *nodes->find("x"));
    CHECK(x.numbers[2] == 2.0);

    query::QueryArguments arguments;
    arguments.set("columns", std::vector<std::string>{"elementId", "type", "nodes"});
    const auto elements = api::runQuery("elementsTable", deck, nullptr, arguments);
    CHECK(elements->columnNames() == std::vector<std::string>{"elementId", "type", "nodes"});
    const query::PlainColumn connectivity = query::plainColumn(*elements, 2);
    CHECK(connectivity.kind == query::PlainKind::NumberList);
    CHECK(connectivity.offsets == std::vector<std::int64_t>{0, 4, 8});
    CHECK(connectivity.numbers[6] == 6.0);

    const std::string text = deck->toModelText();
    CHECK(text.find("Node;1;") != std::string::npos);
    CHECK(deck->writeModel((files.root() / "copy.oos").string()).empty());
    CHECK(api::Deck::readModel((files.root() / "copy.oos").string())->cardCount() == deck->cardCount());
}

TEST_CASE("a model is written and read as records regardless of the path extension", "[api]")
{
    const testing::TestFiles files;
    const auto deck = api::Deck::readModel(base::pathToUtf8(files.write("plate.oos", PLATE)));
    REQUIRE(deck->cardCount() == 10);
    const std::string text = deck->toModelText();
    CHECK_THAT(text, Catch::Matchers::ContainsSubstring("ShellQuad4;10;1;1;2;5;4\n"));
    const std::string modelPath = base::pathToUtf8(files.root() / "copy.oos");
    CHECK(deck->writeModel(modelPath).empty());
    CHECK(files.read("copy.oos") == text);
    const auto model = api::Deck::readModel(modelPath);
    CHECK(model->diagnostics().empty());
    CHECK(model->cardCounts() == deck->cardCounts());
    CHECK(model->toModelText() == text);
    CHECK(model->writeModel(base::pathToUtf8(files.root() / "again.txt")).empty());
    CHECK(files.read("again.txt") == text);
    CHECK(api::Deck::readModel(base::pathToUtf8(files.root() / "again.txt"))->toModelText() == text);
}

TEST_CASE("results are listed, read, attached to a deck and converted", "[api]")
{
    const testing::TestFiles files;
    const auto deck = api::Deck::readModel(files.write("plate.oos", PLATE).string());
    const auto results = plateDisplacements();
    CHECK(results->format() == "memory");
    const auto tables = results->tables();
    REQUIRE(tables.size() == 1);
    CHECK(tables.front().kind == "displacements");
    CHECK(tables.front().analysis == "static");
    CHECK(tables.front().rowCount == 6);

    api::TableSelection selection;
    selection.components = {"t2"};
    selection.ids = {5};
    const api::TableValues values = results->read(0, selection);
    CHECK(values.shape == std::vector<std::size_t>{1, 1, 1});
    CHECK(std::abs(values.data->values<double>()[0] - 0.025) < 1e-9);
    CHECK_THROWS(results->read(7));

    results->attach(deck);
    const auto maximum = api::runQuery("maxDisplacement", deck, results, {});
    CHECK(maximum->rowCount() >= 1);

    const auto archive = files.root() / "plate.oorh5";
    CHECK(results->writeArchive(archive.string()).finished);
    const auto reopened = api::Results::open(archive.string());
    CHECK(reopened->format() == "archive");
    CHECK(reopened->tables().size() == 1);
    CHECK(reopened->hasModel());
    CHECK(reopened->model()->toModelText() == deck->toModelText());
    CHECK(api::Deck::readModel(archive.string())->toModelText() == deck->toModelText());
    CHECK(api::runQuery("maxDisplacement", reopened->model(), reopened, {})->rowCount() >= 1);
    // Rewriting an archive before requesting its model must preserve that model too.
    const auto copy = files.root() / "copy.oorh5";
    CHECK(api::Results::open(archive.string())->writeArchive(copy.string()).finished);
    CHECK(api::Deck::readModel(copy.string())->toModelText() == deck->toModelText());

    const auto workbook = files.root() / "plate.xlsx";
    const results::ResultWriteReport report = results->writeXlsx(workbook.string());
    CHECK(report.finished);
    CHECK(std::filesystem::file_size(workbook) == report.bytes);
}

TEST_CASE("a native archive holds a model alone or results alone", "[api][archive-model]")
{
    const testing::TestFiles files;
    const auto deck = api::Deck::readModel(files.write("plate.oos", PLATE).string());
    const auto modelOnly = files.root() / "model.oorh5";
    CHECK(deck->writeArchive(modelOnly.string()).finished);
    const auto archive = api::Results::open(modelOnly.string());
    CHECK(archive->tables().empty());
    CHECK(archive->hasModel());
    CHECK(archive->model()->toModelText() == deck->toModelText());
    const auto resultsOnly = files.root() / "results.oorh5";
    CHECK(plateDisplacements()->writeArchive(resultsOnly.string()).finished);
    const auto results = api::Results::open(resultsOnly.string());
    CHECK_FALSE(results->hasModel());
    CHECK(results->model() == nullptr);
    CHECK_THROWS(api::Deck::readModel(resultsOnly.string()));
}

TEST_CASE("query tables are written to HDF5 as custom outputs", "[api]")
{
    const testing::TestFiles files;
    const auto deck = api::Deck::readModel(files.write("plate.oos", PLATE).string());
    const auto nodes = api::runQuery("nodesTable", deck, nullptr, {});
    const auto elements = api::runQuery("elementsTable", deck, nullptr, {});
    const auto path = files.root() / "outputs.h5";
    query::writeQueryTables(path, {{"nodes", nodes}, {"elements", elements}});
    const io::Hdf5Tree tree = io::Hdf5Tree::read(path);
    CHECK(tree.find("/tables/nodes/columnNames") != nullptr);
    const io::Hdf5Node* x = tree.find("/tables/nodes/columns/x");
    REQUIRE(x != nullptr);
    CHECK(x->shape == std::vector<std::uint64_t>{6});
    CHECK(tree.find("/tables/elements/columns/nodes.offsets") != nullptr);
    CHECK_THROWS(query::writeQueryTables(path, {{"bad name", nodes}}));
    CHECK(api::toCsv(*nodes).rfind("nodeId,", 0) == 0);
}

TEST_CASE("the query catalogue describes every query and its parameters", "[api]")
{
    const auto catalogue = api::queryCatalogue();
    CHECK(catalogue.size() > 100);
    CHECK(api::parameterKind("nodesTable", "nodeIds") == query::ParameterKind::IdList);
    CHECK(api::parameterKind("nodesTable", "limit") == query::ParameterKind::Integer);
    CHECK_THROWS_AS(api::parameterKind("nodesTable", "nonsense"), query::QueryError);
    CHECK_THROWS_AS(api::runQuery("no_such_query", nullptr, nullptr, {}), query::QueryError);
}

TEST_CASE("query arguments are read from JSON by the kind of each parameter", "[api]")
{
    const query::QueryArguments arguments =
        api::argumentsFromJson("nodesTable", R"({"nodeIds": [1, 3], "columns": ["nodeId", "x"], "limit": 1})");
    CHECK(arguments.ids("nodeIds") == std::vector<std::int64_t>{1, 3});
    CHECK(arguments.texts("columns") == std::vector<std::string>{"nodeId", "x"});
    CHECK(arguments.integerOr("limit", 0) == 1);
    CHECK(api::argumentsFromJson("nodesTable", R"({"nodeIds": 5})").ids("nodeIds") == std::vector<std::int64_t>{5});
    CHECK_FALSE(api::argumentsFromJson("nodesTable", R"({"nodeIds": null})").has("nodeIds"));
    CHECK(api::argumentsFromJson("nodesTable", "").values().empty());
    CHECK_THROWS_AS(api::argumentsFromJson("nodesTable", R"({"limit": 1.5})"), base::UsageError);
    CHECK_THROWS_AS(api::argumentsFromJson("nodesTable", R"({"limit": "two"})"), base::UsageError);
    CHECK_THROWS_AS(api::argumentsFromJson("nodesTable", R"({"nonsense": 1})"), query::QueryError);
    CHECK_THROWS_AS(api::argumentsFromJson("nodesTable", R"({"limit": 1)"), base::UsageError);

    const base::JsonValue value = base::JsonValue::parse(R"({"a": [1, 2.5e1, true, null, "x\"y"], "b": {}})");
    const auto& items = value.member("a")->asArray("a");
    CHECK(items[1].asNumber("a[1]") == 25.0);
    CHECK(items[2].asBool("a[2]"));
    CHECK(items[3].isNull());
    CHECK(items[4].asText("a[4]") == "x\"y");
    CHECK(value.member("b")->asObject("b").empty());
}

namespace
{

// The value of a named field of a card view.
const api::FieldValue& valueOf(const api::CardView& card, const std::string& name)
{
    for (const auto& [key, value] : card.values)
        if (key == name)
            return value;
    FAIL("the card has no value " + name);
    static const api::FieldValue NONE;
    return NONE;
}

} // namespace

TEST_CASE("a card is found by ID, with its named values, fields, text and comment", "[api][cards]")
{
    const testing::TestFiles files;
    const std::string modelText =
        std::string(PLATE).replace(std::string(PLATE).find("Node;3;"), 0, "# the corner of the plate\n");
    const auto deck = api::Deck::readModel(files.write("plate.oos", modelText).string());

    const auto node = api::findCard(*deck, "nodes", 3, {"xyzGlobal"});
    REQUIRE(node);
    CHECK(node->type == "Node");
    CHECK(node->id == 3);
    CHECK(valueOf(*node, "xyz").numbers == std::vector<double>{2.0, 0.0, 0.0});
    CHECK(valueOf(*node, "xyzGlobal").numbers == std::vector<double>{2.0, 0.0, 0.0});
    CHECK(valueOf(*node, "positionFrameId").integer);
    CHECK(node->comment == " the corner of the plate");
    CHECK(node->fields.front() == "3");
    CHECK(node->text.rfind("Node;", 0) == 0);

    const auto quad = api::findCard(*deck, "elements", 11);
    REQUIRE(quad);
    CHECK(quad->type == "ShellQuad4");
    CHECK(valueOf(*quad, "nodes").numbers == std::vector<double>{2, 3, 6, 5});
    CHECK(api::findCard(*deck, "ShellQuad4", 11)->id == 11);
    CHECK(api::findCard(*deck, "ShellProp", 1)->type == "ShellProp");
    CHECK_FALSE(api::findCard(*deck, "elements", 99));
    CHECK_FALSE(api::findCard(*deck, "ShellTria3", 10)); // element 10 is a ShellQuad4
    CHECK_THROWS_AS(api::findCard(*deck, "WIDGETS", 1), query::QueryError);
    CHECK(api::cardIds(*deck, "nodes") == std::vector<base::EntityId>{1, 2, 3, 4, 5, 6});
    CHECK(api::cardIds(*deck, "materials") == std::vector<base::EntityId>{1});
}

TEST_CASE("a card is edited in place, keeps its comment, and a refused edit changes nothing", "[api][cards]")
{
    const testing::TestFiles files;
    std::string modelText = PLATE;
    modelText.insert(modelText.find("Node;3;"), "# the corner of the plate\n");
    const auto deck = api::Deck::readModel(files.write("plate.oos", modelText).string());

    api::setCardField(*deck, "nodes", 3, 3, "2.5"); // compact field 3 is position1
    CHECK(valueOf(*api::findCard(*deck, "nodes", 3), "xyz").numbers == std::vector<double>{2.5, 0.0, 0.0});
    CHECK(api::findCard(*deck, "nodes", 3)->comment == " the corner of the plate");
    // the card keeps its place: still between Node 2 and Node 4 in the model records
    const std::string written = deck->toModelText();
    CHECK(written.find("Node;2;") < written.find("Node;3;"));
    CHECK(written.find("Node;3;") < written.find("Node;4;"));
    CHECK(written.find("2.5") != std::string::npos);

    api::replaceCard(*deck, "ShellProp", 1, "ShellProp;1;1;0.25");
    CHECK(valueOf(*api::findCard(*deck, "ShellProp", 1), "thickness").number == 0.25);

    // refused: a field that is not a number, an ID taken by another card, a card of another type
    CHECK_THROWS_AS(api::setCardField(*deck, "nodes", 3, 3, "abc"), base::UsageError);
    CHECK_THROWS_AS(api::setCardField(*deck, "nodes", 3, 1, "4"), base::DuplicateIdError);
    CHECK_THROWS_AS(api::replaceCard(*deck, "nodes", 3, "IsoMat;9;100.;;0.3"), base::UsageError);
    CHECK(valueOf(*api::findCard(*deck, "nodes", 3), "xyz").numbers == std::vector<double>{2.5, 0.0, 0.0});
    CHECK(api::cardIds(*deck, "nodes").size() == 6);

    // a new ID of its own is accepted
    api::setCardField(*deck, "nodes", 3, 1, "30");
    CHECK_FALSE(api::findCard(*deck, "nodes", 3));
    CHECK(api::findCard(*deck, "nodes", 30)->comment == " the corner of the plate");
}

TEST_CASE("cards are added all or none, and removed", "[api][cards]")
{
    const testing::TestFiles files;
    const auto deck = api::Deck::readModel(files.write("plate.oos", PLATE).string());
    const std::size_t before = deck->cardCount();

    const auto added = api::addCards(*deck, "Node;7;;3.;0.;0.\nNode;8;;3.;1.;0.\nShellQuad4;12;1;3;7;8;6");
    REQUIRE(added.size() == 3);
    CHECK(added[2].type == "ShellQuad4");
    CHECK(added[2].id == 12);
    CHECK(deck->cardCount() == before + 3);
    CHECK(api::findCard(*deck, "elements", 12));

    // all or none: the second card takes an ID that exists, so the first is not added either
    CHECK_THROWS_AS(api::addCards(*deck, "Node;9;;4.;0.;0.\nNode;1;;5.;0.;0."), base::DuplicateIdError);
    CHECK_FALSE(api::findCard(*deck, "nodes", 9));
    CHECK_THROWS_AS(api::addCards(*deck, "Node;9;;4.;0.;0.\nNode;9;;5.;0.;0."), base::DuplicateIdError);
    CHECK_THROWS_AS(api::addCards(*deck, "Widget;1;2"), base::UsageError);
    CHECK_THROWS_AS(api::addCards(*deck, "Node;abc"), base::UsageError);
    CHECK(deck->cardCount() == before + 3);

    CHECK(api::removeCard(*deck, "elements", 12));
    CHECK_FALSE(api::removeCard(*deck, "elements", 12));
    CHECK_FALSE(api::findCard(*deck, "elements", 12));
    CHECK(api::Deck::readModel(files.write("copy.oos", deck->toModelText()).string())->cardCount() == before + 2);
}

TEST_CASE("a field of a card is set by the name its reader gives it", "[api][cards]")
{
    const testing::TestFiles files;
    const auto deck = api::Deck::readModel(files.write("plate.oos", PLATE).string());

    const auto names = api::cardFieldNames(*deck, "ShellProp", 1);
    REQUIRE(names.size() >= 3);
    CHECK(names[0] == std::pair<int, std::string>{1, "id"});
    CHECK(names[2] == std::pair<int, std::string>{3, "thickness"});

    api::setCardValue(*deck, "properties", 1, "thickness", "2");
    CHECK(valueOf(*api::findCard(*deck, "ShellProp", 1), "thickness").number == 2.0);
    api::setCardValue(*deck, "nodes", 3, "position2", "0.5");
    CHECK(valueOf(*api::findCard(*deck, "nodes", 3), "xyz").numbers == std::vector<double>{2.0, 0.5, 0.0});
    api::setCardValue(*deck, "ShellProp", 1, "bendingStiffnessRatio", "1.5");
    CHECK(api::cardFields(*deck, "ShellProp", 1)[4] == "1.5");

    CHECK_THROWS_WITH(api::setCardValue(*deck, "ShellProp", 1, "depth", "1"),
                      Catch::Matchers::ContainsSubstring("its named fields are id, membraneMatId, thickness"));
    CHECK_THROWS_AS(api::setCardValue(*deck, "ShellProp", 1, "thickness", "thick"), base::UsageError);
    CHECK(valueOf(*api::findCard(*deck, "ShellProp", 1), "thickness").number == 2.0);
}

TEST_CASE("card editing uses compact alternatives and includes word fields in its names", "[api][cards]")
{
    const testing::TestFiles files;
    const auto deck = api::Deck::readModel(files.write("plate.oos", PLATE).string());
    api::setCardValue(*deck, "ShellQuad4", 10, "matFrameId", "2");
    const auto fields = api::cardFields(*deck, "ShellQuad4", 10);
    REQUIRE(fields.size() >= 8);
    CHECK(fields[6].empty());
    CHECK(fields[7] == "2");
    CHECK_THROWS_AS(api::setCardValue(*deck, "ShellQuad4", 10, "materialAngle", "15."), base::UsageError);
    CHECK(api::cardFields(*deck, "ShellQuad4", 10) == fields);
    api::setCardValue(*deck, "ShellQuad4", 10, "matFrameId", "");
    api::setCardValue(*deck, "ShellQuad4", 10, "materialAngle", "15.");
    CHECK(api::cardFields(*deck, "ShellQuad4", 10)[6] == "15.");
    (void)api::addCards(*deck,
                        "BarProp;2;1;1.;1.;1.;1.\n"
                        "Bar2;20;2;1;2;0.;;1.;0.\n");
    api::setCardValue(*deck, "Bar2", 20, "offsetKind", "OffsetsBasicGlobalOrient");
    CHECK(valueOf(*api::findCard(*deck, "Bar2", 20), "offsetKind").text == "OffsetsBasicGlobalOrient");
}
