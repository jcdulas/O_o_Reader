// Checks of the coordinate systems and the core elements: their variants, defaults, errors and references.

#include "oor/model/cards/coordinate_systems/cylindricalFrame1/CylindricalFrame1Table.hpp"
#include "oor/model/cards/coordinate_systems/cylindricalFrame2/CylindricalFrame2Table.hpp"
#include "oor/model/cards/coordinate_systems/rectFrame1/RectFrame1Table.hpp"
#include "oor/model/cards/coordinate_systems/rectFrame2/RectFrame2Table.hpp"
#include "oor/model/cards/coordinate_systems/sphericalFrame1/SphericalFrame1Table.hpp"
#include "oor/model/cards/coordinate_systems/sphericalFrame2/SphericalFrame2Table.hpp"
#include "oor/model/cards/elements/bar2/Bar2Table.hpp"
#include "oor/model/cards/elements/beam2/Beam2Table.hpp"
#include "oor/model/cards/elements/bushing/BushingTable.hpp"
#include "oor/model/cards/elements/solidHexa/SolidHexaTable.hpp"
#include "oor/model/cards/elements/solidPenta/SolidPentaTable.hpp"
#include "oor/model/cards/elements/pointMassInertia/PointMassInertiaTable.hpp"
#include "oor/model/cards/elements/springDofProperty/SpringDofPropertyTable.hpp"
#include "oor/model/cards/elements/springDofInline/SpringDofInlineTable.hpp"
#include "oor/model/cards/elements/springScalarProperty/SpringScalarPropertyTable.hpp"
#include "oor/model/cards/elements/springScalarInline/SpringScalarInlineTable.hpp"
#include "oor/model/cards/elements/solidTetra/SolidTetraTable.hpp"
#include "oor/model/cards/elements/shellTria3/ShellTria3Table.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using oor::testing::readModelText;
using oor::testing::writeModelText;

namespace
{

Model readBulk(const std::string& cards)
{
    return readModelText(cards, true);
}

template <typename TableT>
typename TableT::CardType cardAt(const Model& model, std::uint32_t row = 0)
{
    return model.findTable<TableT>()->card(row);
}

// The number of references the model has to cards that do not exist.
std::size_t unresolvedIn(Model& model)
{
    base::DiagnosticSink sink(base::ReadMode::Tolerant);
    return ReferenceChecker::check(model, sink);
}

// Records the references a card lists: the field names and what each points to.
struct RecordingVisitor final : ReferenceVisitor
{
    std::vector<std::string> fields;
    std::vector<ReferenceTarget> targets;
    base::EntityId visit(std::string_view field, ReferenceTarget target, base::EntityId id) override
    {
        fields.emplace_back(field);
        targets.push_back(target);
        return id;
    }
};

} // namespace

TEST_CASE("CORD2R reads three points in the system RID and defaults RID to the basic system", "[model][cards]")
{
    const Model model = readBulk("RectFrame2;3;12;-1.7;2.;0.;4.3;0.;1.5;6.1;2.;-1.7\nRectFrame2;4");
    const RectFrame2Card first = cardAt<RectFrame2Table>(model, 0);
    REQUIRE(first.id == 3);
    REQUIRE(first.referenceFrameId == 12);
    REQUIRE(first.pointA == std::array<double, 3>{-1.7, 2.0, 0.0});
    REQUIRE(first.pointB == std::array<double, 3>{4.3, 0.0, 1.5});
    REQUIRE(first.pointC == std::array<double, 3>{6.1, 2.0, -1.7});
    const RectFrame2Card blank = cardAt<RectFrame2Table>(model, 1);
    REQUIRE(blank.referenceFrameId == 0);
    REQUIRE(blank.pointA == std::array<double, 3>{});
}

TEST_CASE("CORD2C and CORD2S read the same fields as CORD2R", "[model][cards]")
{
    const Model model =
        readBulk("CylindricalFrame2;3;1;1.;2.;3.;4.;5.;6.;7.;8.;9.\nSphericalFrame2;4;2;9.;8.;7.;6.;5.;4.;3.;2.;1.");
    const CylindricalFrame2Card cylindrical = cardAt<CylindricalFrame2Table>(model);
    REQUIRE(cylindrical.referenceFrameId == 1);
    REQUIRE(cylindrical.pointB == std::array<double, 3>{4.0, 5.0, 6.0});
    const SphericalFrame2Card spherical = cardAt<SphericalFrame2Table>(model);
    REQUIRE(spherical.referenceFrameId == 2);
    REQUIRE(spherical.pointC == std::array<double, 3>{3.0, 2.0, 1.0});
}

TEST_CASE("a coordinate system cannot be defined with respect to itself", "[model][cards]")
{
    REQUIRE_THROWS_AS(readBulk("RectFrame2;5;5"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("RectFrame2;5;-1"), base::DeckError);
}

TEST_CASE("one CORD1R card with two systems gives two rows, each written as a card of its own", "[model][cards]")
{
    const Model model = readBulk("RectFrame1;3;14;27;21;4;1;2;3");
    const RectFrame1Table& table = *model.findTable<RectFrame1Table>();
    REQUIRE(table.rows().aliveCount() == 2);
    REQUIRE(table.card(0).id == 3);
    REQUIRE(table.card(0).nodes == std::array<base::EntityId, 3>{14, 27, 21});
    REQUIRE(table.card(1).id == 4);
    REQUIRE(table.card(1).nodes == std::array<base::EntityId, 3>{1, 2, 3});
    REQUIRE(model.locate(base::IdSpace::Coordinate, 4).has_value());

    oor::testing::requireModelFileRoundTrip(model);
    // Each system can be renumbered or deleted without the other.
    Model copy = model.copy();
    copy.remove(*copy.locate(base::IdSpace::Coordinate, 3));
    oor::testing::requireModelFileRoundTrip(copy);
}

TEST_CASE("the second system of a CORD1C or CORD1S card needs its ID and its three grid points", "[model][cards]")
{
    const Model one = readBulk("CylindricalFrame1;3;1;2;3\nSphericalFrame1;4;1;2;3");
    REQUIRE(one.findTable<CylindricalFrame1Table>()->rows().aliveCount() == 1);
    REQUIRE(one.findTable<SphericalFrame1Table>()->rows().aliveCount() == 1);
    REQUIRE_THROWS_AS(readBulk("CylindricalFrame1;3;1;2;3;4;1;2"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("SphericalFrame1;3;1;2;3;;1;2;3"), base::DeckError);
    // The error names the field of the second system.
    const Model tolerant = readModelText("[model]\nCylindricalFrame1;3;1;2;3;4;1;2\n", false);
    REQUIRE(tolerant.diagnostics().firstError()->location.fieldName == "node3B");
}

TEST_CASE("the six coordinate system cards share one ID space", "[model][cards]")
{
    REQUIRE_THROWS_AS(readBulk("RectFrame2;5\nCylindricalFrame1;5;1;2;3"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("RectFrame1;5;1;2;3;6;4;5;6\nSphericalFrame2;6"), base::DeckError);
    REQUIRE_NOTHROW(
        readBulk("RectFrame2;5\nCylindricalFrame2;6\nSphericalFrame2;7\nRectFrame1;8;1;2;3\nCylindricalFrame1;9;1;2;"
                 "3\nSphericalFrame1;10;1;2;3"));
}

TEST_CASE("coordinate systems refer to a system and to grid points", "[model][cards]")
{
    Model model = readBulk("RectFrame2;1\nRectFrame2;2;1\nRectFrame2;3;9\nRectFrame1;4;1;2;3");
    // CORD2R 3 refers to system 9 (absent), CORD1R 4 to grid points 1, 2, 3 (absent).
    REQUIRE(unresolvedIn(model) == 4);
    RecordingVisitor visitor;
    model.table<RectFrame2Table>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"referenceFrameId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Coordinate});
    RecordingVisitor basic;
    model.table<RectFrame2Table>().visitReferences(0, basic); // RID 0 is the basic system: no reference
    REQUIRE(basic.fields.empty());
}

TEST_CASE("CTRIA3 takes an integer in field 6 as MCID and a real as THETA, and PID defaults to EID", "[model][cards]")
{
    const Model model = readBulk("ShellTria3;1;;1;2;3;;7\nShellTria3;2;4;1;2;3;30.;;0.5;1;0.1;0.2");
    const ShellTria3Card byMcid = cardAt<ShellTria3Table>(model, 0);
    REQUIRE(byMcid.propId == 1);
    REQUIRE(byMcid.usesMaterialSystem());
    REQUIRE(byMcid.matFrameId == 7);
    const ShellTria3Card byTheta = cardAt<ShellTria3Table>(model, 1);
    REQUIRE_FALSE(byTheta.usesMaterialSystem());
    REQUIRE(byTheta.thetaInDegrees == 30.0);
    REQUIRE(byTheta.offset == 0.5);
    REQUIRE(byTheta.thicknessFlag == 1);
    REQUIRE(byTheta.cornerThickness[0] == 0.1);
    REQUIRE(byTheta.cornerThickness[1] == 0.2);
    REQUIRE(byTheta.cornerThickness[2] == ShellTria3Card::NO_THICKNESS);
    REQUIRE_THROWS_AS(readBulk("ShellTria3;1;1;1;2;3;0.;;0.;2"), base::DeckError); // TFLAG is 0 or 1
}

TEST_CASE("CBAR reads its orientation as a vector or as a grid point", "[model][cards]")
{
    const Model model = readBulk(
        "Bar2;1;5;1;2;0.8;;14.;31.;OffsetsBasicOrientGlobal;456;12;0.1;0.2;0.3\n"
        "Bar2;2;;3;4;;105;;;OffsetsGlobalOrientGlobal\n"
        "Bar2;3;;5;6;;;2.");
    const Bar2Card byVector = cardAt<Bar2Table>(model, 0);
    REQUIRE_FALSE(byVector.usesGridPoint());
    REQUIRE(byVector.orientation == std::array<double, 3>{0.8, 14.0, 31.0});
    REQUIRE(byVector.offsetKind == "OffsetsBasicOrientGlobal");
    REQUIRE(byVector.pinFlagsA == 456);
    REQUIRE(byVector.pinFlagsB == 12);
    REQUIRE(byVector.offsetA == std::array<double, 3>{0.1, 0.2, 0.3});
    REQUIRE(byVector.offsetB == std::array<double, 3>{});
    const Bar2Card byGrid = cardAt<Bar2Table>(model, 1);
    REQUIRE(byGrid.usesGridPoint());
    REQUIRE(byGrid.orientationNode == 105);
    REQUIRE(byGrid.orientation == std::array<double, 3>{});
    REQUIRE(byGrid.propId == 2); // PID defaults to EID
    REQUIRE(byGrid.offsetKind == "OffsetsGlobalOrientGlobal");
    REQUIRE(cardAt<Bar2Table>(model, 2).offsetKind == "OffsetsGlobalGlobalGlobal"); // blank OFFT
    // A blank X1 is 0.0, the other two components are still read.
    REQUIRE(cardAt<Bar2Table>(model, 2).orientation == std::array<double, 3>{0.0, 2.0, 0.0});
}

TEST_CASE("CBAR OFFT is an offset kind word and the pin flags are kept in ascending order", "[model][cards]")
{
    const Model model = readBulk("Bar2;1;1;1;2;0.;;0.;1.;OffsetsBasicGlobalOrient;654;21");
    const Bar2Card card = cardAt<Bar2Table>(model);
    REQUIRE(card.offsetKind == "OffsetsBasicGlobalOrient");
    REQUIRE(card.pinFlagsA == 456);
    REQUIRE(card.pinFlagsB == 12);
    for (const char* offt : {"OOO", "GGB", "GG", "BBBB", "3", "BGO", "offsetsBasicGlobalOrient"})
        REQUIRE_THROWS_AS(readBulk(std::string("Bar2;1;1;1;2;0.;;0.;1.;") + offt), base::DeckError);
    REQUIRE_NOTHROW(readBulk("Bar2;1;1;1;2;0.;;0.;1.;2")); // an old integer code
}

TEST_CASE("CBAR with no orientation is written with blank orientation fields", "[model][cards]")
{
    // A CBAR that relies on a BAROR card has blank fields 5 to 7: they must stay blank, not become 0.0.
    const Model model = readBulk("Bar2;1;1;1;2");

    oor::testing::requireModelFileRoundTrip(model);
}

TEST_CASE("CBAR lists the property, the two ends and G0 as references", "[model][cards]")
{
    Model model = readBulk("Bar2;1;5;1;2;;105\nBar2;2;5;1;2;0.;;0.;1.");
    RecordingVisitor withGrid;
    model.table<Bar2Table>().visitReferences(0, withGrid);
    REQUIRE(withGrid.fields == std::vector<std::string>{"propId", "nodeA", "nodeB", "orientationNode"});
    REQUIRE(withGrid.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Node,
                                                             ReferenceTarget::Node, ReferenceTarget::Node});
    RecordingVisitor withVector;
    model.table<Bar2Table>().visitReferences(1, withVector);
    REQUIRE(withVector.fields == std::vector<std::string>{"propId", "nodeA", "nodeB"});
}

TEST_CASE("CBEAM reads the warping points and refuses a numeric OFFT", "[model][cards]")
{
    const Model model = readBulk("Beam2;1;5;1;2;0.;;0.;1.;;;;;;;;;;7;8\nBeam2;2;5;1;2;;105");
    const Beam2Card warped = cardAt<Beam2Table>(model, 0);
    REQUIRE(warped.warpingPointA == 7);
    REQUIRE(warped.warpingPointB == 8);
    const Beam2Card byGrid = cardAt<Beam2Table>(model, 1);
    REQUIRE(byGrid.orientationNode == 105);
    REQUIRE(byGrid.warpingPointA == 0);
    REQUIRE_THROWS_AS(readBulk("Beam2;1;1;1;2;0.;;0.;1.;0.5"), base::DeckError);
}

TEST_CASE("the warping points of a CBEAM are grid or scalar points and need not exist", "[model][cards]")
{
    Model model = readBulk("Beam2;1;5;1;2;0.;;0.;1.;;;;;;;;;;7;8");
    RecordingVisitor visitor;
    model.table<Beam2Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propId", "nodeA", "nodeB", "warpingPointA", "warpingPointB"});
    REQUIRE(visitor.targets[3] == ReferenceTarget::ScalarOrGridPoint);
    // Only the property and the two grid points are missing: the scalar points do not count.
    REQUIRE(unresolvedIn(model) == 3);
}

TEST_CASE("solid elements keep a fixed array of nodes with 0 for a missing edge node", "[model][cards]")
{
    const Model model = readBulk(
        "SolidTetra;1;2;10;20;30;40\n"
        "SolidTetra;2;2;10;20;30;40;50;;60;;70\n"
        "SolidPenta;3;2;1;2;3;4;5;6\n"
        "SolidHexa;4;2;1;2;3;4;5;6;7;8;9;10;0;0;30;31;55;56;57;58;59;60");
    const SolidTetraCard corners = cardAt<SolidTetraTable>(model, 0);
    REQUIRE(corners.nodes.size() == 10);
    REQUIRE_FALSE(corners.hasEdgeNodes());
    REQUIRE(corners.nodes[3] == 40);
    REQUIRE(corners.nodes[4] == 0);
    const SolidTetraCard partial = cardAt<SolidTetraTable>(model, 1);
    REQUIRE(partial.hasEdgeNodes());
    REQUIRE(partial.nodes == std::array<base::EntityId, 10>{10, 20, 30, 40, 50, 0, 60, 0, 70, 0});
    const SolidPentaCard prism = cardAt<SolidPentaTable>(model);
    REQUIRE(prism.nodes.size() == 15);
    REQUIRE_FALSE(prism.hasEdgeNodes());
    const SolidHexaCard brick = cardAt<SolidHexaTable>(model);
    REQUIRE(brick.nodes.size() == 20);
    REQUIRE(brick.nodes[8] == 9);
    REQUIRE(brick.nodes[10] == 0);
    REQUIRE(brick.nodes[11] == 0);
    REQUIRE(brick.nodes[12] == 30);
    REQUIRE(brick.nodes[19] == 60);
}

TEST_CASE("solid elements need their corners and refuse a repeated node", "[model][cards]")
{
    REQUIRE_THROWS_AS(readBulk("SolidTetra;1;2;10;20;30"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("SolidTetra;1;2;10;20;30;40;10"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("SolidPenta;1;2;1;2;3;4;5"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("SolidHexa;1;2;1;2;3;4;5;6;7"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("SolidHexa;1;2;1;2;3;4;5;6;7;8;9;2"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("SolidTetra;1;;10;20;30;40"), base::DeckError); // PID has no default
    // A blank edge node is not a repeat of another blank one.
    REQUIRE_NOTHROW(readBulk("SolidHexa;1;2;1;2;3;4;5;6;7;8;;;11;;13"));
}

TEST_CASE("solid elements list only the nodes they have as references", "[model][cards]")
{
    Model model = readBulk("SolidTetra;1;2;10;20;30;40;50");
    RecordingVisitor visitor;
    model.table<SolidTetraTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.size() == 6); // the property and five nodes
    REQUIRE(visitor.targets[0] == ReferenceTarget::Property);
    REQUIRE(visitor.targets[5] == ReferenceTarget::Node);
}

TEST_CASE("CONM2 defaults and the order of the inertia terms", "[model][cards]")
{
    const Model model =
        readBulk("PointMassInertia;1;5;;10.\nPointMassInertia;2;5;3;2.;1.;2.;3.;1.;2.;3.;4.;5.;6.\nPointMassInertia;3;5;-1;2.;1.;2.;3.");
    const PointMassInertiaCard minimal = cardAt<PointMassInertiaTable>(model, 0);
    REQUIRE(minimal.frameId == 0);
    REQUIRE(minimal.mass == 10.0);
    REQUIRE(minimal.offset == std::array<double, 3>{});
    REQUIRE(minimal.inertia == std::array<double, 6>{});
    const PointMassInertiaCard full = cardAt<PointMassInertiaTable>(model, 1);
    REQUIRE(full.frameId == 3);
    REQUIRE(full.offset == std::array<double, 3>{1.0, 2.0, 3.0});
    REQUIRE(full.inertia == std::array<double, 6>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}); // I11 I21 I22 I31 I32 I33
    REQUIRE(cardAt<PointMassInertiaTable>(model, 2).frameId == PointMassInertiaCard::BASIC_COORDINATES);
    REQUIRE_THROWS_AS(readBulk("PointMassInertia;1;5;-2;1."), base::DeckError);
}

TEST_CASE("CONM2 refers to its grid point and to its system when it has one", "[model][cards]")
{
    Model model = readBulk("PointMassInertia;1;5;-1;1.\nPointMassInertia;2;5;0;1.\nPointMassInertia;3;5;7;1.");
    // Grid point 5 is absent three times and system 7 once; -1 and 0 refer to no system.
    REQUIRE(unresolvedIn(model) == 4);
}

TEST_CASE("CELAS1 and CELAS2 take a grounded end or scalar points", "[model][cards]")
{
    const Model model = readBulk(
        "SpringDofProperty;1;;5;1\nSpringDofProperty;2;3;14;;15\nSpringDofInline;3;2.5;5;1\nSpringDofInline;4;5.;10;;11;;0.01;0.5");
    const SpringDofPropertyCard grounded = cardAt<SpringDofPropertyTable>(model, 0);
    REQUIRE(grounded.propId == 1); // PID defaults to EID
    REQUIRE(grounded.node2 == 0);
    REQUIRE(grounded.components2 == 0);
    const SpringDofPropertyCard scalar = cardAt<SpringDofPropertyTable>(model, 1);
    REQUIRE(scalar.propId == 3);
    REQUIRE(scalar.node1 == 14);
    REQUIRE(scalar.components1 == 0);
    REQUIRE(scalar.node2 == 15);
    const SpringDofInlineCard spring = cardAt<SpringDofInlineTable>(model, 0);
    REQUIRE(spring.stiffness == 2.5);
    REQUIRE(spring.structuralDamping == 0.0);
    REQUIRE(spring.stressCoefficient == 0.0);
    const SpringDofInlineCard damped = cardAt<SpringDofInlineTable>(model, 1);
    REQUIRE(damped.structuralDamping == 0.01);
    REQUIRE(damped.stressCoefficient == 0.5);
}

TEST_CASE("the two ends of a scalar spring must be different", "[model][cards]")
{
    REQUIRE_THROWS_AS(readBulk("SpringDofProperty;1;1;5;1;5;1"), base::DeckError);
    REQUIRE_NOTHROW(readBulk("SpringDofProperty;1;1;5;1;5;2"));
    REQUIRE_THROWS_AS(readBulk("SpringDofInline;1;1."), base::DeckError);     // ground to ground
    REQUIRE_THROWS_AS(readBulk("SpringDofInline;1;1.;5;7"), base::DeckError); // component 7
    REQUIRE_THROWS_AS(readBulk("SpringScalarProperty;1;1;7;7"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("SpringScalarInline;1;1."), base::DeckError); // S1 and S2 cannot both be blank
    REQUIRE_NOTHROW(readBulk("SpringScalarInline;1;1.;;8"));
}

TEST_CASE("scalar points need no card: a spring on them has no unresolved reference", "[model][cards]")
{
    Model model = readBulk("SpringDofInline;1;5.;14;;15\nSpringScalarInline;2;5.;14;15");
    REQUIRE(unresolvedIn(model) == 0);
    RecordingVisitor visitor;
    model.table<SpringScalarInlineTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"scalarPoint1", "scalarPoint2"});
    REQUIRE(visitor.targets[0] == ReferenceTarget::ScalarOrGridPoint);
    // The property of a CELAS1 or CELAS3 is still checked.
    Model withProperty = readBulk("SpringDofProperty;1;9;5;1\nSpringScalarProperty;2;9;14;15");
    REQUIRE(unresolvedIn(withProperty) == 2);
}

TEST_CASE("CBUSH reads GO, a vector or no orientation, and defaults the rest", "[model][cards]")
{
    const Model model =
        readBulk("Bushing;1;2;1;120;83\nBushing;2;;1;;;0.;1.;0.\nBushing;3;2;1;120;;;;;0\nBushing;4;2;1;120");
    const BushingCard byGrid = cardAt<BushingTable>(model, 0);
    REQUIRE(byGrid.orientationNode == 83);
    REQUIRE_FALSE(byGrid.hasVector());
    REQUIRE(byGrid.nodeB == 120);
    REQUIRE(byGrid.frameId == BushingCard::NO_FRAME_ID);
    REQUIRE(byGrid.offsetFraction == 0.5);
    REQUIRE(byGrid.offsetFrameId == -1);
    const BushingCard byVector = cardAt<BushingTable>(model, 1);
    REQUIRE(byVector.propId == 2); // PID defaults to EID
    REQUIRE(byVector.nodeB == 0);  // grounded
    REQUIRE(byVector.orientationNode == 0);
    REQUIRE(byVector.orientation == std::array<double, 3>{0.0, 1.0, 0.0});
    REQUIRE(byVector.hasVector());                         // X1 is 0.0 but the vector is not zero
    REQUIRE(cardAt<BushingTable>(model, 2).frameId == 0); // the basic system, not a blank
    const BushingCard axis = cardAt<BushingTable>(model, 3);
    REQUIRE(axis.orientationNode == 0);
    REQUIRE_FALSE(axis.hasVector());
}

TEST_CASE("CBUSH reads the spring-damper location and offset", "[model][cards]")
{
    const Model model = readBulk("Bushing;1;6;1;120;;;;;640;0.35;12;0.;12.;8.");
    const BushingCard card = cardAt<BushingTable>(model);
    REQUIRE(card.frameId == 640);
    REQUIRE(card.offsetFraction == 0.35);
    REQUIRE(card.offsetFrameId == 12);
    REQUIRE(card.offset == std::array<double, 3>{0.0, 12.0, 8.0});
    REQUIRE_THROWS_AS(readBulk("Bushing;1;6;1;120;;;;;;0.35;-2"), base::DeckError);
}

TEST_CASE("CBUSH references its system and offset system only when they are systems", "[model][cards]")
{
    Model model = readBulk("Bushing;1;6;1;120;83;;;;0;;-1\nBushing;2;6;1;120;;;;;640;;12");
    RecordingVisitor basic;
    model.table<BushingTable>().visitReferences(0, basic);
    REQUIRE(basic.fields == std::vector<std::string>{"propId", "nodeA", "nodeB", "orientationNode"});
    RecordingVisitor systems;
    model.table<BushingTable>().visitReferences(1, systems);
    REQUIRE(systems.fields == std::vector<std::string>{"propId", "nodeA", "nodeB", "frameId", "offsetFrameId"});
    REQUIRE(systems.targets[3] == ReferenceTarget::Coordinate);
    REQUIRE(systems.targets[4] == ReferenceTarget::Coordinate);
}
