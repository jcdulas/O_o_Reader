#include "oor/base/errors/Errors.hpp"
#include "oor/model/core/CardRegistry.hpp"
#include "oor/model/format/CardLayouts.hpp"
#include "oor/model/format/LayoutSlots.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using Catch::Matchers::ContainsSubstring;

namespace
{

// A small catalogue: a card with alternatives and one with a repeated group.
constexpr std::string_view CATALOGUE = R"({
  "formatVersion": 3,
  "cards": {
    "Shell": {"fields": [
      {"name": "id", "slot": 1, "kind": "integer"},
      {"name": "angle", "slot": 3, "kind": "real", "default": "0.0"},
      {"name": "frameId", "slot": 3, "kind": "integer"},
      {"name": "flag", "slot": 10, "kind": "integer"}]},
    "Laminate": {"fields": [{"name": "id", "slot": 1, "kind": "integer"}],
      "group": {"name": "plies", "firstSlot": 9, "period": 4, "fields": [
        {"name": "matId", "offset": 0, "kind": "integer"},
        {"name": "thickness", "offset": 1, "kind": "real"},
        {"name": "output", "offset": 3, "kind": "text"}]}}
  }
})";

std::string problemOf(const std::string& json)
{
    try
    {
        (void)CardLayouts::fromJson(json);
    }
    catch (const base::UsageError& error)
    {
        return error.what();
    }
    return {};
}

std::string catalogueWith(const std::string& card)
{
    return R"({"formatVersion": 3, "cards": {"Card": )" + card + "}}";
}

} // namespace

TEST_CASE("the standard card catalogue has a layout for every card type the library knows", "[model][layouts]")
{
    const CardLayouts& layouts = *CardLayouts::standard();
    const CardRegistry& registry = CardRegistry::instance();
    std::vector<std::string> missing;
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        if (layouts.find(registry.entry(type).info.name) == nullptr)
            missing.emplace_back(registry.entry(type).info.name);
    }
    CHECK(missing.empty());
    for (const CardLayout& layout : layouts.layouts())
    {
        INFO(layout.card);
        CHECK(registry.typeIndexOf(layout.card).has_value());
    }
}

TEST_CASE("a record's fields go to the slots of its layout, alternatives on one slot", "[model][layouts]")
{
    const auto layouts = CardLayouts::fromJson(CATALOGUE);
    const CardLayout& shell = *layouts->find("Shell");
    std::vector<std::string> slots;
    std::string problem;
    REQUIRE(shell.toSlots(std::vector<std::string>{"7", "", "12", "1"}, slots, problem));
    CHECK(slots == std::vector<std::string>{"7", "", "12", "", "", "", "", "", "", "1"});
    REQUIRE(shell.toSlots(std::vector<std::string>{"7", "30."}, slots, problem));
    CHECK(slots == std::vector<std::string>{"7", "", "30."});
    CHECK_FALSE(shell.toSlots(std::vector<std::string>{"7", "30.", "12"}, slots, problem));
    CHECK_THAT(problem, ContainsSubstring("angle") && ContainsSubstring("frameId"));
    CHECK_FALSE(shell.toSlots(std::vector<std::string>{"7", "", "", "1", "2"}, slots, problem));
    CHECK_THAT(problem, ContainsSubstring("4 fields"));
}

TEST_CASE("the slots a card writer fills become a compact record", "[model][layouts]")
{
    const auto layouts = CardLayouts::fromJson(CATALOGUE);
    const CardLayout& shell = *layouts->find("Shell");
    std::vector<std::string> values;
    std::string problem;
    const std::vector<std::string> slots{"7", "", "12"};
    REQUIRE(shell.toValues(slots, std::vector<SlotKind>{SlotKind::Integer, SlotKind::Blank, SlotKind::Integer}, values,
                           problem));
    CHECK(values == std::vector<std::string>{"7", "", "12"});
    REQUIRE(shell.toValues(std::vector<std::string>{"7", "", "30."},
                           std::vector<SlotKind>{SlotKind::Integer, SlotKind::Blank, SlotKind::Real}, values, problem));
    CHECK(values == std::vector<std::string>{"7", "30."});
    CHECK_FALSE(shell.toValues(std::vector<std::string>{"7", "x"}, {}, values, problem));
    CHECK_THAT(problem, ContainsSubstring("slot 2"));
}

TEST_CASE("a repeated group comes after the fixed fields, its padding dropped", "[model][layouts]")
{
    const auto layouts = CardLayouts::fromJson(CATALOGUE);
    const CardLayout& laminate = *layouts->find("Laminate");
    std::vector<std::string> slots;
    std::string problem;
    REQUIRE(laminate.toSlots(std::vector<std::string>{"5", "1", "0.1", "YES", "2", "0.2"}, slots, problem));
    std::vector<std::string> expected(14);
    expected[0] = "5";
    expected[8] = "1";
    expected[9] = "0.1";
    expected[11] = "YES";
    expected[12] = "2";
    expected[13] = "0.2";
    CHECK(slots == expected);
    std::vector<std::string> values;
    REQUIRE(laminate.toValues(slots, {}, values, problem));
    CHECK(values == std::vector<std::string>{"5", "1", "0.1", "YES", "2", "0.2"});
}

TEST_CASE("a catalogue that does not hold together is refused with the card and the problem", "[model][layouts]")
{
    CHECK_THAT(problemOf(R"({"formatVersion": 2, "cards": {}})"), ContainsSubstring("formatVersion"));
    CHECK_THAT(problemOf(catalogueWith(R"({"fields": [{"name": "id", "slot": 0, "kind": "integer"}]})")),
               ContainsSubstring("card Card, field id: the slot is below 1"));
    CHECK_THAT(problemOf(catalogueWith(R"({"fields": [{"name": "id", "slot": 1, "kind": "word"}]})")),
               ContainsSubstring("unknown kind"));
    CHECK_THAT(problemOf(catalogueWith(R"({"fields": [{"name": "a", "slot": 1, "kind": "real"},
                                                     {"name": "a", "slot": 2, "kind": "real"}]})")),
               ContainsSubstring("used twice"));
    CHECK_THAT(problemOf(catalogueWith(R"({"fields": [{"name": "a", "slot": 1, "kind": "real"},
                                                     {"name": "b", "slot": 1, "kind": "real"}]})")),
               ContainsSubstring("do not tell them apart"));
    CHECK_THAT(problemOf(catalogueWith(R"({"fields": [{"name": "a", "slot": 9, "kind": "real"}],
        "group": {"name": "g", "firstSlot": 9, "period": 2, "fields": [{"name": "b", "offset": 0, "kind": "real"}]}})")),
               ContainsSubstring("in the slots of the group"));
    CHECK_THAT(problemOf(catalogueWith(R"({"fields": [],
        "group": {"name": "g", "firstSlot": 9, "period": 2, "fields": [{"name": "b", "offset": 2, "kind": "real"}]}})")),
               ContainsSubstring("not inside the period"));
    CHECK_THAT(problemOf(catalogueWith(R"({"group": {}})")), ContainsSubstring("\"fields\" is missing"));
}

namespace
{

// Cards with repetitions that do not fill their lines, and cards made of keyword lines.
constexpr std::string_view SECTION_CATALOGUE = R"({
  "formatVersion": 3,
  "cards": {
    "Pairs": {"fields": [{"name": "id", "slot": 1, "kind": "integer"}],
      "group": {"name": "pairs", "firstSlot": 10, "period": 2, "perLine": 3, "fields": [
        {"name": "node", "offset": 0, "kind": "integer"},
        {"name": "value", "offset": 1, "kind": "real"}]}},
    "Steps": {"fields": [{"name": "id", "slot": 1, "kind": "integer"}],
      "group": {"name": "items", "firstSlot": 9, "period": 2, "fields": [
        {"name": "itemId", "offset": 0, "kind": "integer"},
        {"name": "repeats", "offset": 1, "kind": "real"}]},
      "sections": {"name": "trailer", "keyOffset": 0, "kinds": [
        {"key": "UNITS", "fields": [{"name": "factor", "offset": 1, "kind": "real"},
                                    {"name": "unitName", "offset": 2, "kind": "text", "span": 6}]}]}},
    "Tables": {"fields": [{"name": "id", "slot": 1, "kind": "integer"}],
      "sections": {"name": "lines", "firstSlot": 1, "keyOffset": 1, "kinds": [
        {"key": "K", "fields": [{"name": "k1", "offset": 2, "kind": "integer"},
                                {"name": "k2", "offset": 3, "kind": "integer"}]},
        {"key": "", "fields": [{"name": "rule", "offset": 2, "kind": "text"},
                               {"name": "retention", "offset": 10, "kind": "real"}]},
        {"key": "DESVAR", "list": {"name": "variables", "offset": 2, "period": 1, "perLine": 6, "fields": [
          {"name": "variableId", "offset": 0, "kind": "integer"}]}}]}}
  }
})";

std::vector<std::string> slotsOf(std::initializer_list<std::pair<int, const char*>> filled)
{
    std::vector<std::string> slots;
    for (const auto& [slot, text] : filled)
    {
        if (slots.size() < static_cast<std::size_t>(slot))
            slots.resize(static_cast<std::size_t>(slot));
        slots[static_cast<std::size_t>(slot - 1)] = text;
    }
    return slots;
}

// The record of `values` goes to `expected` slots and comes back the same.
void checkBothWays(const CardLayout& layout, const std::vector<std::string>& values,
                   const std::vector<std::string>& expected)
{
    INFO(layout.card);
    std::vector<std::string> slots;
    std::string problem;
    REQUIRE(layout.toSlots(values, slots, problem));
    CHECK(slots == expected);
    std::vector<std::string> back;
    REQUIRE(layout.toValues(slots, {}, back, problem));
    CHECK(back == values);
}

} // namespace

TEST_CASE("a group with a few repetitions to a line leaves the rest of each line blank", "[model][layouts]")
{
    const auto layouts = CardLayouts::fromJson(SECTION_CATALOGUE);
    checkBothWays(*layouts->find("Pairs"), {"1", "10", "1.", "11", "2.", "12", "3.", "13", "4."},
                  slotsOf({{1, "1"},
                           {10, "10"},
                           {11, "1."},
                           {12, "11"},
                           {13, "2."},
                           {14, "12"},
                           {15, "3."},
                           {18, "13"},
                           {19, "4."}}));
    std::vector<std::string> values;
    std::string problem;
    CHECK_FALSE(layouts->find("Pairs")->toValues(slotsOf({{1, "1"}, {16, "x"}}), {}, values, problem));
    CHECK_THAT(problem, ContainsSubstring("slot 16"));
}

TEST_CASE("sections after a group follow its count and start on the line after it", "[model][layouts]")
{
    const auto layouts = CardLayouts::fromJson(SECTION_CATALOGUE);
    const CardLayout& steps = *layouts->find("Steps");
    checkBothWays(steps, {"1", "5", "1", "", "2", "2.", "3", "", "4", "", "5", "", "UNITS", "60.", "minutes"},
                  slotsOf({{1, "1"},
                           {9, "1"},
                           {11, "2"},
                           {12, "2."},
                           {13, "3"},
                           {15, "4"},
                           {17, "5"},
                           {25, "UNITS"},
                           {26, "60."},
                           {27, "minutes"}}));
    checkBothWays(steps, {"1", "", "UNITS", "60."}, slotsOf({{1, "1"}, {9, "UNITS"}, {10, "60."}}));
    checkBothWays(steps, {"1", "2", "1", "", "2"}, slotsOf({{1, "1"}, {9, "1"}, {11, "2"}}));
    std::vector<std::string> slots;
    std::string problem;
    CHECK_FALSE(steps.toSlots(std::vector<std::string>{"1", "x"}, slots, problem));
    CHECK_THAT(problem, ContainsSubstring("not a count"));
    CHECK_FALSE(steps.toSlots(std::vector<std::string>{"1", "", "HOURS"}, slots, problem));
    CHECK_THAT(problem, ContainsSubstring("HOURS"));
}

TEST_CASE("keyword sections keep their order, their lines and their lists", "[model][layouts]")
{
    const auto layouts = CardLayouts::fromJson(SECTION_CATALOGUE);
    const CardLayout& tables = *layouts->find("Tables");
    checkBothWays(tables, {"7", "DESVAR", "8", "1", "2", "3", "4", "5", "6", "7", "8", "K", "10", "20"},
                  slotsOf({{1, "7"},
                           {2, "DESVAR"},
                           {3, "1"},
                           {4, "2"},
                           {5, "3"},
                           {6, "4"},
                           {7, "5"},
                           {8, "6"},
                           {11, "7"},
                           {12, "8"},
                           {18, "K"},
                           {19, "10"},
                           {20, "20"}}));
    checkBothWays(tables, {"7", "", "TRXY", "0.5", "K", "", "4"},
                  slotsOf({{1, "7"}, {3, "TRXY"}, {11, "0.5"}, {18, "K"}, {20, "4"}}));
    std::vector<std::string> values;
    std::string problem;
    CHECK_FALSE(tables.toValues(slotsOf({{1, "7"}, {2, "k"}, {3, "1"}}), {}, values, problem));
    std::vector<std::string> slots;
    CHECK_FALSE(tables.toSlots(std::vector<std::string>{"7", "k", "1"}, slots, problem));
}

TEST_CASE("sections that do not hold together are refused", "[model][layouts]")
{
    const auto card = [](const std::string& sections)
    {
        return catalogueWith(R"({"fields": [{"name": "id", "slot": 1, "kind": "integer"}], "sections": )" + sections +
                             "}");
    };
    CHECK_THAT(problemOf(card(R"({"name": "s", "firstSlot": 3, "keyOffset": 0, "kinds": [{"key": "A"}]})")),
               ContainsSubstring("first slot of a line"));
    CHECK_THAT(problemOf(card(R"({"name": "s", "firstSlot": 9, "keyOffset": 0, "kinds": [
        {"key": "A", "fields": [{"name": "a", "offset": 8, "kind": "text"}]}]})")),
               ContainsSubstring("place of the key word"));
    CHECK_THAT(
        problemOf(card(R"({"name": "s", "firstSlot": 9, "keyOffset": 0, "kinds": [{"key": "A"}, {"key": "A"}]})")),
        ContainsSubstring("two kinds"));
    CHECK_THAT(problemOf(card(R"({"name": "s", "firstSlot": 9, "keyOffset": 0, "kinds": [
        {"key": "A", "list": {"name": "l", "offset": 1, "period": 1, "fields": [{"name": "v", "offset": 0, "kind": "text"}]}}]})")),
               ContainsSubstring("it holds numbers"));
    CHECK_THAT(problemOf(card(R"({"name": "s", "firstSlot": 1, "keyOffset": 0, "kinds": [{"key": "A"}]})")),
               ContainsSubstring("in the slots of the sections"));
}

TEST_CASE("a text with a span of 0 runs to the last slot of the card", "[model][layouts]")
{
    const auto layouts =
        CardLayouts::fromJson(catalogueWith(R"({"fields": [{"name": "id", "slot": 1, "kind": "integer"},
        {"name": "list", "slot": 9, "kind": "text", "span": 0}]})"));
    const CardLayout& card = *layouts->find("Card");
    std::vector<std::string> values;
    std::string problem;
    REQUIRE(card.toValues(slotsOf({{1, "4"}, {9, "LIST=(1,"}, {10, "2)"}, {30, "x"}}), {}, values, problem));
    CHECK(values == std::vector<std::string>{"4", "LIST=(1,2)x"});
    std::vector<std::string> slots;
    REQUIRE(card.toSlots(std::vector<std::string>{"4", "LIST=(1,2)"}, slots, problem));
    CHECK(slots == slotsOf({{1, "4"}, {9, "LIST=(1,2)"}}));
    CHECK_THAT(problemOf(catalogueWith(R"({"fields": [{"name": "a", "slot": 9, "kind": "text", "span": 0},
                                                     {"name": "b", "slot": 5, "kind": "real"}]})")),
               ContainsSubstring("runs to the end"));
}

TEST_CASE("a card with forms takes the form its key value selects, in a record and in the slots", "[model][layouts]")
{
    const CardLayouts& layouts = *CardLayouts::standard();
    const CardLayout& matrix = *layouts.find("DirectMatrix");
    checkBothWays(matrix, {"KAA", "0", "6", "1", "", "12", "12"},
                  slotsOf({{1, "KAA"}, {2, "0"}, {3, "6"}, {4, "1"}, {7, "12"}, {8, "12"}}));
    checkBothWays(matrix, {"KAA", "3", "1", "2.5", "Through", "4"},
                  slotsOf({{1, "KAA"}, {2, "3"}, {3, "1"}, {4, "2.5"}, {5, "Through"}, {6, "4"}}));
    // The kind of a value selects the form of a curved beam property: an integer for a pipe.
    const CardLayout& bend = *layouts.find("CurvedBeamProp");
    checkBothWays(bend, {"1", "2", "1", "0.1", "0.01", "", "", "", "0.5"},
                  slotsOf({{1, "1"}, {2, "2"}, {3, "1"}, {4, "0.1"}, {5, "0.01"}, {11, "0.5"}}));
    checkBothWays(bend, {"1", "2", "0.1", "1.", "1.", "2.", "", "", "", "", "", "", "", "", "", "", "", "", "0.5"},
                  slotsOf({{1, "1"}, {2, "2"}, {3, "0.1"}, {4, "1."}, {5, "1."}, {6, "2."}, {19, "0.5"}}));
}

TEST_CASE("a form of the old layout only is read from the slots, a record takes the other one", "[model][layouts]")
{
    const CardLayout& name = *CardLayouts::standard()->find("UserFileName");
    std::vector<std::string> values;
    std::string problem;
    REQUIRE(name.toValues(slotsOf({{1, "3"}, {2, "results"}, {3, "/run.h5"}}), {}, values, problem));
    CHECK(values == std::vector<std::string>{"3", "results/run.h5"});
    std::vector<std::string> slots;
    REQUIRE(name.toSlots(values, slots, problem));
    CHECK(slots == slotsOf({{1, "3"}, {9, "results/run.h5"}}));
}

TEST_CASE("groups follow each other, counted by a field, a fixed count or a marker", "[model][layouts]")
{
    const CardLayouts& layouts = *CardLayouts::standard();
    // Two lists of X values counted by valueCount1 and valueCount2, each on a line of its own, then the ordinates.
    std::vector<std::string> table{"7", "2", "1", "2", "", "", "2", "1"};
    table.resize(26);
    for (const char* value : {"0.", "1.", "5.", "10.", "20."})
        table.emplace_back(value);
    std::vector<std::string> slots = slotsOf({{1, "7"}, {2, "2"}, {3, "1"}, {4, "2"}, {7, "2"}, {8, "1"}});
    slots.resize(41);
    slots[32] = "0.";
    slots[33] = "1.";
    slots[40] = "5.";
    slots.resize(51);
    slots[48] = "10.";
    slots[49] = "20.";
    slots.pop_back();
    checkBothWays(*layouts.find("TableMultiVariable1"), table, slots);
    // The independent nodes, EndList, then the dependent nodes; the record gives the number of the first ones.
    const CardLayout& searched = *layouts.find("RigidBodySearched");
    std::vector<std::string> nodes{"1", "", "", "", "0.5"};
    nodes.resize(10);
    for (const char* value : {"2", "10", "11", "20"})
        nodes.emplace_back(value);
    checkBothWays(searched, nodes,
                  slotsOf({{1, "1"}, {6, "0.5"}, {12, "10"}, {13, "11"}, {14, "EndList"}, {15, "20"}}));
    nodes[10] = "";
    nodes.erase(nodes.begin() + 11, nodes.begin() + 13);
    checkBothWays(searched, nodes, slotsOf({{1, "1"}, {6, "0.5"}, {12, "EndList"}, {13, "20"}}));
}

TEST_CASE("a nested list ends where a value fits the first field of the group", "[model][layouts]")
{
    const CardLayout& interpolation = *CardLayouts::standard()->find("RigidInterpolation");
    checkBothWays(interpolation, {"9",    "1",
                                  "123",  "2",
                                  "1.",   "123",
                                  "2",    "2",
                                  "3",    "0.5",
                                  "1",    "1",
                                  "4",    "DependentDofs",
                                  "2",    "5",
                                  "1",    "6",
                                  "2",    "ThermalExpansion",
                                  "1.e-5"},
                  slotsOf({{1, "9"},
                           {3, "1"},
                           {4, "123"},
                           {5, "1."},
                           {6, "123"},
                           {7, "2"},
                           {8, "3"},
                           {9, "0.5"},
                           {10, "1"},
                           {11, "4"},
                           {17, "DependentDofs"},
                           {18, "5"},
                           {19, "1"},
                           {20, "6"},
                           {21, "2"},
                           {25, "ThermalExpansion"},
                           {26, "1.e-5"}}));
}

TEST_CASE("sections may be packed, have a key word of their own place, or take all their lines", "[model][layouts]")
{
    const CardLayouts& layouts = *CardLayouts::standard();
    checkBothWays(*layouts.find("ConnectorSearchParams"),
                  {"1", "WeldMass", "1", "Weld", "1", "GeometryCheckLevel", "2"},
                  slotsOf({{1, "WeldMass"}, {2, "1"}, {3, "Weld"}, {4, "GeometryCheckLevel"}, {5, "2"}}));
    const CardLayout& table = *layouts.find("ContactTable");
    std::vector<std::string> values{"4", "", "", "", "", "", "Secondary", "2"};
    values.resize(37); // the 30 fields of a secondary set
    values.emplace_back("SegmentContact");
    values.emplace_back("0.5");
    checkBothWays(table, values, slotsOf({{1, "4"}, {9, "Secondary"}, {10, "2"}, {18, "SegmentContact"}, {19, "0.5"}}));
    // A station with stress points takes its two lines, blank or not; the last lines start after it.
    const CardLayout& beam = *layouts.find("BeamProp");
    std::vector<std::string> stations{"1", "2", "1.", "1.", "1.", "", "", "", "Yes", "1."};
    stations.resize(24);
    stations.emplace_back("");
    stations.resize(33);
    stations.emplace_back("1.e-3");
    checkBothWays(
        beam, stations,
        slotsOf({{1, "1"}, {2, "2"}, {3, "1."}, {4, "1."}, {5, "1."}, {9, "Yes"}, {10, "1."}, {33, "1.e-3"}}));
}

TEST_CASE("forms, groups and sections that do not hold together are refused", "[model][layouts]")
{
    const std::string id = R"({"name": "id", "slot": 1, "kind": "integer"})";
    CHECK_THAT(problemOf(catalogueWith(R"({"forms": [{"name": "a", "fields": [)" + id +
                                       R"(]}, {"name": "b", "fields": [)" + id + "]}]}")),
               ContainsSubstring("only the last form"));
    CHECK_THAT(
        problemOf(catalogueWith(R"({"forms": [{"name": "a", "when": {"field": "x", "values": ["1"]}, "fields": [)" +
                                id + R"(]}, {"name": "b", "fields": [)" + id + "]}]}")),
        ContainsSubstring("the form has no field x"));
    CHECK_THAT(problemOf(catalogueWith(R"({"fields": [)" + id +
                                       R"(], "group": {"name": "a", "firstSlot": 9, "period": 1, "fields": [
        {"name": "v", "offset": 0, "kind": "real"}]}, "nextGroups": [{"name": "b", "period": 1, "fields": [
        {"name": "w", "offset": 0, "kind": "real"}]}]})")),
               ContainsSubstring("it gives a count, or the next group a marker"));
    CHECK_THAT(
        problemOf(catalogueWith(R"({"fields": [)" + id +
                                R"(], "group": {"name": "a", "firstSlot": 9, "period": 1, "count": "n", "fields": [
        {"name": "v", "offset": 0, "kind": "real"}]}})")),
        ContainsSubstring("count field n"));
    CHECK_THAT(problemOf(catalogueWith(R"({"fields": [)" + id +
                                       R"(], "group": {"name": "a", "firstSlot": 9, "period": 1, "fields": [
        {"name": "v", "offset": 0, "kind": "integer"}], "list": {"name": "l", "fields": [
        {"name": "w", "offset": 0, "kind": "integer"}]}}})")),
               ContainsSubstring("tells apart"));
}

TEST_CASE("a count larger than the values given is refused before any repetition is placed", "[model][layouts]")
{
    // padCount (the 13th field, slot 33) counts the pads; two billion pads would take all the memory.
    const CardLayout& bearing = *CardLayouts::standard()->find("TiltingPadBearingProp");
    std::vector<std::string> values(15);
    values[0] = "1";
    values[12] = "2000000000";
    values.emplace_back("1.");
    std::vector<std::string> slots;
    std::string problem;
    CHECK_FALSE(bearing.toSlots(values, slots, problem));
    CHECK_THAT(problem, ContainsSubstring("is more than the"));
    std::vector<std::string> back;
    problem.clear();
    CHECK_FALSE(bearing.toValues(slotsOf({{1, "1"}, {33, "2000000000"}, {41, "1."}}), {}, back, problem));
    CHECK_THAT(problem, ContainsSubstring("is more than the"));
}

TEST_CASE("neutral form values select layouts only with their exact case", "[model][layouts]")
{
    const CardLayout& beam = *CardLayouts::standard()->find("BeamShapeProp");
    std::vector<std::string> values{"1", "2", "", "Rod", "0.1"};
    std::vector<std::string> slots;
    std::string problem;
    REQUIRE(beam.toSlots(values, slots, problem));
    const CardLayout* selected = recordLayout(beam, values);
    REQUIRE(selected != nullptr);
    REQUIRE(placeSlots(beam, slots).layout == selected);
    values[3] = "rod";
    CHECK(recordLayout(beam, values) != selected);
    slots = slotsOf({{1, "1"}, {2, "2"}, {4, "rod"}, {9, "0.1"}});
    CHECK(placeSlots(beam, slots).layout != selected);
}
