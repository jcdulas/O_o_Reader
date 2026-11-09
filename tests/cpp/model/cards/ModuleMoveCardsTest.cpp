// Checks of the module repositioning cards and of the mesh adaptivity cards: MDTRAN, MDROT1,
// MDROT2, MDMIR1, MDMIR2, MDMOVE, MDMPLN, MDLOC, HADACRI and HADAPTL. Per card: the fields in their format reference
// positions, the defaults, the references it lists and what each one points to, the renumbering of the space the card
// owns, the refused values and the round trip through the three formats. The generic tests of CardRoundTripTest.cpp
// read, write and copy every sample; this file covers what each card means.

#include "oor/model/cards/nodes/adaptiveRefineControl/AdaptiveRefineControlTable.hpp"
#include "oor/model/cards/nodes/adaptiveRefineCriterion/AdaptiveRefineCriterionTable.hpp"
#include "oor/model/cards/nodes/moduleLocation/ModuleLocationTable.hpp"
#include "oor/model/cards/nodes/moduleMirror1/ModuleMirror1Table.hpp"
#include "oor/model/cards/nodes/moduleMirror2/ModuleMirror2Table.hpp"
#include "oor/model/cards/nodes/moduleMirrorPlane/ModuleMirrorPlaneTable.hpp"
#include "oor/model/cards/nodes/moduleMoveSequence/ModuleMoveSequenceTable.hpp"
#include "oor/model/cards/nodes/moduleRotation1/ModuleRotation1Table.hpp"
#include "oor/model/cards/nodes/moduleRotation2/ModuleRotation2Table.hpp"
#include "oor/model/cards/nodes/moduleTranslation/ModuleTranslationTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The points and the coordinate system that the moves of the decks name: three grid points, a POINT and a CORD2R.
const std::string POINTS_TEXT =
    "Node;1;;0.;0.;0.\nNode;2;;1.;0.;0.\nNode;3;;0.;1.;0.\nGeomPoint;4;;0.;0.;1.\nRectFrame2;5\n";

// One move of each kind, with and without the optional fields. The MVIDs are all different, as the format reference
// asks (one number space for the five kinds).
const std::string MOVES_TEXT =
    "ModuleTranslation;10;1;2\n"
    "ModuleTranslation;11;2;3;2.5\n"
    "ModuleRotation1;20;;1;2;90.\n"
    "ModuleRotation1;21;4;1;2;-30.\n"
    "ModuleRotation2;30;;;ZAxis;45.\n"
    "ModuleRotation2;31;5;3;MinusXAxis;15.5\n"
    "ModuleMirror1;40;1;2;3\n"
    "ModuleMirror2;50;;;PlaneXy\n"
    "ModuleMirror2;51;5;4;PlaneZx\n";

// An MDMOVE of nine moves: a mirror first, as the format reference asks, then the others, over two lines.
const std::string MOVE_SEQUENCE_TEXT = "ModuleMoveSequence;60;40;10;20;30;31;11;21;50;51\n";

// The IDs that the references of a card name, as a visitor sees them.
struct Visited
{
    std::vector<std::string> fields;
    std::vector<ReferenceTarget> targets;
};

template <typename TableT>
Visited visitedBy(Model& model, std::uint32_t row)
{
    RecordingVisitor visitor;
    model.table<TableT>().visitReferences(row, visitor);
    return Visited{visitor.fields, visitor.targets};
}

} // namespace

TEST_CASE("the five moves are one category, each with a number of its own per card type", "[model][cards][modulemove]")
{
    REQUIRE((ModuleTranslationTable::INFO.category == CardCategory::ModuleMove &&
             ModuleTranslationTable::INFO.idSpace == base::IdSpace::PerCardType));
    REQUIRE((ModuleRotation1Table::INFO.category == CardCategory::ModuleMove &&
             ModuleRotation1Table::INFO.idSpace == base::IdSpace::PerCardType));
    REQUIRE((ModuleRotation2Table::INFO.category == CardCategory::ModuleMove &&
             ModuleRotation2Table::INFO.idSpace == base::IdSpace::PerCardType));
    REQUIRE((ModuleMirror1Table::INFO.category == CardCategory::ModuleMove &&
             ModuleMirror1Table::INFO.idSpace == base::IdSpace::PerCardType));
    REQUIRE((ModuleMirror2Table::INFO.category == CardCategory::ModuleMove &&
             ModuleMirror2Table::INFO.idSpace == base::IdSpace::PerCardType));
    REQUIRE_THROWS_AS(readBulk("ModuleTranslation;10;1;2\nModuleTranslation;10;2;3"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("ModuleRotation1;20;;1;2;90.\nModuleRotation1;20;;2;3;90."), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("ModuleRotation2;30;;;ZAxis;45.\nModuleRotation2;30;;;XAxis;45."), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("ModuleMirror1;40;1;2;3\nModuleMirror1;40;1;2;4"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("ModuleMirror2;50;;;PlaneXy\nModuleMirror2;50;;;PlaneXz"), base::DeckError);
}

TEST_CASE("an MDTRAN is a vector between two points and a magnitude that defaults to the length of the vector",
          "[model][cards][modulemove]")
{
    Model model = readBulk(POINTS_TEXT + MOVES_TEXT);
    const ModuleTranslationCard plain = cardAt<ModuleTranslationTable>(model, 0);
    REQUIRE((plain.id == 10 && plain.startNode == 1 && plain.endNode == 2 && plain.magnitude == 0.0));
    const ModuleTranslationCard scaled = cardAt<ModuleTranslationTable>(model, 1);
    REQUIRE((scaled.id == 11 && scaled.startNode == 2 && scaled.endNode == 3 && scaled.magnitude == 2.5));
    // A zero magnitude is the default, so the card keeps it and writes the field blank.
    const Model zero = readBulk("ModuleTranslation;12;1;2;0.");
    REQUIRE(cardAt<ModuleTranslationTable>(zero).magnitude == 0.0);
    requireRoundTrip(zero);

    const Visited visited = visitedBy<ModuleTranslationTable>(model, 0);
    REQUIRE(visited.fields == std::vector<std::string>{"startNode", "endNode"});
    REQUIRE(visited.targets == std::vector<ReferenceTarget>{ReferenceTarget::Node, ReferenceTarget::Node});

    expectInvalid("ModuleTranslation;0;1;2", "id");
    expectInvalid("ModuleTranslation;-3;1;2", "id");
    expectInvalid("ModuleTranslation;10;0;2", "startNode");
    expectInvalid("ModuleTranslation;10;1", "endNode");   // required
    expectInvalid("ModuleTranslation;10;1;1", "endNode"); // a vector between a point and itself
    expectInvalid("ModuleTranslation;10;1;2;abc", "magnitude");
}

TEST_CASE("an MDROT1 turns about the vector from IDS to IDE, or about a reference point", "[model][cards][modulemove]")
{
    Model model = readBulk(POINTS_TEXT + MOVES_TEXT);
    const ModuleRotation1Card about = cardAt<ModuleRotation1Table>(model, 0);
    REQUIRE((about.id == 20 && about.referenceNode == 0 && about.startNode == 1 && about.endNode == 2 &&
             about.magnitude == 90.0));
    const ModuleRotation1Card point = cardAt<ModuleRotation1Table>(model, 1);
    REQUIRE((point.referenceNode == 4 && point.startNode == 1 && point.endNode == 2 && point.magnitude == -30.0));

    // A blank reference point is not visited; a POINT is a node.
    REQUIRE(visitedBy<ModuleRotation1Table>(model, 0).fields == std::vector<std::string>{"startNode", "endNode"});
    const Visited visited = visitedBy<ModuleRotation1Table>(model, 1);
    REQUIRE(visited.fields == std::vector<std::string>{"referenceNode", "startNode", "endNode"});
    REQUIRE(visited.targets == std::vector<ReferenceTarget>(3, ReferenceTarget::Node));

    expectInvalid("ModuleRotation1;0;;1;2;90.", "id");
    expectInvalid("ModuleRotation1;20;-1;1;2;90.", "referenceNode");
    expectInvalid("ModuleRotation1;20;;0;2;90.", "startNode");
    expectInvalid("ModuleRotation1;20;;1;2", "magnitude");    // required
    expectInvalid("ModuleRotation1;20;;1;1;90.", "endNode");  // no axis
    expectInvalid("ModuleRotation1;20;;1;2;0.", "magnitude"); // a rotation by nothing
}

TEST_CASE("an MDROT2 turns about an axis of a coordinate system, on either side of the axis",
          "[model][cards][modulemove]")
{
    Model model = readBulk(POINTS_TEXT + MOVES_TEXT);
    const ModuleRotation2Card basic = cardAt<ModuleRotation2Table>(model, 0);
    REQUIRE((basic.id == 30 && basic.frameId == 0 && basic.referenceNode == 0 && basic.axis == ModuleRotation2Axis::Z &&
             basic.magnitude == 45.0));
    const ModuleRotation2Card negative = cardAt<ModuleRotation2Table>(model, 1);
    REQUIRE((negative.frameId == 5 && negative.referenceNode == 3 && negative.axis == ModuleRotation2Axis::MinusX &&
             negative.magnitude == 15.5));
    for (const auto& [word, axis] :
         std::vector<std::pair<std::string, ModuleRotation2Axis>>{{"XAxis", ModuleRotation2Axis::X},
                                                                  {"YAxis", ModuleRotation2Axis::Y},
                                                                  {"ZAxis", ModuleRotation2Axis::Z},
                                                                  {"MinusXAxis", ModuleRotation2Axis::MinusX},
                                                                  {"MinusYAxis", ModuleRotation2Axis::MinusY},
                                                                  {"MinusZAxis", ModuleRotation2Axis::MinusZ}})
    {
        INFO(word);
        const Model each = readBulk("ModuleRotation2;30;;;" + word + ";10.");
        REQUIRE(cardAt<ModuleRotation2Table>(each).axis == axis);
        requireRoundTrip(each);
    }

    REQUIRE(visitedBy<ModuleRotation2Table>(model, 0).fields.empty()); // the basic system and no point
    const Visited visited = visitedBy<ModuleRotation2Table>(model, 1);
    REQUIRE(visited.fields == std::vector<std::string>{"frameId", "referenceNode"});
    REQUIRE(visited.targets == std::vector<ReferenceTarget>{ReferenceTarget::Coordinate, ReferenceTarget::Node});

    expectInvalid("ModuleRotation2;0;;;ZAxis;45.", "id");
    expectInvalid("ModuleRotation2;30;-1;;ZAxis;45.", "frameId");
    expectInvalid("ModuleRotation2;30;;-1;ZAxis;45.", "referenceNode");
    expectInvalid("ModuleRotation2;30;;;Q;45.", "axis");
    expectInvalid("ModuleRotation2;30;;;PlaneXy;45.", "axis"); // a pair of axes belongs to an MDMIR2
    expectInvalid("ModuleRotation2;30;;;;45.", "axis");        // required
    expectInvalid("ModuleRotation2;30;;;ZAxis", "magnitude");  // required
    expectInvalid("ModuleRotation2;30;;;ZAxis;0.", "magnitude");
}

TEST_CASE("an MDMIR1 mirrors in the plane of three points", "[model][cards][modulemove]")
{
    Model model = readBulk(POINTS_TEXT + MOVES_TEXT);
    const ModuleMirror1Card card = cardAt<ModuleMirror1Table>(model, 0);
    REQUIRE((card.id == 40 && card.point1 == 1 && card.point2 == 2 && card.point3 == 3));
    const Visited visited = visitedBy<ModuleMirror1Table>(model, 0);
    REQUIRE(visited.fields == std::vector<std::string>{"point1", "point2", "point3"});
    REQUIRE(visited.targets == std::vector<ReferenceTarget>(3, ReferenceTarget::Node));

    expectInvalid("ModuleMirror1;0;1;2;3", "id");
    expectInvalid("ModuleMirror1;40;0;2;3", "point1");
    expectInvalid("ModuleMirror1;40;1;2", "point3");   // required
    expectInvalid("ModuleMirror1;40;1;1;3", "point2"); // three different points
    expectInvalid("ModuleMirror1;40;1;2;1", "point3");
}

TEST_CASE("an MDMIR2 mirrors in the plane of two axes of a coordinate system", "[model][cards][modulemove]")
{
    Model model = readBulk(POINTS_TEXT + MOVES_TEXT);
    const ModuleMirror2Card basic = cardAt<ModuleMirror2Table>(model, 0);
    REQUIRE((basic.id == 50 && basic.frameId == 0 && basic.referenceNode == 0 && basic.axes == ModuleMirror2Axes::Xy));
    const ModuleMirror2Card moved = cardAt<ModuleMirror2Table>(model, 1);
    REQUIRE((moved.frameId == 5 && moved.referenceNode == 4 && moved.axes == ModuleMirror2Axes::Zx));
    for (const auto& [word, axes] :
         std::vector<std::pair<std::string, ModuleMirror2Axes>>{{"PlaneXy", ModuleMirror2Axes::Xy},
                                                                {"PlaneXz", ModuleMirror2Axes::Xz},
                                                                {"PlaneYz", ModuleMirror2Axes::Yz},
                                                                {"PlaneYx", ModuleMirror2Axes::Yx},
                                                                {"PlaneZx", ModuleMirror2Axes::Zx},
                                                                {"PlaneZy", ModuleMirror2Axes::Zy}})
    {
        INFO(word);
        const Model each = readBulk("ModuleMirror2;50;;;" + word);
        REQUIRE(cardAt<ModuleMirror2Table>(each).axes == axes);
        requireRoundTrip(each);
    }

    REQUIRE(visitedBy<ModuleMirror2Table>(model, 0).fields.empty());
    const Visited visited = visitedBy<ModuleMirror2Table>(model, 1);
    REQUIRE(visited.fields == std::vector<std::string>{"frameId", "referenceNode"});
    REQUIRE(visited.targets == std::vector<ReferenceTarget>{ReferenceTarget::Coordinate, ReferenceTarget::Node});

    expectInvalid("ModuleMirror2;0;;;PlaneXy", "id");
    expectInvalid("ModuleMirror2;50;-2;;PlaneXy", "frameId");
    expectInvalid("ModuleMirror2;50;;-2;PlaneXy", "referenceNode");
    expectInvalid("ModuleMirror2;50;;;XX", "axes");
    expectInvalid("ModuleMirror2;50;;;XAxis", "axes"); // a single axis belongs to an MDROT2
    expectInvalid("ModuleMirror2;50", "axes");         // required
}

TEST_CASE("an MDMOVE lists the moves in order over any number of lines and names them as moves",
          "[model][cards][modulemove]")
{
    Model model = readBulk(POINTS_TEXT + MOVES_TEXT + MOVE_SEQUENCE_TEXT +
                           "ModuleMoveSequence;61;10\nModuleMoveSequence;62;11;;20\n");
    REQUIRE((ModuleMoveSequenceTable::INFO.category == CardCategory::Other &&
             ModuleMoveSequenceTable::INFO.idSpace == base::IdSpace::PerCardType));
    const ModuleMoveSequenceCard sequence = cardAt<ModuleMoveSequenceTable>(model, 0);
    REQUIRE(sequence.id == 60);
    REQUIRE(sequence.moveIds == std::vector<base::EntityId>{40, 10, 20, 30, 31, 11, 21, 50, 51});
    REQUIRE(cardAt<ModuleMoveSequenceTable>(model, 1).moveIds == std::vector<base::EntityId>{10});
    REQUIRE(cardAt<ModuleMoveSequenceTable>(model, 2).moveIds ==
            std::vector<base::EntityId>{11, 20}); // a blank is skipped
    REQUIRE(unresolvedIn(model) == 0); // the moves are cards of five kinds, and the points and the system exist

    // The module number is a plain integer: only the moves are visited.
    const Visited visited = visitedBy<ModuleMoveSequenceTable>(model, 1);
    REQUIRE(visited.fields == std::vector<std::string>{"moveId"});
    REQUIRE(visited.targets == std::vector<ReferenceTarget>{ReferenceTarget::ModuleMove});
    REQUIRE(visitedBy<ModuleMoveSequenceTable>(model, 0).fields.size() == 9);

    // A move that no card defines is an unresolved reference; the module is not looked up.
    Model missing = readBulk("ModuleMoveSequence;60;10;99");
    REQUIRE(unresolvedIn(missing) == 2);

    REQUIRE_THROWS_AS(readBulk("ModuleMoveSequence;60;10\nModuleMoveSequence;60;11"),
                      base::DeckError); // one sequence per module
    REQUIRE_NOTHROW(readBulk("ModuleMoveSequence;60;10\nModuleMirrorPlane;60;1;2;3\nModuleLocation;60;1;2;3;1;2;3"));

    // Renumbering the moves follows into the list and leaves the module alone.
    Renumbering::apply(model, RenumberSpace::ModuleMove, Renumbering::offset(1000));
    REQUIRE((cardAt<ModuleTranslationTable>(model, 0).id == 1010 && cardAt<ModuleMirror2Table>(model, 1).id == 1051));
    REQUIRE(cardAt<ModuleMoveSequenceTable>(model, 0).id == 60);
    REQUIRE(cardAt<ModuleMoveSequenceTable>(model, 0).moveIds ==
            std::vector<base::EntityId>{1040, 1010, 1020, 1030, 1031, 1011, 1021, 1050, 1051});
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE((cardAt<ModuleTranslationTable>(model, 0).startNode == 101 &&
             cardAt<ModuleRotation1Table>(model, 1).referenceNode == 104));
    REQUIRE((cardAt<ModuleMirror1Table>(model, 0).point3 == 103 &&
             cardAt<ModuleRotation2Table>(model, 1).referenceNode == 103));
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    REQUIRE(
        (cardAt<ModuleRotation2Table>(model, 1).frameId == 15 && cardAt<ModuleMirror2Table>(model, 1).frameId == 15));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    expectInvalid("ModuleMoveSequence;0;10", "id");
    expectInvalid("ModuleMoveSequence;60", "moveId1"); // at least one move
    expectInvalid("ModuleMoveSequence;60;0", "moveId");
    expectInvalid("ModuleMoveSequence;60;10;-3", "moveId");
    expectInvalid("ModuleMoveSequence;60;10;abc", "moveId");
}

TEST_CASE("an MDMPLN and an MDLOC are keyed by their module and look up the points of module 0 only",
          "[model][cards][modulemove]")
{
    Model model = readBulk(POINTS_TEXT + "ModuleMirrorPlane;70;1;2;3\nModuleLocation;80;10;100;120;1;2;4\n");
    REQUIRE((ModuleMirrorPlaneTable::INFO.category == CardCategory::Other &&
             ModuleMirrorPlaneTable::INFO.idSpace == base::IdSpace::PerCardType));
    REQUIRE((ModuleLocationTable::INFO.category == CardCategory::Other &&
             ModuleLocationTable::INFO.idSpace == base::IdSpace::PerCardType));
    const ModuleMirrorPlaneCard plane = cardAt<ModuleMirrorPlaneTable>(model);
    REQUIRE((plane.id == 70 && plane.point1 == 1 && plane.point2 == 2 && plane.point3 == 3));
    const ModuleLocationCard placed = cardAt<ModuleLocationTable>(model);
    REQUIRE((placed.id == 80 && placed.pointA1 == 10 && placed.pointA2 == 100 && placed.pointA3 == 120));
    REQUIRE((placed.pointB1 == 1 && placed.pointB2 == 2 && placed.pointB3 == 4));
    // The PAi are points of the module's own Bulk Data (text in this model): no grid 10, 100 or 120 is looked up.
    REQUIRE(unresolvedIn(model) == 0);

    const Visited planeVisited = visitedBy<ModuleMirrorPlaneTable>(model, 0);
    REQUIRE(planeVisited.fields == std::vector<std::string>{"point1", "point2", "point3"});
    REQUIRE(planeVisited.targets == std::vector<ReferenceTarget>(3, ReferenceTarget::Node));
    const Visited placedVisited = visitedBy<ModuleLocationTable>(model, 0);
    REQUIRE(placedVisited.fields == std::vector<std::string>{"pointB1", "pointB2", "pointB3"});
    REQUIRE(placedVisited.targets == std::vector<ReferenceTarget>(3, ReferenceTarget::Node));

    // The points of module 0 follow a renumbering of the nodes, the points of the module and the module do not.
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE(
        (cardAt<ModuleMirrorPlaneTable>(model).point1 == 1001 && cardAt<ModuleMirrorPlaneTable>(model).point3 == 1003));
    REQUIRE((cardAt<ModuleLocationTable>(model).pointB1 == 1001 && cardAt<ModuleLocationTable>(model).pointB3 == 1004));
    REQUIRE((cardAt<ModuleLocationTable>(model).pointA1 == 10 && cardAt<ModuleLocationTable>(model).pointA3 == 120));
    REQUIRE((cardAt<ModuleLocationTable>(model).id == 80 && cardAt<ModuleMirrorPlaneTable>(model).id == 70));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    expectInvalid("ModuleMirrorPlane;0;1;2;3", "id");
    expectInvalid("ModuleMirrorPlane;70;0;2;3", "point1");
    expectInvalid("ModuleMirrorPlane;70;1;2", "point3");   // required
    expectInvalid("ModuleMirrorPlane;70;1;2;1", "point3"); // three different points

    expectInvalid("ModuleLocation;0;10;100;120;1;2;3", "id");
    expectInvalid("ModuleLocation;80;0;100;120;1;2;3", "pointA1");
    expectInvalid("ModuleLocation;80;10;10;120;1;2;3", "pointA2");
    expectInvalid("ModuleLocation;80;10;100;120;1;2", "pointB3"); // required
    expectInvalid("ModuleLocation;80;10;100;120;1;3;3", "pointB3");
    // The module and the main model may use the same numbers: each triple is its own set.
    REQUIRE_NOTHROW(readBulk("ModuleLocation;80;1;2;3;1;2;3"));
}

TEST_CASE("a HADACRI is a criterion of one of four types and up to six parameters", "[model][cards][adaptivity]")
{
    REQUIRE((AdaptiveRefineCriterionTable::INFO.category == CardCategory::AdaptivityCriterion &&
             AdaptiveRefineCriterionTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(
        "AdaptiveRefineCriterion;1;1;0.9\n"
        "AdaptiveRefineCriterion;2;2;1.5;2.5;3.5;0.75\n"
        "AdaptiveRefineCriterion;3;3;0.;0.;0.;1.;2.;3.\n"
        "AdaptiveRefineCriterion;4;4\n"
        "AdaptiveRefineCriterion;5;;0.5\n"
        "AdaptiveRefineCriterion;6;1;0.25;9.;9.;9.;9.;9.\n"
        "AdaptiveRefineCriterion;7;2;;;;2.\n");
    const AdaptiveRefineCriterionCard errorBased = cardAt<AdaptiveRefineCriterionTable>(model, 0);
    REQUIRE((errorBased.id == 1 && errorBased.kind == 1 && errorBased.parameters[0] == 0.9));
    REQUIRE_FALSE(errorBased.parameters[1].has_value());
    const AdaptiveRefineCriterionCard sphere = cardAt<AdaptiveRefineCriterionTable>(model, 1);
    REQUIRE((sphere.kind == 2 && sphere.parameters[0] == 1.5 && sphere.parameters[1] == 2.5));
    REQUIRE((sphere.parameters[2] == 3.5 && sphere.parameters[3] == 0.75));
    REQUIRE_FALSE((sphere.parameters[4].has_value() || sphere.parameters[5].has_value()));
    const AdaptiveRefineCriterionCard box = cardAt<AdaptiveRefineCriterionTable>(model, 2);
    REQUIRE((box.kind == 3 && box.parameters[3] == 1.0 && box.parameters[4] == 2.0 && box.parameters[5] == 3.0));
    const AdaptiveRefineCriterionCard contact = cardAt<AdaptiveRefineCriterionTable>(model, 3);
    REQUIRE((contact.kind == 4 && !contact.parameters[0].has_value()));
    REQUIRE(cardAt<AdaptiveRefineCriterionTable>(model, 4).kind == 1); // a blank TYPE is 1
    // The parameters a type ignores are kept as written, and a blank one stays blank (a zero centre or corner).
    REQUIRE(cardAt<AdaptiveRefineCriterionTable>(model, 5).parameters[5] == 9.0);
    const AdaptiveRefineCriterionCard partial = cardAt<AdaptiveRefineCriterionTable>(model, 6);
    REQUIRE((partial.parameters[0] == std::nullopt && partial.parameters[3] == 2.0));
    REQUIRE(visitedBy<AdaptiveRefineCriterionTable>(model, 0).fields.empty()); // names no other card
    requireRoundTrip(model);

    // Only the weight of TYPE 1 has a range; for the other types F1 is a coordinate.
    REQUIRE_NOTHROW(
        readBulk("AdaptiveRefineCriterion;1;1;0.\nAdaptiveRefineCriterion;2;1;1.\nAdaptiveRefineCriterion;3;2;-4.5;0.;"
                 "0.;2.\nAdaptiveRefineCriterion;4;3;5.;5.;5.;9.;9.;9."));
    expectInvalid("AdaptiveRefineCriterion;0;1;0.5", "id");
    expectInvalid("AdaptiveRefineCriterion;1;0", "kind");
    expectInvalid("AdaptiveRefineCriterion;1;5", "kind");
    expectInvalid("AdaptiveRefineCriterion;1;-1", "kind");
    expectInvalid("AdaptiveRefineCriterion;1;1;1.5", "parameter1");
    expectInvalid("AdaptiveRefineCriterion;1;1;-0.1", "parameter1");
    expectInvalid("AdaptiveRefineCriterion;1;1;abc", "parameter1");
    expectInvalid("AdaptiveRefineCriterion;1;3;0.;0.;0.;1.;2.;x", "parameter6");
}

TEST_CASE("a HADAPTL has the layout of its two lines, a REPEAT that gives the default of MAXLEVEL, and a shared ID",
          "[model][cards][adaptivity]")
{
    REQUIRE((AdaptiveRefineControlTable::INFO.category == CardCategory::AdaptivitySet &&
             AdaptiveRefineControlTable::INFO.idSpace == base::IdSpace::Group));
    Model model = readBulk(
        "AdaptiveRefineControl;1;10;1;Property;5\n"
        "AdaptiveRefineControl;2;;1\n"
        "AdaptiveRefineControl;3;8;2;Superelement;5;1;4\n"
        "AdaptiveRefineControl;4;;3;All;5\n"
        "AdaptiveRefineControl;5;;3;All;0;0;2\n");
    const AdaptiveRefineControlCard property = cardAt<AdaptiveRefineControlTable>(model, 0);
    REQUIRE((property.id == 1 && property.repeatCount == 10 && property.criterionId == 1));
    REQUIRE((property.regionMethod == AdaptivityRegion::Prop && property.regionId == 5 && property.snapMethod == 0));
    REQUIRE(property.maximumLevel == 10); // a blank MAXLEVEL is REPEAT, stored resolved
    const AdaptiveRefineControlCard defaults = cardAt<AdaptiveRefineControlTable>(model, 1);
    REQUIRE((defaults.repeatCount == 6 && defaults.regionMethod == AdaptivityRegion::All && defaults.regionId == 0));
    REQUIRE((defaults.snapMethod == 0 && defaults.maximumLevel == 6));
    const AdaptiveRefineControlCard superelement = cardAt<AdaptiveRefineControlTable>(model, 2);
    REQUIRE((superelement.repeatCount == 8 && superelement.regionMethod == AdaptivityRegion::Super));
    REQUIRE((superelement.regionId == 5 && superelement.snapMethod == 1 && superelement.maximumLevel == 4));
    // WHEREID is ignored with ALL, and kept as written.
    REQUIRE(cardAt<AdaptiveRefineControlTable>(model, 3).regionId == 5);
    const AdaptiveRefineControlCard lowest = cardAt<AdaptiveRefineControlTable>(model, 4);
    REQUIRE((lowest.repeatCount == 6 && lowest.maximumLevel == 2)); // a MAXLEVEL below REPEAT is allowed
    requireRoundTrip(model);

    // Several cards may have one ID, each with its own criterion and region.
    const Model shared = readBulk("AdaptiveRefineControl;1;10;1;Property;5\nAdaptiveRefineControl;1;;2;All");
    REQUIRE(rowCount<AdaptiveRefineControlTable>(shared) == 2);
    REQUIRE((cardAt<AdaptiveRefineControlTable>(shared, 0).criterionId == 1 &&
             cardAt<AdaptiveRefineControlTable>(shared, 1).criterionId == 2));
    requireRoundTrip(shared);

    // The edited REPEAT does not move a MAXLEVEL that was resolved from it.
    AdaptiveRefineControlCard edited = cardAt<AdaptiveRefineControlTable>(model, 1);
    edited.repeatCount = 9;
    REQUIRE_FALSE(AdaptiveRefineControlFormat::check(edited).has_value());
    REQUIRE(edited.maximumLevel == 6);
    edited.maximumLevel = 0;
    REQUIRE(AdaptiveRefineControlFormat::check(edited).has_value());
}

TEST_CASE("a HADAPTL names its criterion, and a property or a superelement according to WHEREMET",
          "[model][cards][adaptivity]")
{
    // Superelement 5 exists because a grid point has it as its SEID; the property 5 is a PSHELL.
    Model model = readBulk(
        "AdaptiveRefineCriterion;1;1;0.9\n"
        "AdaptiveRefineCriterion;2;4\n"
        "AdaptiveRefineCriterion;3;4\n"
        "ShellProp;5;1;0.1\n"
        "IsoMat;1;2.1E+11;;0.3\n"
        "Node;9;;0.;0.;0.;;;5\n"
        "AdaptiveRefineControl;1;10;1;Property;5\n"
        "AdaptiveRefineControl;1;;2;All;5\n"
        "AdaptiveRefineControl;2;;3;Superelement;5\n");
    REQUIRE(unresolvedIn(model) == 0);

    const Visited property = visitedBy<AdaptiveRefineControlTable>(model, 0);
    REQUIRE(property.fields == std::vector<std::string>{"criterionId", "regionId"});
    REQUIRE(property.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::AdaptivityCriterion, ReferenceTarget::Property});
    const Visited everything = visitedBy<AdaptiveRefineControlTable>(model, 1);
    REQUIRE(everything.fields == std::vector<std::string>{"criterionId"}); // WHEREID is ignored
    REQUIRE(everything.targets == std::vector<ReferenceTarget>{ReferenceTarget::AdaptivityCriterion});
    const Visited superelement = visitedBy<AdaptiveRefineControlTable>(model, 2);
    REQUIRE(superelement.fields == std::vector<std::string>{"criterionId", "regionId"});
    REQUIRE(superelement.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::AdaptivityCriterion, ReferenceTarget::Superelement});
    // SUPER with a zero WHEREID is the residual structure: nothing to look up.
    Model residual = readBulk("AdaptiveRefineControl;1;;1;Superelement");
    REQUIRE(visitedBy<AdaptiveRefineControlTable>(residual, 0).fields == std::vector<std::string>{"criterionId"});

    // Each reference follows the renumbering of its own space, and only that one.
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE(cardAt<AdaptiveRefineControlTable>(model, 0).regionId == 1005);
    REQUIRE((cardAt<AdaptiveRefineControlTable>(model, 1).regionId == 5 &&
             cardAt<AdaptiveRefineControlTable>(model, 2).regionId == 5));
    Renumbering::apply(model, RenumberSpace::Superelement, Renumbering::offset(100));
    REQUIRE(cardAt<AdaptiveRefineControlTable>(model, 2).regionId == 105);
    REQUIRE((cardAt<AdaptiveRefineControlTable>(model, 0).regionId == 1005 &&
             cardAt<AdaptiveRefineControlTable>(model, 1).regionId == 5));
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::AdaptivityCriterion, Renumbering::offset(50));
    REQUIRE((cardAt<AdaptiveRefineControlTable>(model, 0).criterionId == 51 &&
             cardAt<AdaptiveRefineControlTable>(model, 1).criterionId == 52));
    REQUIRE((cardAt<AdaptiveRefineControlTable>(model, 2).criterionId == 53 &&
             cardAt<AdaptiveRefineCriterionTable>(model, 2).id == 53));
    // The cards of one set get one new ID.
    Renumbering::apply(model, RenumberSpace::AdaptivitySet, Renumbering::offset(200));
    REQUIRE((cardAt<AdaptiveRefineControlTable>(model, 0).id == 201 &&
             cardAt<AdaptiveRefineControlTable>(model, 1).id == 201));
    REQUIRE(cardAt<AdaptiveRefineControlTable>(model, 2).id == 202);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // A criterion, a property or a superelement that does not exist is reported.
    Model missing = readBulk("AdaptiveRefineControl;1;10;7;Property;5");
    REQUIRE(unresolvedIn(missing) == 2);

    expectInvalid("AdaptiveRefineControl;0;;1", "id");
    expectInvalid("AdaptiveRefineControl;1", "criterionId"); // required
    expectInvalid("AdaptiveRefineControl;1;;0", "criterionId");
    expectInvalid("AdaptiveRefineControl;1;-1;1", "repeatCount");
    expectInvalid("AdaptiveRefineControl;1;;1;GLOBAL", "regionMethod");
    expectInvalid("AdaptiveRefineControl;1;;1;Property", "regionId"); // a property is needed
    expectInvalid("AdaptiveRefineControl;1;;1;Property;-2", "regionId");
    expectInvalid("AdaptiveRefineControl;1;;1;All;0;2", "snapMethod");
    expectInvalid("AdaptiveRefineControl;1;;1;All;0;-1", "snapMethod");
    expectInvalid("AdaptiveRefineControl;1;;1;All;0;0;0", "maximumLevel");
    expectInvalid("AdaptiveRefineControl;1;0;1", "maximumLevel"); // a blank MAXLEVEL would be the REPEAT of 0
}
