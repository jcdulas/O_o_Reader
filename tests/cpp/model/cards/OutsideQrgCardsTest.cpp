// The cards that are not in the format reference list: PLOTEL3 and PLOTEL4 (plot-only triangles and quadrilaterals),
// PPLANE (the plane strain and stress property of another implementation of the format), PCOMPS (its layered solid
// property) and CGEN (the generation of elements in a field of GRIDG points). Per card: the fields of the deck, the
// defaults, the refused values with their field, the references the card lists and the renumbering of the spaces it
// takes part in. The generic tests of CardRoundTripTest.cpp cover reading, writing and copying every sample in every
// format.

#include "oor/model/cards/elements/elemGeneration/ElemGenerationTable.hpp"
#include "oor/model/cards/elements/plotQuad4/PlotQuad4Table.hpp"
#include "oor/model/cards/elements/plotTria3/PlotTria3Table.hpp"
#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/cards/elements/shellTria3/ShellTria3Table.hpp"
#include "oor/model/cards/properties/compSolidProp/CompSolidPropTable.hpp"
#include "oor/model/cards/properties/planeProp/PlanePropTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The grid points 1 to 4, which the plot elements of these tests name.
const std::string GRIDS = "Node;1;;0.;0.;0.\nNode;2;;1.;0.;0.\nNode;3;;1.;1.;0.\nNode;4;;0.;1.;0.\n";

// A PCOMPS with SB and NB blank, TREF 20., GE 0., a ply with a failure theory PFA, YES for the stress output and the
// word ABS in the last field of the ply.
const std::string FIXED_COMP_SOLID_PROP = "CompSolidProp;2;0;23;;;20.;0.;1;1;0.5;0.;ProgressiveFailure;;Yes;Absolute\n";

} // namespace

// ---------------------------------------------------------------------------------------------------------------------
// PLOTEL3 and PLOTEL4
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a PLOTEL3 is a triangle between three grid points and has an ID space of its own", "[model][cards][outside]")
{
    // A plot triangle, then an ID that a CTRIA3 has too: a plot element never replaces a structural one.
    Model model = readBulk("PlotTria3;10;1847;6253;18041\nShellTria3;10;1;1;2;3\nPlotTria3;11;1;2;3");
    REQUIRE(rowCount<PlotTria3Table>(model) == 2);
    REQUIRE(rowCount<ShellTria3Table>(model) == 1);
    const PlotTria3Card card = cardAt<PlotTria3Table>(model, 0);
    REQUIRE((card.id == 10 && card.node1 == 1847 && card.node2 == 6253 && card.node3 == 18041));
    REQUIRE((PlotTria3Table::INFO.category == CardCategory::Element &&
             PlotTria3Table::INFO.idSpace == base::IdSpace::PerCardType));
    requireRoundTrip(model);

    // Two PLOTEL3 cards cannot have the same ID.
    REQUIRE_THROWS_AS(readBulk("PlotTria3;1;1;2;3\nPlotTria3;1;2;3;4"), base::DeckError);
}

TEST_CASE("a PLOTEL3 refuses repeated points, missing fields and a second element", "[model][cards][outside]")
{
    expectInvalid("PlotTria3;0;1;2;3", "id");
    expectInvalid("PlotTria3;1;0;2;3", "node1");
    expectInvalid("PlotTria3;1;1;1;3", "node2");
    expectInvalid("PlotTria3;1;1;2;1", "node3");
    expectInvalid("PlotTria3;1;1;2;2", "node3");
    expectInvalid("PlotTria3;1;1;2", "node3");
    // The layout of the decks has four fields: a fifth one (a second element, or a fourth point) is not understood.
}

TEST_CASE("a PLOTEL3 names three grid points, which a renumbering of the nodes follows", "[model][cards][outside]")
{
    Model model = readBulk(GRIDS + "PlotTria3;10;1;2;3\nPlotTria3;11;2;3;99");
    RecordingVisitor visitor;
    model.table<PlotTria3Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"node1", "node2", "node3"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(3, ReferenceTarget::Node));
    // The point 99 is no grid point.
    REQUIRE(unresolvedIn(model) == 1);

    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    const PlotTria3Card card = cardAt<PlotTria3Table>(model, 0);
    REQUIRE((card.node1 == 101 && card.node2 == 102 && card.node3 == 103));
    // The plot element's own ID is not in the element space.
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(1000));
    REQUIRE(cardAt<PlotTria3Table>(model, 0).id == 10);
}

TEST_CASE("a PLOTEL4 is a quadrilateral between four grid points and has an ID space of its own",
          "[model][cards][outside]")
{
    Model model = readBulk("PlotQuad4;20;1847;6253;18041;9175\nShellQuad4;20;1;1;2;3;4\nPlotQuad4;21;1;2;3;4");
    REQUIRE(rowCount<PlotQuad4Table>(model) == 2);
    REQUIRE(rowCount<ShellQuad4Table>(model) == 1);
    const PlotQuad4Card card = cardAt<PlotQuad4Table>(model, 0);
    REQUIRE((card.id == 20 && card.node1 == 1847 && card.node2 == 6253 && card.node3 == 18041 && card.node4 == 9175));
    REQUIRE((PlotQuad4Table::INFO.category == CardCategory::Element &&
             PlotQuad4Table::INFO.idSpace == base::IdSpace::PerCardType));
    requireRoundTrip(model);

    REQUIRE_THROWS_AS(readBulk("PlotQuad4;1;1;2;3;4\nPlotQuad4;1;2;3;4;5"), base::DeckError);
}

TEST_CASE("a PLOTEL4 refuses repeated points, missing fields and a second element", "[model][cards][outside]")
{
    expectInvalid("PlotQuad4;0;1;2;3;4", "id");
    expectInvalid("PlotQuad4;1;1;2;3;0", "node1");
    expectInvalid("PlotQuad4;1;1;1;3;4", "node2");
    expectInvalid("PlotQuad4;1;1;2;1;4", "node3");
    expectInvalid("PlotQuad4;1;1;2;3;2", "node4");
    // A triangle written on a PLOTEL4 is a PLOTEL3.
    expectInvalid("PlotQuad4;1;1;2;3;3", "node4");
    expectInvalid("PlotQuad4;1;1;2;3", "node4");
}

TEST_CASE("a PLOTEL4 names four grid points, which a renumbering of the nodes follows", "[model][cards][outside]")
{
    Model model = readBulk(GRIDS + "PlotQuad4;20;1;2;3;4\nPlotQuad4;21;1;2;3;99");
    RecordingVisitor visitor;
    model.table<PlotQuad4Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"node1", "node2", "node3", "node4"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(4, ReferenceTarget::Node));
    REQUIRE(unresolvedIn(model) == 1);

    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(10));
    const PlotQuad4Card card = cardAt<PlotQuad4Table>(model, 0);
    REQUIRE((card.node1 == 11 && card.node2 == 12 && card.node3 == 13 && card.node4 == 14));
}

// ---------------------------------------------------------------------------------------------------------------------
// PPLANE
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a PPLANE has the fields of another implementation of the format and keeps a blank thickness blank",
          "[model][cards][outside]")
{
    Model model = readBulk("PlaneProp;3;1;1.\nPlaneProp;4;1;0.25;0.01;2\nPlaneProp;5;1");
    // A thickness given: PPLANE 3 1 1.
    const PlanePropCard deck = cardAt<PlanePropTable>(model, 0);
    REQUIRE((deck.id == 3 && deck.matId == 1 && deck.thickness && *deck.thickness == 1.0));
    REQUIRE((deck.nonStructMass == 0.0 && deck.formulationOption == 0));
    const PlanePropCard full = cardAt<PlanePropTable>(model, 1);
    REQUIRE((full.thickness && *full.thickness == 0.25 && full.nonStructMass == 0.01 && full.formulationOption == 2));
    // No source gives a default for T: a blank one stays blank, and is written blank.
    const PlanePropCard bare = cardAt<PlanePropTable>(model, 2);
    REQUIRE((bare.id == 5 && !bare.thickness.has_value()));

    REQUIRE((PlanePropTable::INFO.category == CardCategory::Property &&
             PlanePropTable::INFO.idSpace == base::IdSpace::Property));
    requireRoundTrip(model);
    // A property: its number is shared with the other properties.
    REQUIRE_THROWS_AS(readBulk("PlaneProp;3;1;1.\nShellProp;3;1;0.1"), base::DeckError);
}

TEST_CASE("a PPLANE refuses what it does not know", "[model][cards][outside]")
{
    expectInvalid("PlaneProp;0;1;1.", "id");
    expectInvalid("PlaneProp;3;0;1.", "matId");
    expectInvalid("PlaneProp;3", "matId");
    expectInvalid("PlaneProp;3;1;-1.", "thickness");
    expectInvalid("PlaneProp;3;1;1.;;-1", "formulationOption");
    expectInvalid("PlaneProp;3;1;x", "thickness");
}

TEST_CASE("a PPLANE names a material, which a renumbering of the materials and of the properties follows",
          "[model][cards][outside]")
{
    Model model = readBulk("IsoMat;1;3.E+7;;0.3\nPlaneProp;3;1;1.\nPlaneProp;4;2;1.");
    RecordingVisitor visitor;
    model.table<PlanePropTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material});
    // The material 2 is missing.
    REQUIRE(unresolvedIn(model) == 1);

    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(100));
    REQUIRE((cardAt<PlanePropTable>(model, 0).matId == 101 && cardAt<PlanePropTable>(model, 1).matId == 102));
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE((cardAt<PlanePropTable>(model, 0).id == 1003 && cardAt<PlanePropTable>(model, 1).id == 1004));
}

// ---------------------------------------------------------------------------------------------------------------------
// PCOMPS
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a PCOMPS reads its plies, the last field of a ply included", "[model][cards][outside]")
{
    Model model = readBulk(FIXED_COMP_SOLID_PROP +
                           "CompSolidProp;7;0;13;;;20.;0.;1;1;5.;0.;;;Yes;Absolute;2;1;5.;0.;;;Yes;Absolute\n"
                           "CompSolidProp;8;5;12;100.;200.;20.;0.01;1;11;0.5;30.;Hill;MaximumShearStress;Yes;;2;12;0."
                           "25;-30.;TsaiWu;;No;Absolute");
    // PSDIR 23, TREF 20., one ply with the failure theory PFA.
    const CompSolidPropCard deck = cardAt<CompSolidPropTable>(model, 0);
    REQUIRE((deck.id == 2 && deck.matFrameId == 0 && deck.stackingDirection == 23 &&
             deck.referenceTemperature == 20.0 && deck.structuralDamping == 0.0));
    REQUIRE((!deck.bondAllowable.has_value() && !deck.normalBondAllowable.has_value()));
    REQUIRE(deck.plies.size() == 1);
    const CompSolidPropPly& ply = deck.plies[0];
    REQUIRE((ply.globalId == 1 && ply.matId == 1 && ply.thickness == 0.5 && ply.thetaInDegrees == 0.0));
    REQUIRE((ply.failureTheory == "ProgressiveFailure" && ply.interlaminarTheory.empty() &&
             ply.output == CompSolidPropOutput::Yes));
    REQUIRE(ply.tail == "Absolute");

    // Two plies of the same material.
    const CompSolidPropCard two = cardAt<CompSolidPropTable>(model, 1);
    REQUIRE((two.stackingDirection == 13 && two.plies.size() == 2 && two.totalThickness() == 10.0));
    REQUIRE((two.plies[1].globalId == 2 && two.plies[1].failureTheory.empty()));

    // Every field given; a word is kept in upper case.
    const CompSolidPropCard full = cardAt<CompSolidPropTable>(model, 2);
    REQUIRE((full.matFrameId == 5 && full.stackingDirection == 12 && full.bondAllowable &&
             *full.bondAllowable == 100.0 && full.normalBondAllowable && *full.normalBondAllowable == 200.0));
    REQUIRE((full.referenceTemperature == 20.0 && full.structuralDamping == 0.01));
    REQUIRE((full.plies[0].thetaInDegrees == 30.0 && full.plies[0].interlaminarTheory == "MaximumShearStress"));
    REQUIRE((full.plies[0].tail.empty() && full.plies[1].failureTheory == "TsaiWu"));
    REQUIRE((full.plies[1].output == CompSolidPropOutput::No && full.plies[1].tail == "Absolute"));
    REQUIRE((CompSolidPropTable::INFO.category == CardCategory::Property &&
             CompSolidPropTable::INFO.idSpace == base::IdSpace::Property));
    requireRoundTrip(model);
}

TEST_CASE("a PCOMPS has the defaults of its blank fields and writes THETA of every ply", "[model][cards][outside]")
{
    Model model = readBulk("CompSolidProp;7;;;;;;;1;1;5.;;;;;;2;1;5.;30.;;;No");
    const CompSolidPropCard card = cardAt<CompSolidPropTable>(model, 0);
    REQUIRE((card.matFrameId == 0 && card.stackingDirection == 13 && card.referenceTemperature == 0.0 &&
             card.structuralDamping == 0.0));
    REQUIRE((card.plies[0].thetaInDegrees == 0.0 && card.plies[0].output == CompSolidPropOutput::No));
    REQUIRE((card.plies[0].failureTheory.empty() && card.plies[0].interlaminarTheory.empty()));
    REQUIRE((card.plies[0].tail.empty() && card.plies[1].thetaInDegrees == 30.0));
    // The first line is the property alone; THETA is written for the first ply although it is the default: the line of
    // the first ply reaches the fourth field (thirty-two characters and the marker field), where THETA is, and does not
    // stop at the thickness.

    const Model again = readModelText(oor::testing::writeModelText(model), true);
    REQUIRE((cardAt<CompSolidPropTable>(again, 0).plies.size() == 2 &&
             cardAt<CompSolidPropTable>(again, 0).plies[1].thetaInDegrees == 30.0));
    requireRoundTrip(model);
}

TEST_CASE("a PCOMPS refuses what it does not know", "[model][cards][outside]")
{
    expectInvalid("CompSolidProp;0;;;;;;;1;1;0.5", "id");
    expectInvalid("CompSolidProp;2;-2;;;;;;1;1;0.5", "matFrameId");
    expectInvalid("CompSolidProp;2;;14;;;;;1;1;0.5", "stackingDirection");
    expectInvalid("CompSolidProp;2;;11;;;;;1;1;0.5", "stackingDirection");
    expectInvalid("CompSolidProp;2;;;-1.;;;;1;1;0.5", "bondAllowable");
    expectInvalid("CompSolidProp;2;;;;-1.;;;1;1;0.5", "normalBondAllowable");
    // The eighth field of the first line is blank in the layout.
    expectInvalid("CompSolidProp;2", "globalPlyId");
    expectInvalid("CompSolidProp;2;;;;;;;0;1;0.5", "globalPlyId");
    expectInvalid("CompSolidProp;2;;;;;;;1;1;0.5;;;;;;1;2;0.5", "globalPlyId");
    expectInvalid("CompSolidProp;2;;;;;;;1;0;0.5", "matId");
    expectInvalid("CompSolidProp;2;;;;;;;1;1;0.", "thickness");
    expectInvalid("CompSolidProp;2;;;;;;;1;1;-0.5", "thickness");
    expectInvalid("CompSolidProp;2;;;;;;;1;1", "thickness");
    expectInvalid("CompSolidProp;2;;;;;;;1;1;0.5;0.;;;MAYBE", "plyStressOutput");
    oor::testing::requireModelFileRoundTrip(readModelText("CompSolidProp;2;;;;;;;1;1;0.5;0.;TOOLONGWORD", true));
    oor::testing::requireModelFileRoundTrip(readModelText("CompSolidProp;2;;;;;;;1;1;0.5;0.;;;;TOOLONGWORD", true));
}

TEST_CASE("a PCOMPS names the materials of its plies and a coordinate system, which renumberings follow",
          "[model][cards][outside]")
{
    Model model = readBulk(
        "IsoMat;11;3.E+7;;0.3\n"
        "IsoMat;12;3.E+7;;0.3\n"
        "RectFrame2;5;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "CompSolidProp;8;5;12;;;20.;0.;1;11;0.5;30.;;;;;2;12;0.25;-30.\n"
        "CompSolidProp;9;-1;;;;;;1;13;1.");
    RecordingVisitor visitor;
    model.table<CompSolidPropTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId", "matId", "matFrameId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material, ReferenceTarget::Material,
                                                            ReferenceTarget::Coordinate});
    // The element system (-1) and the basic system (0) are no cards: only the material 13 is missing.
    visitor = RecordingVisitor{};
    model.table<CompSolidPropTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId"});
    REQUIRE(unresolvedIn(model) == 1);

    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(100));
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    const CompSolidPropCard card = cardAt<CompSolidPropTable>(model, 0);
    REQUIRE((card.id == 1008 && card.matFrameId == 15 && card.plies[0].matId == 111 && card.plies[1].matId == 112));
    REQUIRE((cardAt<CompSolidPropTable>(model, 1).id == 1009 && cardAt<CompSolidPropTable>(model, 1).matFrameId == -1));
}

// ---------------------------------------------------------------------------------------------------------------------
// CGEN
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a CGEN keeps its four leading fields and the fields it does not know as written", "[model][cards][outside]")
{
    // Two cards with their leading fields only (the fifth field is an integer in the first one), and a card with the
    // options in both lines.
    Model model = readBulk(
        "ElemGeneration;Quad4;120001;2;1;1\nElemGeneration;Hexa8;1001;1;1\nElemGeneration;Hexa20;5402;5400;4;L;;;;1.;"
        "2.");
    REQUIRE(rowCount<ElemGenerationTable>(model) == 3);
    const ElemGenerationCard quad = cardAt<ElemGenerationTable>(model, 0);
    REQUIRE((quad.kind == "Quad4" && quad.firstElemId == 120001 && quad.propId == 2 && quad.gridFieldId == 1));
    REQUIRE(quad.options[0] == "1");
    for (std::size_t index = 1; index < quad.options.size(); ++index)
        REQUIRE(quad.options[index].empty());
    const ElemGenerationCard hexa = cardAt<ElemGenerationTable>(model, 1);
    REQUIRE((hexa.kind == "Hexa8" && hexa.firstElemId == 1001 && hexa.propId == 1 && hexa.gridFieldId == 1));
    REQUIRE(hexa.options[0].empty());
    // The neutral type is kept; TA and TB are in the second line.
    const ElemGenerationCard full = cardAt<ElemGenerationTable>(model, 2);
    REQUIRE((full.kind == "Hexa20" && full.options[0] == "L" && full.options[4] == "1." && full.options[5] == "2."));
    REQUIRE((full.options[1].empty() && full.options[2].empty() && full.options[3].empty() && full.options[6].empty()));
    // A generator has no ID of its own, so the same first element may be asked twice: it is the deck's mistake to
    // make, not a duplicate of the model.
    REQUIRE((ElemGenerationTable::INFO.category == CardCategory::Other &&
             ElemGenerationTable::INFO.idSpace == base::IdSpace::None));
    REQUIRE(rowCount<ElemGenerationTable>(readBulk("ElemGeneration;Quad4;1;1;1\nElemGeneration;Quad4;1;1;1")) == 2);
    requireRoundTrip(model);
}

TEST_CASE("a CGEN preserves long words and refuses a missing type or number", "[model][cards][outside]")
{
    expectInvalid("ElemGeneration;;1001;1;1", "kind");
    oor::testing::requireModelFileRoundTrip(readModelText("ElemGeneration;HEXA8TOOLONG;1001;1;1", true));
    expectInvalid("ElemGeneration;Hexa8;0;1;1", "firstElemId");
    expectInvalid("ElemGeneration;Hexa8;1001;0;1", "propId");
    expectInvalid("ElemGeneration;Hexa8;1001;1", "gridFieldId");
    expectInvalid("ElemGeneration;Hexa8;1001;1;0", "gridFieldId");
    oor::testing::requireModelFileRoundTrip(readModelText("ElemGeneration;Quad4;1;1;1;TOOLONGWORD", true));
    // A thirteenth field: fields 9 to 12 are blank and field 13 is not.
}

TEST_CASE("a CGEN names the property of its elements and not its GRIDG field", "[model][cards][outside]")
{
    Model model =
        readBulk("IsoMat;1;3.E+7;;0.3\nSolidProp;1;1\nElemGeneration;Hexa8;1001;1;1\nElemGeneration;Hexa8;1002;2;1");
    RecordingVisitor visitor;
    model.table<ElemGenerationTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property});
    // The property 2 is missing; the GRIDG field 1 is a plain number that nothing looks up.
    REQUIRE(unresolvedIn(model) == 1);

    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE(
        (cardAt<ElemGenerationTable>(model, 0).propId == 1001 && cardAt<ElemGenerationTable>(model, 1).propId == 1002));
    REQUIRE(cardAt<ElemGenerationTable>(model, 0).gridFieldId == 1);
    REQUIRE(cardAt<ElemGenerationTable>(model, 0).firstElemId == 1001);
}
