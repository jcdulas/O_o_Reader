// The user dummy elements: the 27 cards ADUM1 to ADUM9 (the attributes of an element), CDUM1 to
// CDUM9 (its connection) and PDUM1 to PDUM9 (its property). Each number is a card of its own, so each test runs over
// the nine tables of a kind. What is checked: the fields in their positions of the format reference (the examples of
// the formats), the default of a blank, the fields of a connection card that are grid points and the ones that are not,
// the references a card lists and what a renumbering rewrites, the ID spaces, the refused values with their field, and
// the round trip through a model file.

#include "oor/model/cards/elements/DummyFieldList.hpp"
#include "oor/model/cards/elements/rod2/Rod2Table.hpp"
#include "oor/model/cards/elements/userDefined1/UserDefined1Table.hpp"
#include "oor/model/cards/elements/userDefined2/UserDefined2Table.hpp"
#include "oor/model/cards/elements/userDefined3/UserDefined3Table.hpp"
#include "oor/model/cards/elements/userDefined4/UserDefined4Table.hpp"
#include "oor/model/cards/elements/userDefined5/UserDefined5Table.hpp"
#include "oor/model/cards/elements/userDefined6/UserDefined6Table.hpp"
#include "oor/model/cards/elements/userDefined7/UserDefined7Table.hpp"
#include "oor/model/cards/elements/userDefined8/UserDefined8Table.hpp"
#include "oor/model/cards/elements/userDefined9/UserDefined9Table.hpp"
#include "oor/model/cards/elements/userDefined1Attributes/UserDefined1AttributesTable.hpp"
#include "oor/model/cards/elements/userDefined2Attributes/UserDefined2AttributesTable.hpp"
#include "oor/model/cards/elements/userDefined3Attributes/UserDefined3AttributesTable.hpp"
#include "oor/model/cards/elements/userDefined4Attributes/UserDefined4AttributesTable.hpp"
#include "oor/model/cards/elements/userDefined5Attributes/UserDefined5AttributesTable.hpp"
#include "oor/model/cards/elements/userDefined6Attributes/UserDefined6AttributesTable.hpp"
#include "oor/model/cards/elements/userDefined7Attributes/UserDefined7AttributesTable.hpp"
#include "oor/model/cards/elements/userDefined8Attributes/UserDefined8AttributesTable.hpp"
#include "oor/model/cards/elements/userDefined9Attributes/UserDefined9AttributesTable.hpp"
#include "oor/model/cards/elements/userDefined1Prop/UserDefined1PropTable.hpp"
#include "oor/model/cards/elements/userDefined2Prop/UserDefined2PropTable.hpp"
#include "oor/model/cards/elements/userDefined3Prop/UserDefined3PropTable.hpp"
#include "oor/model/cards/elements/userDefined4Prop/UserDefined4PropTable.hpp"
#include "oor/model/cards/elements/userDefined5Prop/UserDefined5PropTable.hpp"
#include "oor/model/cards/elements/userDefined6Prop/UserDefined6PropTable.hpp"
#include "oor/model/cards/elements/userDefined7Prop/UserDefined7PropTable.hpp"
#include "oor/model/cards/elements/userDefined8Prop/UserDefined8PropTable.hpp"
#include "oor/model/cards/elements/userDefined9Prop/UserDefined9PropTable.hpp"
#include "oor/model/cards/materials/isoMat/IsoMatTable.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/properties/shellProp/ShellPropTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

using AdumTables = std::tuple<UserDefined1AttributesTable, UserDefined2AttributesTable, UserDefined3AttributesTable,
                              UserDefined4AttributesTable, UserDefined5AttributesTable, UserDefined6AttributesTable,
                              UserDefined7AttributesTable, UserDefined8AttributesTable, UserDefined9AttributesTable>;
using CdumTables = std::tuple<UserDefined1Table, UserDefined2Table, UserDefined3Table, UserDefined4Table, UserDefined5Table,
                              UserDefined6Table, UserDefined7Table, UserDefined8Table, UserDefined9Table>;
using PdumTables =
    std::tuple<UserDefined1PropTable, UserDefined2PropTable, UserDefined3PropTable, UserDefined4PropTable, UserDefined5PropTable,
               UserDefined6PropTable, UserDefined7PropTable, UserDefined8PropTable, UserDefined9PropTable>;

DummyField integer(std::int32_t value)
{
    return DummyField::ofInteger(value);
}

DummyField real(double value)
{
    return DummyField::ofReal(value);
}

DummyField blank()
{
    return DummyField{};
}

// The record of a card with `count` integers or reals after its first two fields.
std::string longCard(const std::string& name, std::size_t count)
{
    std::string text = name + ";7;9";
    for (std::size_t index = 0; index < count; ++index)
        text += ";" + (index % 3 == 2 ? std::string("1.5E3") : std::to_string(index + 20));
    return text;
}

const std::string NODES_TEXT = "Node;3\nNode;4\nNode;7\nNode;9\nNode;12\n";

} // namespace

// --- ADUMi -----------------------------------------------------------------------------------------------------------

TEMPLATE_LIST_TEST_CASE("an ADUMi reads its five fields, takes zero for a blank count and keeps the name exactly",
                        "[model][cards][dummy]", AdumTables)
{
    const std::string name(TestType::NAME);
    Model model = readBulk(name + ";14;3;7;3;quad8x\n" + name + ";3;;;;A");
    REQUIRE(rowCount<TestType>(model) == 2);
    const auto full = cardAt<TestType>(model, 0);
    REQUIRE((full.nodeCount == 14 && full.connectionFieldCount == 3 && full.propertyFieldCount == 7 &&
             full.displacementComponentCount == 3));
    REQUIRE(full.elementName == "quad8x"); // kept as written
    const auto defaults = cardAt<TestType>(model, 1);
    REQUIRE((defaults.nodeCount == 3 && defaults.connectionFieldCount == 0 && defaults.propertyFieldCount == 0 &&
             defaults.displacementComponentCount == 0 && defaults.elementName == "A"));

    // The card has no ID and no category of its own, and ELNM is a name: nothing is referred to.
    REQUIRE(TestType::INFO.category == CardCategory::Other);
    REQUIRE(TestType::INFO.idSpace == base::IdSpace::None);
    RecordingVisitor visitor;
    model.table<TestType>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
}

TEMPLATE_LIST_TEST_CASE("an ADUMi takes the largest sizes and the three values of ND", "[model][cards][dummy]",
                        AdumTables)
{
    const std::string name(TestType::NAME);
    for (const char* line : {";1;0;0;0;X", ";9999;1000;24;3;X", ";2;5;24;6;EIGHTCHR"})
        REQUIRE_NOTHROW(readBulk(name + line));
    // A blank ELNM leaves the cards their own names (CDUMi, PDUMi): it is kept blank and written blank.
    const Model blank = readBulk(name + ";2;1;2;0");
    REQUIRE(cardAt<TestType>(blank).elementName.empty());
    requireRoundTrip(blank);
}

TEMPLATE_LIST_TEST_CASE("an ADUMi refuses the values its format forbids, naming the field", "[model][cards][dummy]",
                        AdumTables)
{
    const std::string name(TestType::NAME);
    expectInvalid(name + ";0;0;0;0;AB", "nodeCount");
    expectInvalid(name + ";-3;0;0;0;AB", "nodeCount");
    expectInvalid(name + ";;0;0;0;AB", "nodeCount"); // required
    expectInvalid(name + ";4;-1;0;0;AB", "connectionFieldCount");
    expectInvalid(name + ";4;0.5;0;0;AB", "connectionFieldCount");
    expectInvalid(name + ";4;0;25;0;AB", "propertyFieldCount"); // at most 24 additional fields on the property
    expectInvalid(name + ";4;0;-1;0;AB", "propertyFieldCount");
    expectInvalid(name + ";4;0;0;1;AB", "displacementComponentCount"); // 0, 3 or 6
    expectInvalid(name + ";4;0;0;4;AB", "displacementComponentCount");
    const Model longName = readBulk(name + ";4;0;0;0;LongUserElementName");
    REQUIRE(cardAt<TestType>(longName).elementName == "LongUserElementName");
    requireRoundTrip(longName);
}

TEST_CASE("two ADUM cards of one number are both kept: the format reference does not forbid them",
          "[model][cards][dummy]")
{
    const Model model = readBulk("UserDefined3Attributes;4;0;0;0;AAA\nUserDefined3Attributes;5;1;1;0;BBB");
    REQUIRE(rowCount<UserDefined3AttributesTable>(model) == 2);
    REQUIRE(cardAt<UserDefined3AttributesTable>(model, 1).elementName == "BBB");
}

// --- CDUMi -----------------------------------------------------------------------------------------------------------

TEMPLATE_LIST_TEST_CASE("a CDUMi keeps every field after the PID in the positions of the card (a common deck)",
                        "[model][cards][dummy]", CdumTables)
{
    const std::string name(TestType::NAME);
    // Five grid points and a blank in the first six places, then the additional fields: 1.8, a blank,
    // 6.E3, 3, a blank and 40.
    Model model = readBulk(name + ";126;117;3;4;7;9;12;;1.8;;6.E3;3;;40\n" + NODES_TEXT);
    const auto card = cardAt<TestType>(model);
    REQUIRE((card.id == 126 && card.propId == 117));
    const DummyFieldList expected = {integer(3), integer(4), integer(7), integer(9), integer(12), blank(),
                                     real(1.8),  blank(),    real(6.E3), integer(3), blank(),     integer(40)};
    REQUIRE(card.fields == expected);
    // The grid points the model sees are the run of integers at the start: the sixth place is blank, so it ends there.
    REQUIRE(card.nodes() == std::vector<base::EntityId>{3, 4, 7, 9, 12});
    const auto stored = model.findTable<TestType>()->nodes(0);
    REQUIRE(std::vector<base::EntityId>(stored.begin(), stored.end()) == card.nodes());

    REQUIRE(TestType::INFO.category == CardCategory::Element);
    REQUIRE(TestType::INFO.idSpace == base::IdSpace::Element);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
}

TEMPLATE_LIST_TEST_CASE("a CDUMi lists its grid points as nodes and its PID as nothing", "[model][cards][dummy]",
                        CdumTables)
{
    const std::string name(TestType::NAME);
    Model model = readBulk(name + ";126;117;3;4;7;9;12;;1.8;;6.E3;3;;40\n" + NODES_TEXT);
    RecordingVisitor visitor;
    model.table<TestType>().visitReferences(0, visitor);
    // PID may name a PDUMi or, for an element that needs no property, a material: not a reference.
    REQUIRE(visitor.fields == std::vector<std::string>(5, "node1"));
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(5, ReferenceTarget::Node));
}

TEMPLATE_LIST_TEST_CASE("a renumbering rewrites the grid points of a CDUMi and leaves its additional fields alone",
                        "[model][cards][dummy]", CdumTables)
{
    const std::string name(TestType::NAME);
    // The integer 3 among the additional fields happens to be the number of a grid point.
    Model model = readBulk(name + ";126;117;3;4;7;9;12;;1.8;;6.E3;3;;40\n" + NODES_TEXT);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    const auto card = cardAt<TestType>(model);
    const DummyFieldList expected = {integer(103), integer(104), integer(107), integer(109), integer(112), blank(),
                                     real(1.8),    blank(),      real(6.E3),   integer(3),   blank(),      integer(40)};
    REQUIRE(card.fields == expected);
    REQUIRE(card.nodes() == std::vector<base::EntityId>{103, 104, 107, 109, 112});
    const auto stored = model.findTable<TestType>()->nodes(0);
    REQUIRE(std::vector<base::EntityId>(stored.begin(), stored.end()) == card.nodes());
    REQUIRE(unresolvedIn(model) == 0);

    // The element ID is renumbered with the elements; the PID is a plain ID and no renumbering of the properties moves
    // it.
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(1000));
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE((cardAt<TestType>(model).id == 1126 && cardAt<TestType>(model).propId == 117));
    requireRoundTrip(model);
}

TEMPLATE_LIST_TEST_CASE("the grid points of a CDUMi end at the first blank, real, or integer that is not positive",
                        "[model][cards][dummy]", CdumTables)
{
    const std::string name(TestType::NAME);
    // A grid point left out in the middle ends the run, and the blank at the end of the card is not kept.
    const Model gap = readBulk(name + ";1;7;10;11;;12;3.5;");
    const auto left = cardAt<TestType>(gap);
    REQUIRE(left.fields == DummyFieldList{integer(10), integer(11), blank(), integer(12), real(3.5)});
    REQUIRE(left.nodes() == std::vector<base::EntityId>{10, 11});

    // An integer additional field that follows the last grid point directly cannot be told from a grid point.
    const Model flag = readBulk(name + ";1;7;10;11;5;2.5");
    REQUIRE(cardAt<TestType>(flag).nodes() == std::vector<base::EntityId>{10, 11, 5});
    REQUIRE(cardAt<TestType>(flag).fields == DummyFieldList{integer(10), integer(11), integer(5), real(2.5)});

    // Zero and negative integers are not grid points: they end the run and stay in the list as the integers they are.
    const Model zero = readBulk(name + ";1;7;10;0;12");
    REQUIRE(cardAt<TestType>(zero).nodes() == std::vector<base::EntityId>{10});
    REQUIRE(cardAt<TestType>(zero).fields == DummyFieldList{integer(10), integer(0), integer(12)});
    const Model negative = readBulk(name + ";1;7;10;-1;12");
    REQUIRE(cardAt<TestType>(negative).nodes() == std::vector<base::EntityId>{10});

    // A real is a real, whatever it looks like: 3. and 3.0 and 3.E0 are one value, and 3 is an integer.
    const Model kinds = readBulk(name + ";1;7;10;3.;3.0E0;3");
    REQUIRE(cardAt<TestType>(kinds).fields == DummyFieldList{integer(10), real(3.0), real(3.0), integer(3)});
    requireRoundTrip(gap);
    requireRoundTrip(flag);
    requireRoundTrip(zero);
    requireRoundTrip(negative);
    requireRoundTrip(kinds);
}

TEMPLATE_LIST_TEST_CASE("a CDUMi may have many lines of fields and keeps them all", "[model][cards][dummy]", CdumTables)
{
    const std::string name(TestType::NAME);
    const Model model = readBulk(longCard(name, 60));
    const auto card = cardAt<TestType>(model);
    REQUIRE(card.fields.size() == 60);
    REQUIRE(card.fields.front() == integer(20));
    REQUIRE(card.fields[2] == real(1.5E3));
    REQUIRE(card.fields[58] == integer(78)); // every third field is a real
    REQUIRE(card.fields.back() == real(1.5E3));
    REQUIRE(card.nodes() == std::vector<base::EntityId>{20, 21});
    requireRoundTrip(model);
}

TEMPLATE_LIST_TEST_CASE("a CDUMi refuses what its format forbids, naming the field", "[model][cards][dummy]",
                        CdumTables)
{
    const std::string name(TestType::NAME);
    expectInvalid(name + ";0;7;10", "id");
    expectInvalid(name + ";-5;7;10", "id");
    expectInvalid(name + ";100000000;7;10", "id"); // below 100,000,000
    expectInvalid(name + ";;7;10", "id");
    expectInvalid(name + ";1;0;10", "propId");
    expectInvalid(name + ";1;;10", "propId");
    expectInvalid(name + ";1;7", "node1"); // an element connects a grid point at least
    expectInvalid(name + ";1;7;2.5", "node1");
    expectInvalid(name + ";1;7;;5", "node1");
    expectInvalid(name + ";1;7;0", "node1");
    expectInvalid(name + ";1;7;10;ABC", "nodes/connectionFields"); // a field is an integer, a real or blank
    expectInvalid(name + ";1;7;10;1.E999", "nodes/connectionFields");
    REQUIRE_NOTHROW(readBulk(name + ";99999999;7;10"));
}

TEST_CASE("the CDUM cards of every number share the element ID space, with every other element",
          "[model][cards][dummy]")
{
    REQUIRE_NOTHROW(readBulk("UserDefined1;5;7;10\nUserDefined2;6;7;10\nRod2;7;1;10;11"));
    REQUIRE_THROWS_AS(readBulk("UserDefined1;5;7;10\nUserDefined2;5;7;11"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("UserDefined9;5;7;10\nRod2;5;1;10;11"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("UserDefined3;5;7;10\nUserDefined3;5;7;10"), base::DeckError);
}

// --- PDUMi -----------------------------------------------------------------------------------------------------------

TEMPLATE_LIST_TEST_CASE("a PDUMi keeps its additional fields in the positions of the card (a common deck)",
                        "[model][cards][dummy]", PdumTables)
{
    const std::string name(TestType::NAME);
    Model model = readBulk(name + ";117;3;1.8;7.2;4.E3;12.;;2.5;6;;4\nIsoMat;3;2.1E+11;;0.3");
    const auto card = cardAt<TestType>(model);
    REQUIRE((card.id == 117 && card.matId == 3));
    const DummyFieldList expected = {real(1.8), real(7.2),  real(4.E3), real(12.), blank(),
                                     real(2.5), integer(6), blank(),    integer(4)};
    REQUIRE(card.fields == expected);
    REQUIRE(TestType::INFO.category == CardCategory::Property);
    REQUIRE(TestType::INFO.idSpace == base::IdSpace::Property);
    REQUIRE(unresolvedIn(model) == 0);

    // Only the material is a reference: an additional field names nothing, an integer one included.
    RecordingVisitor visitor;
    model.table<TestType>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material});
    requireRoundTrip(model);
}

TEMPLATE_LIST_TEST_CASE("a PDUMi renumbers with the properties, its material with the materials",
                        "[model][cards][dummy]", PdumTables)
{
    const std::string name(TestType::NAME);
    Model model = readBulk(name + ";117;3;1.8;7.2;4.E3;12.;;2.5;6;;4\nIsoMat;3;2.1E+11;;0.3");
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(10));
    REQUIRE((cardAt<TestType>(model).matId == 13 && cardAt<TestType>(model).id == 117));
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(100));
    const auto card = cardAt<TestType>(model);
    REQUIRE((card.id == 217 && card.matId == 13));
    // The integers among the additional fields (A7 and A9) are the user's numbers, not references.
    REQUIRE((card.fields[6] == integer(6) && card.fields[8] == integer(4)));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
}

TEMPLATE_LIST_TEST_CASE("a PDUMi may have no additional field and at most 24", "[model][cards][dummy]", PdumTables)
{
    const std::string name(TestType::NAME);
    const Model none = readBulk(name + ";1;2");
    REQUIRE(cardAt<TestType>(none).fields.empty());
    const Model full = readBulk(longCard(name, 24));
    REQUIRE(cardAt<TestType>(full).fields.size() == 24);
    requireRoundTrip(none);
    requireRoundTrip(full);
}

TEMPLATE_LIST_TEST_CASE("a PDUMi refuses what its format forbids, naming the field", "[model][cards][dummy]",
                        PdumTables)
{
    const std::string name(TestType::NAME);
    expectInvalid(name + ";0;2", "id");
    expectInvalid(name + ";;2", "id");
    expectInvalid(name + ";1;0", "matId");
    expectInvalid(name + ";1", "matId");
    expectInvalid(name + ";1;2;1.5;ABC", "propertyFields");
    expectInvalid(longCard(name, 25), "propertyFields"); // at most 24 additional fields
}

TEST_CASE("the PDUM cards of every number share the property ID space, with every other property",
          "[model][cards][dummy]")
{
    REQUIRE_NOTHROW(readBulk("UserDefined1Prop;5;1\nUserDefined2Prop;6;1\nShellProp;7;1;0.1"));
    REQUIRE_THROWS_AS(readBulk("UserDefined1Prop;5;1\nUserDefined2Prop;5;1"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("UserDefined9Prop;5;1\nShellProp;5;1;0.1"), base::DeckError);
}

// --- the 27 cards together ---------------------------------------------------------------------------------------

TEST_CASE("the cards of the nine numbers are read together, resolve and go through a model file",
          "[model][cards][dummy]")
{
    std::string deck = "Node;1\nNode;2\nNode;3\nIsoMat;1;2.1E+11;;0.3\n";
    for (int number = 1; number <= 9; ++number)
    {
        const std::string suffix = std::to_string(number);
        deck += "UserDefined" + suffix + "Attributes;6;2;3;3;USER" + suffix + "\n";
        deck += "UserDefined" + suffix + ";" + std::to_string(100 + number) + ";" + std::to_string(200 + number) +
                ";1;2;3;;2.5;7\n";
        deck += "UserDefined" + suffix + "Prop;" + std::to_string(200 + number) + ";1;1.5;;3\n";
    }
    Model model = readBulk(deck);
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(model.findTable<UserDefined4AttributesTable>() != nullptr);
    REQUIRE(model.findTable<UserDefined7Table>() != nullptr);
    REQUIRE(model.findTable<UserDefined9PropTable>() != nullptr);
    REQUIRE(cardAt<UserDefined4AttributesTable>(model).elementName == "USER4");
    REQUIRE(cardAt<UserDefined7Table>(model).id == 107);
    REQUIRE(cardAt<UserDefined9PropTable>(model).id == 209);
    requireRoundTrip(model);
}
