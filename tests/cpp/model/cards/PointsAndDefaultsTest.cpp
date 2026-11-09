// Checks of the points and of the default cards: scalar and extra points in the node ID space, the
// ScalarOrGridPoint reference, and the defaults of GRDSET, BAROR and BEAMOR applied when a deck is read.

#include "oor/model/cards/coordinate_systems/generalFrame3/GeneralFrame3Table.hpp"
#include "oor/model/cards/elements/bar2/Bar2Table.hpp"
#include "oor/model/cards/elements/bar2Defaults/Bar2DefaultsTable.hpp"
#include "oor/model/cards/elements/beam2/Beam2Table.hpp"
#include "oor/model/cards/elements/beam2Defaults/Beam2DefaultsTable.hpp"
#include "oor/model/cards/elements/springDofProperty/SpringDofPropertyTable.hpp"
#include "oor/model/cards/elements/springScalarProperty/SpringScalarPropertyTable.hpp"
#include "oor/model/cards/elements/springScalarInline/SpringScalarInlineTable.hpp"
#include "oor/model/cards/nodes/axisymFluidRingNode/AxisymFluidRingNodeTable.hpp"
#include "oor/model/cards/nodes/extraPoint/ExtraPointTable.hpp"
#include "oor/model/cards/nodes/geomPoint/GeomPointTable.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/nodes/nodeDefaults/NodeDefaultsTable.hpp"
#include "oor/model/cards/nodes/scalarPoint/ScalarPointTable.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "oor/model/references/Renumbering.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using oor::testing::readModelText;
using oor::testing::writeModelText;

namespace
{

Model readBulk(const std::string& cards, bool strict = true)
{
    return readModelText(cards, strict);
}

template <typename TableT>
typename TableT::CardType cardAt(const Model& model, std::uint32_t row = 0)
{
    return model.findTable<TableT>()->card(row);
}

std::size_t unresolvedIn(Model& model)
{
    base::DiagnosticSink sink(base::ReadMode::Tolerant);
    return ReferenceChecker::check(model, sink);
}

// The IDs of the live rows of a table, in row order.
template <typename TableT>
std::vector<base::EntityId> idsOf(const Model& model)
{
    std::vector<base::EntityId> ids;
    const TableT* table = model.findTable<TableT>();
    for (std::uint32_t row = 0; table != nullptr && row < table->rows().rowCount(); ++row)
    {
        if (table->rows().isAlive(row))
            ids.push_back(table->idAtRow(row));
    }
    return ids;
}

} // namespace

TEST_CASE("an SPOINT card gives one scalar point per ID, a THRU range giving all of its points", "[model][cards]")
{
    const Model model = readBulk("ScalarPoint;1;3;Through;6;10;20");
    REQUIRE(idsOf<ScalarPointTable>(model) == std::vector<base::EntityId>{1, 3, 4, 5, 6, 10, 20});
    const Model continued = readBulk("ScalarPoint;1;2;3;4;5;6;7;8;9;10");
    REQUIRE(idsOf<ScalarPointTable>(continued).size() == 10);
    const Model blanks = readBulk("ScalarPoint;;5;;7");
    REQUIRE(idsOf<ScalarPointTable>(blanks) == std::vector<base::EntityId>{5, 7});
}

TEST_CASE("scalar and extra points are members of the node ID space, next to the grid points", "[model][cards]")
{
    const Model model = readBulk("Node;1;;0.;0.;0.\nScalarPoint;14;Through;16\nExtraPoint;20;21");
    const std::optional<CardLocation> scalar = model.locate(base::IdSpace::Node, 15);
    REQUIRE(scalar.has_value());
    REQUIRE(scalar->table == typeIndexOf<ScalarPointTable>());
    REQUIRE(model.locate(base::IdSpace::Node, 21)->table == typeIndexOf<ExtraPointTable>());
    REQUIRE(model.locate(base::IdSpace::Node, 1)->table == typeIndexOf<NodeTable>());
    REQUIRE_FALSE(model.locate(base::IdSpace::Node, 17).has_value());
    // A node reference of any kind is resolved by any of them.
    for (const ReferenceTarget target :
         {ReferenceTarget::Node, ReferenceTarget::NodeInRange, ReferenceTarget::ScalarOrGridPoint})
    {
        REQUIRE(resolveReference(model, target, 1).has_value());
        REQUIRE(resolveReference(model, target, 15).has_value());
        REQUIRE(resolveReference(model, target, 21).has_value());
        REQUIRE_FALSE(resolveReference(model, target, 99).has_value());
    }
}

TEST_CASE("a scalar point and a grid point cannot have the same ID, but a scalar point may be listed twice",
          "[model][cards]")
{
    REQUIRE_THROWS_AS(readBulk("Node;5;;0.;0.;0.\nScalarPoint;5"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("ScalarPoint;5\nExtraPoint;5"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("ScalarPoint;5\nNode;5;;0.;0.;0."), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("GeomPoint;5;;1.;2.;3.\nScalarPoint;5"), base::DeckError);
    // The format reference allows repeated scalar point IDs: the repeat is the same point.
    const Model model = readBulk("ScalarPoint;5;6\nScalarPoint;6;7;5");
    REQUIRE(idsOf<ScalarPointTable>(model).size() == 3);
    REQUIRE(model.diagnostics().diagnostics().empty());
    oor::testing::requireModelFileRoundTrip(model);
    // In tolerant mode a clash with a grid point is a warning and the last card wins.
    const Model tolerant = readBulk("Node;5;;0.;0.;0.\nScalarPoint;5", false);
    REQUIRE(tolerant.cardCount() == 1);
    REQUIRE(tolerant.locate(base::IdSpace::Node, 5)->table == typeIndexOf<ScalarPointTable>());
    REQUIRE_FALSE(tolerant.diagnostics().diagnostics().empty());
}

TEST_CASE("an SPOINT THRU range must go up and stay small, and THRU needs a single ID before it", "[model][cards]")
{
    REQUIRE_THROWS_AS(readBulk("ScalarPoint;1;Through"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("ScalarPoint;3;Through;3"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("ScalarPoint;3;Through;2"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("ScalarPoint;Through;3"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("ScalarPoint;1;Through;3;Through;9"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("ScalarPoint;1;Through;2000000000"), base::DeckError); // would make two billion points
    REQUIRE_THROWS_AS(readBulk("ScalarPoint;0"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("ScalarPoint"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("ExtraPoint;1;Through;2000000000"), base::DeckError);
    const Model tolerant = readBulk("ScalarPoint;1;Through;2000000000", false);
    REQUIRE(tolerant.diagnostics().hasErrors());
    REQUIRE(tolerant.cardCount() == 1); // kept as text
}

TEST_CASE("an SPOINT or EPOINT range at the ends of the ID range is counted and expanded without overflow",
          "[model][cards][robustness]")
{
    // The last ID 2147483647: a 32-bit counter would wrap after it and never stop.
    const Model top = readBulk("ScalarPoint;2147483640;Through;2147483647\nExtraPoint;2147483630;Through;2147483637");
    REQUIRE(top.cardCount() == 16);
    REQUIRE(top.locate(base::IdSpace::Node, 2147483647).has_value());
    // A range from the lowest ID to the highest is refused by its count, computed without overflow.
    const Model whole = readBulk("ScalarPoint;-2147483648;Through;2147483647", false);
    REQUIRE(whole.diagnostics().hasErrors());
    REQUIRE(whole.cardCount() == 1);
}

TEST_CASE("every scalar point is written as a card of its own and reads back the same", "[model][cards]")
{
    const Model model = readBulk("ScalarPoint;5010;Through;5014\nExtraPoint;5110;Through;5112");

    oor::testing::requireModelFileRoundTrip(model);
    const Model again = readBulk(writeModelText(model));
    REQUIRE(idsOf<ScalarPointTable>(again) == idsOf<ScalarPointTable>(model));
    REQUIRE(idsOf<ExtraPointTable>(again) == idsOf<ExtraPointTable>(model));
}

TEST_CASE("a field that takes a grid point or a scalar point may name a scalar point no card defines", "[model][cards]")
{
    Model model = readBulk(
        "Node;2;;0.;0.;0.\nScalarPoint;14\nSpringDofProperty;1;9;14;;2;1\nSpringScalarProperty;2;9;30;31\nSpringDofInline;3;5.;14;;15");
    // Only the properties are missing (PID 9 of CELAS1 and CELAS3): the scalar points 14 (defined), 30, 31 and 15 (not)
    // are not reported, and neither is the grid point 2.
    REQUIRE(unresolvedIn(model) == 2);
    REQUIRE(mayPointAtNothing(ReferenceTarget::ScalarOrGridPoint));
    REQUIRE(mayPointAtNothing(ReferenceTarget::NodeInRange));
    REQUIRE_FALSE(mayPointAtNothing(ReferenceTarget::Node));
    // A grid point field is still reported when it is missing.
    Model cbar = readBulk("Bar2;3;5;1;2;0.;;0.;1.\nScalarPoint;1");
    REQUIRE(unresolvedIn(cbar) == 1 + 1); // PID 5 and GB 2; GA 1 is found (a scalar point is in the node space)
}

TEST_CASE("renumbering the nodes renumbers the scalar points, the other points and every reference to them",
          "[model][cards]")
{
    Model model = readBulk(
        "Node;1;;0.;0.;0.\n"
        "Node;2;;1.;0.;0.\n"
        "ScalarPoint;14;Through;16\n"
        "ExtraPoint;20\n"
        "GeomPoint;50;;1.;2.;3.\n"
        "AxisymFluidRingNode;60;0.;;;20\n"
        "SpringDofProperty;1;9;14;;2;1\n"
        "SpringScalarInline;2;5.;15;16\n"
        "SpringScalarProperty;3;9;30;31\n"
        "Bar2;4;5;1;2;0.;;0.;1.\n"
        "Beam2;5;5;1;2;0.;;0.;1.;OffsetsGlobalGlobalGlobal;;;;;;;;;15;16");
    // Two grid points, three scalar points, an extra point, a POINT and a GRIDB.
    REQUIRE(Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100)) == 2 + 3 + 1 + 1 + 1);
    REQUIRE(idsOf<NodeTable>(model) == std::vector<base::EntityId>{101, 102});
    REQUIRE(idsOf<ScalarPointTable>(model) == std::vector<base::EntityId>{114, 115, 116});
    REQUIRE(idsOf<ExtraPointTable>(model) == std::vector<base::EntityId>{120});
    REQUIRE(idsOf<GeomPointTable>(model) == std::vector<base::EntityId>{150});
    REQUIRE(idsOf<AxisymFluidRingNodeTable>(model) == std::vector<base::EntityId>{160});
    REQUIRE(model.locate(base::IdSpace::Node, 115)->table == typeIndexOf<ScalarPointTable>());
    REQUIRE_FALSE(model.locate(base::IdSpace::Node, 15).has_value());
    const SpringDofPropertyCard spring = cardAt<SpringDofPropertyTable>(model);
    REQUIRE(spring.node1 == 114);
    REQUIRE(spring.node2 == 102);
    const SpringScalarInlineCard scalar = cardAt<SpringScalarInlineTable>(model);
    REQUIRE(scalar.scalarPoint1 == 115);
    REQUIRE(scalar.scalarPoint2 == 116);
    // A scalar point no card defines follows the mapping too, like any other reference to the node space.
    REQUIRE(cardAt<SpringScalarPropertyTable>(model).scalarPoint1 == 130);
    REQUIRE(cardAt<SpringScalarPropertyTable>(model).scalarPoint2 == 131);
    REQUIRE(cardAt<Bar2Table>(model).nodeA == 101);
    const Beam2Card beam = cardAt<Beam2Table>(model);
    REQUIRE(beam.warpingPointA == 115);
    REQUIRE(beam.warpingPointB == 116);
}

TEST_CASE("renumbering the nodes in file order treats scalar points and grid points as one space", "[model][cards]")
{
    Model model = readBulk("Node;1;;0.;0.;0.\nScalarPoint;14;Through;15\nNode;2;;1.;0.;0.\nSpringDofProperty;1;9;14;;2;1");
    REQUIRE(Renumbering::apply(model, RenumberSpace::Node, Renumbering::consecutive(RenumberSpace::Node, 1000, 10)) ==
            4);
    REQUIRE(idsOf<NodeTable>(model) == std::vector<base::EntityId>{1000, 1030});
    REQUIRE(idsOf<ScalarPointTable>(model) == std::vector<base::EntityId>{1010, 1020});
    REQUIRE(cardAt<SpringDofPropertyTable>(model).node1 == 1010);
    REQUIRE(cardAt<SpringDofPropertyTable>(model).node2 == 1030);
}

TEST_CASE("a renumbering that gives a scalar point the ID of a grid point changes nothing", "[model][cards]")
{
    Model model = readBulk("Node;1;;0.;0.;0.\nScalarPoint;14\nSpringDofProperty;1;9;14;;1;1");

    const IdMapping toOne = [](base::EntityId id) -> std::optional<base::EntityId>
    { return id == 14 ? std::optional<base::EntityId>(1) : std::nullopt; };
    REQUIRE_THROWS_AS(Renumbering::apply(model, RenumberSpace::Node, Renumbering::fromFunction(toOne)),
                      base::DuplicateIdError);
    oor::testing::requireModelFileRoundTrip(model);
}

TEST_CASE("GRDSET gives CP, CD, PS and SEID to the grid points that leave them blank, wherever it is in the deck",
          "[model][cards]")
{
    const std::string grids =
        "Node;1;;0.;0.;0.\nNode;2;5;1.;0.;0.;6;123;9\nNode;3;0;0.;1.;0.;0;0;0\nNode;4;;0.;0.;1.\n";
    for (const std::string& deck : {grids + "NodeDefaults;14;28;2456;6", "NodeDefaults;14;28;2456;6\n" + grids})
    {
        const Model model = readBulk(deck);
        const NodeTable& table = *model.findTable<NodeTable>();
        // The blank fields take the defaults.
        for (const std::uint32_t row : {0U, 3U})
        {
            const NodeCard grid = table.card(row);
            REQUIRE(grid.positionFrameId == 14);
            REQUIRE(grid.displacementFrameId == 28);
            REQUIRE(grid.permanentConstraints == 2456);
            REQUIRE(grid.superelementId == 6);
        }
        // Fields that are given stay, and so does a field that is zero (the format reference: zero overrides the
        // default).
        const NodeCard given = table.card(1);
        REQUIRE(given.positionFrameId == 5);
        REQUIRE(given.displacementFrameId == 6);
        REQUIRE(given.permanentConstraints == 123);
        REQUIRE(given.superelementId == 9);
        const NodeCard zeros = table.card(2);
        REQUIRE(zeros.positionFrameId == 0);
        REQUIRE(zeros.displacementFrameId == 0);
        REQUIRE(zeros.permanentConstraints == 0);
        REQUIRE(zeros.superelementId == 0);
        // The grids no longer remember what was blank.
        for (std::uint32_t row = 0; row < 4; ++row)
            REQUIRE(table.card(row).blankFields == 0);
    }
}

TEST_CASE("GRDSET applies to each field on its own", "[model][cards]")
{
    const Model model = readBulk("NodeDefaults;;;6\nNode;1;3;0.;0.;0.\nNode;2;;0.;0.;0.;4\nNode;3;;0.;0.;0.;;5");
    const NodeTable& table = *model.findTable<NodeTable>();
    REQUIRE(table.card(0).positionFrameId == 3);
    REQUIRE(table.card(0).permanentConstraints == 6);
    REQUIRE(table.card(1).displacementFrameId == 4);
    REQUIRE(table.card(1).permanentConstraints == 6);
    REQUIRE(table.card(2).permanentConstraints == 5);
    REQUIRE(table.card(2).positionFrameId == 0);
}

TEST_CASE("without a GRDSET the blank fields of a grid point are the zeros of the format reference", "[model][cards]")
{
    const Model model = readBulk("Node;1;;0.;0.;0.");
    const NodeCard grid = cardAt<NodeTable>(model);
    REQUIRE(grid.positionFrameId == 0);
    REQUIRE(grid.permanentConstraints == 0);
    // The blank fields stay noted, for a GRDSET that is added later (Model::applyDefaults).
    REQUIRE(grid.blankFields == (NodeCard::BLANK_POSITION_FRAME | NodeCard::BLANK_DISPLACEMENT_FRAME |
                                 NodeCard::BLANK_PERMANENT_CONSTRAINTS | NodeCard::BLANK_SUPERELEMENT));
}

TEST_CASE("a model written after GRDSET was applied reads back the same grid points", "[model][cards]")
{
    const Model model = readBulk("Node;1;;0.;0.;0.\nNodeDefaults;14;28;2456;6");

    // The grid now says what it got, and the GRDSET is still a card of the model.
    oor::testing::requireModelFileRoundTrip(model);
    const Model again = readBulk(writeModelText(model));
}

TEST_CASE("with a GRDSET in the model the grids write their zeros, so the zero still overrides the default",
          "[model][cards]")
{
    const Model model = readBulk("Node;3;0;0.;1.;0.;0;0;0\nNodeDefaults;14;28;2456;6");

    oor::testing::requireModelFileRoundTrip(model);
    const Model again = readBulk(writeModelText(model));
    const NodeCard grid = cardAt<NodeTable>(again);
    REQUIRE(grid.positionFrameId == 0);
    REQUIRE(grid.displacementFrameId == 0);
    REQUIRE(grid.permanentConstraints == 0);
    REQUIRE(grid.superelementId == 0);
    // Without a GRDSET the same grid is written with blanks.
    const Model alone = readBulk("Node;3;0;0.;1.;0.;0;0;0");
    oor::testing::requireModelFileRoundTrip(alone);
}

TEST_CASE("with a BAROR in the model the bars write their zeros and their OFFT, so the defaults do not apply again",
          "[model][cards]")
{
    const Model model = readBulk(
        "Bar2;2;5;1;2;0.;;0.;1.;OffsetsGlobalGlobalGlobal\nBar2Defaults;47;0.4;;3.1;-4.65;"
        "OffsetsGlobalOrientGlobal");

    oor::testing::requireModelFileRoundTrip(model);
    const Model again = readBulk(writeModelText(model));
    const Bar2Card bar = cardAt<Bar2Table>(again);
    REQUIRE(bar.propId == 5);
    REQUIRE(bar.orientation == std::array<double, 3>{0.0, 0.0, 1.0});
    REQUIRE(bar.offsetKind == "OffsetsGlobalGlobalGlobal");
}

TEST_CASE("a grid point added after the read does not get the GRDSET defaults", "[model][cards]")
{
    Model model = readBulk("Node;1;;0.;0.;0.\nNodeDefaults;16");
    NodeCard added;
    added.id = 2;
    model.add<NodeTable>(added);
    REQUIRE(model.findTable<NodeTable>()->card(0).positionFrameId == 16);
    REQUIRE(model.findTable<NodeTable>()->card(1).positionFrameId == 0);
}

TEST_CASE("GRDSET is one card, all of whose fields are blank or in the format's ranges", "[model][cards]")
{
    REQUIRE_THROWS_AS(readBulk("NodeDefaults"), base::DeckError); // at least one field is needed
    REQUIRE_THROWS_AS(readBulk("NodeDefaults;-1"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("NodeDefaults;;-2"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("NodeDefaults;;;17"), base::DeckError);
    REQUIRE_NOTHROW(readBulk("NodeDefaults;;-1")); // CD may be -1
    // A second GRDSET is an error; in tolerant mode the first one is used.
    REQUIRE_THROWS_AS(readBulk("NodeDefaults;16\nNodeDefaults;17"), base::DeckError);
    const Model tolerant = readBulk("Node;1;;0.;0.;0.\nNodeDefaults;16\nNodeDefaults;17", false);
    REQUIRE(tolerant.diagnostics().hasErrors());
    REQUIRE(tolerant.diagnostics().firstError()->code == "deck.duplicate-node-defaults");
    REQUIRE(cardAt<NodeTable>(tolerant).positionFrameId == 16);
}

TEST_CASE("each part has its own GRDSET", "[model][cards]")
{
    const Model model = readModelText(
        "[model]\nNode;1;;0.;0.;0.\nNodeDefaults;16\n[model;SuperelementPart;5]\nNode;1;;7.;0.;0.\nNodeDefaults;17\n",
        true);
    REQUIRE(cardAt<NodeTable>(model).positionFrameId == 16);
    REQUIRE(model.parts().size() == 1);
    REQUIRE(cardAt<NodeTable>(*model.parts().front()).positionFrameId == 17);
}

TEST_CASE("the coordinate systems of a GRDSET and of the grids that took them are renumbered together",
          "[model][cards]")
{
    Model model = readBulk(
        "RectFrame2;14;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "RectFrame2;28;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "Node;1;;0.;0.;0.\n"
        "NodeDefaults;14;28");
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(100));
    REQUIRE(cardAt<NodeDefaultsTable>(model).positionFrameId == 114);
    REQUIRE(cardAt<NodeDefaultsTable>(model).displacementFrameId == 128);
    REQUIRE(cardAt<NodeTable>(model).positionFrameId == 114);
    REQUIRE(cardAt<NodeTable>(model).displacementFrameId == 128);
    REQUIRE(unresolvedIn(model) == 0);
}

TEST_CASE("BAROR gives the PID, the orientation and OFFT to the CBARs that leave them blank, wherever it is",
          "[model][cards]")
{
    const std::string bars =
        "Bar2;1;;1;2\nBar2;2;5;1;2;0.;;0.;1.;OffsetsBasicGlobalGlobal\nBar2;3;;1;2;;7\nBar2;4;;1;2;9.\n";
    for (const std::string& deck : {bars + "Bar2Defaults;47;0.4;;3.1;-4.65;OffsetsGlobalOrientGlobal",
                                    "Bar2Defaults;47;0.4;;3.1;-4.65;OffsetsGlobalOrientGlobal\n" + bars})
    {
        const Model model = readBulk(deck);
        const Bar2Table& table = *model.findTable<Bar2Table>();
        const Bar2Card blank = table.card(0);
        REQUIRE(blank.propId == 47);
        REQUIRE(blank.orientation == std::array<double, 3>{0.4, 3.1, -4.65});
        REQUIRE(blank.orientationNode == 0);
        REQUIRE(blank.offsetKind == "OffsetsGlobalOrientGlobal");
        // What the card gives itself stays.
        const Bar2Card own = table.card(1);
        REQUIRE(own.propId == 5);
        REQUIRE(own.orientation == std::array<double, 3>{0.0, 0.0, 1.0});
        REQUIRE(own.offsetKind == "OffsetsBasicGlobalGlobal");
        // A grid point of the card's own is not replaced by the vector of the BAROR.
        const Bar2Card byGrid = table.card(2);
        REQUIRE(byGrid.propId == 47);
        REQUIRE(byGrid.orientationNode == 7);
        REQUIRE(byGrid.orientation == std::array<double, 3>{});
        // X1 given, X2 and X3 blank: each blank field takes its own default.
        const Bar2Card partial = table.card(3);
        REQUIRE(partial.orientation == std::array<double, 3>{9.0, 3.1, -4.65});
        for (std::uint32_t row = 0; row < 4; ++row)
            REQUIRE(table.card(row).blankFields == 0);
    }
}

TEST_CASE("BAROR can give a grid point as the default orientation", "[model][cards]")
{
    const Model model = readBulk(
        "Bar2;1;5;1;2\nBar2;2;5;1;2;0.;;0.;1.\nBar2;3;5;1;2;4.\nBar2Defaults;;;105;;;"
        "OffsetsGlobalOrientGlobal");
    const Bar2Table& table = *model.findTable<Bar2Table>();
    REQUIRE(table.card(0).orientationNode == 105);
    REQUIRE(table.card(0).orientation == std::array<double, 3>{});
    REQUIRE(table.card(0).offsetKind == "OffsetsGlobalOrientGlobal");
    REQUIRE(table.card(1).orientationNode == 0);
    REQUIRE(table.card(1).orientation == std::array<double, 3>{0.0, 0.0, 1.0});
    REQUIRE(table.card(1).offsetKind == "OffsetsGlobalOrientGlobal"); // OFFT was blank too
    REQUIRE(table.card(2).orientationNode == 0);
    REQUIRE(table.card(2).orientation == std::array<double, 3>{4.0, 0.0, 0.0});
    REQUIRE(cardAt<Bar2DefaultsTable>(model).orientationNode == 105);
}

TEST_CASE("without a BAROR a blank PID is the EID and a blank orientation is the zero vector", "[model][cards]")
{
    const Model model = readBulk("Bar2;8;;1;2");
    const Bar2Card bar = cardAt<Bar2Table>(model);
    REQUIRE(bar.propId == 8);
    REQUIRE(bar.orientation == std::array<double, 3>{});
    REQUIRE(bar.offsetKind == "OffsetsGlobalGlobalGlobal");
    // A BAROR with no PID leaves a blank PID the EID.
    const Model noPid = readBulk("Bar2;8;;1;2\nBar2Defaults;;0.;;0.;1.;OffsetsBasicOrientOrient");
    REQUIRE(cardAt<Bar2Table>(noPid).propId == 8);
    REQUIRE(cardAt<Bar2Table>(noPid).orientation == std::array<double, 3>{0.0, 0.0, 1.0});
    REQUIRE(cardAt<Bar2Table>(noPid).offsetKind == "OffsetsBasicOrientOrient");
}

TEST_CASE("BAROR is one card with a valid PID, orientation and OFFT", "[model][cards]")
{
    REQUIRE_THROWS_AS(readBulk("Bar2Defaults;-3"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("Bar2Defaults;;;1;;;XXX"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("Bar2Defaults;;;0;;;OffsetsGlobalGlobalGlobal"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("Bar2Defaults;5\nBar2Defaults;6"), base::DeckError);
    const Model tolerant = readBulk("Bar2;1;;1;2\nBar2Defaults;5\nBar2Defaults;6", false);
    REQUIRE(tolerant.diagnostics().firstError()->code == "deck.duplicate-bar-elem-defaults");
    REQUIRE(cardAt<Bar2Table>(tolerant).propId == 5);
}

TEST_CASE("BEAMOR does for CBEAM what BAROR does for CBAR", "[model][cards]")
{
    const Model model = readBulk(
        "Beam2;1;;1;2\n"
        "Beam2;2;5;1;2;0.;;0.;1.;OffsetsBasicGlobalGlobal\n"
        "Beam2;3;;1;2;;7;;;;;;;;;;;;15;16\n"
        "Beam2Defaults;47;0.4;;3.1;-4.65;OffsetsGlobalOrientGlobal");
    const Beam2Table& table = *model.findTable<Beam2Table>();
    REQUIRE(table.card(0).propId == 47);
    REQUIRE(table.card(0).orientation == std::array<double, 3>{0.4, 3.1, -4.65});
    REQUIRE(table.card(0).offsetKind == "OffsetsGlobalOrientGlobal");
    REQUIRE(table.card(1).propId == 5);
    REQUIRE(table.card(1).offsetKind == "OffsetsBasicGlobalGlobal");
    REQUIRE(table.card(2).orientationNode == 7);
    REQUIRE(table.card(2).propId == 47);
    REQUIRE(table.card(2).warpingPointA == 15);
    REQUIRE(table.card(2).warpingPointB == 16);
    REQUIRE(table.card(0).blankFields == 0);
    REQUIRE_THROWS_AS(readBulk("Beam2Defaults;5\nBeam2Defaults;6"), base::DeckError);
}

TEST_CASE("the cards read with BAROR defaults are written with them resolved, so they read back the same",
          "[model][cards]")
{
    const Model model = readBulk("Bar2;1;;1;2\nBar2Defaults;47;0.4;;3.1;-4.65;OffsetsGlobalOrientGlobal");

    oor::testing::requireModelFileRoundTrip(model);
    const Model again = readBulk(writeModelText(model));
}

TEST_CASE("CORD3G is a coordinate system of the coordinate ID space and refers to its reference system",
          "[model][cards]")
{
    // The angles are equations (DEQATN): the six numbers name the equation cards that follow.
    Model model = readBulk(
        "RectFrame2;10;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "GeneralFrame3;20;EulerAngles313;EquationForm;120;121;122;10\n"
        "GeneralFrame3;21;;;1;2;3\n"
        "DesignEquation;120;F(X)=X\n"
        "DesignEquation;121;F(X)=2.*X\n"
        "DesignEquation;122;F(X)=3.*X\n"
        "DesignEquation;1;F(X)=X\n"
        "DesignEquation;2;F(X)=2.*X\n"
        "DesignEquation;3;F(X)=3.*X");
    const GeneralFrame3Card first = cardAt<GeneralFrame3Table>(model, 0);
    REQUIRE(first.methodName == "EulerAngles313");
    REQUIRE(first.matrixForm == "EquationForm");
    REQUIRE(first.angleFunctionIds == std::array<base::EntityId, 3>{120, 121, 122});
    REQUIRE(first.referenceFrameId == 10);
    const GeneralFrame3Card defaults = cardAt<GeneralFrame3Table>(model, 1);
    REQUIRE(defaults.methodName == "EulerAngles313");
    REQUIRE(defaults.matrixForm == "EquationForm");
    REQUIRE(defaults.referenceFrameId == 0);
    REQUIRE(model.locate(base::IdSpace::Coordinate, 20)->table == typeIndexOf<GeneralFrame3Table>());
    REQUIRE_THROWS_AS(readBulk("RectFrame2;20\nGeneralFrame3;20;;;1;2;3"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("GeneralFrame3;20;EulerAngles313;FILE;1;2;3"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("GeneralFrame3;20;;;1;2;3;20"), base::DeckError); // not with respect to itself
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(100));
    REQUIRE(cardAt<GeneralFrame3Table>(model, 0).id == 120);
    REQUIRE(cardAt<GeneralFrame3Table>(model, 0).referenceFrameId == 110);
    REQUIRE(cardAt<GeneralFrame3Table>(model, 0).angleFunctionIds ==
            std::array<base::EntityId, 3>{120, 121, 122}); // not coordinates
    // They are renumbered with the equations.
    Renumbering::apply(model, RenumberSpace::DesignEquation, Renumbering::offset(1000));
    REQUIRE(cardAt<GeneralFrame3Table>(model, 0).angleFunctionIds == std::array<base::EntityId, 3>{1120, 1121, 1122});
    REQUIRE(cardAt<GeneralFrame3Table>(model, 1).angleFunctionIds == std::array<base::EntityId, 3>{1001, 1002, 1003});
    REQUIRE(unresolvedIn(model) == 0);
}

TEST_CASE("POINT and GRIDB read their own fields", "[model][cards]")
{
    const Model model = readBulk("GeomPoint;12;3;1.;2.;5.\nAxisymFluidRingNode;30;40.;4;246;25");
    const GeomPointCard point = cardAt<GeomPointTable>(model);
    REQUIRE(point.id == 12);
    REQUIRE(point.positionFrameId == 3);
    REQUIRE(point.xyz.z == 5.0);
    const AxisymFluidRingNodeCard ring = cardAt<AxisymFluidRingNodeTable>(model);
    REQUIRE(ring.phiAngle == 40.0);
    REQUIRE(ring.displacementFrameId == 4);
    REQUIRE(ring.permanentConstraints == 246);
    REQUIRE(ring.fluidPointId == 25);
    REQUIRE_THROWS_AS(readBulk("Node;12;;0.;0.;0.\nGeomPoint;12"), base::DeckError); // the node ID space
}
