// Checks of the scalar elements, concentrated masses, gap, bush, fastener, weld, beam, bend, plot, auxiliary output and
// general elements: variants, field positions, defaults, second elements on a card, references and rules.

#include "oor/model/cards/elements/bar2/Bar2Table.hpp"
#include "oor/model/cards/elements/bar2Stations/Bar2StationsTable.hpp"
#include "oor/model/cards/elements/beam3/Beam3Table.hpp"
#include "oor/model/cards/elements/bushing1D/Bushing1DTable.hpp"
#include "oor/model/cards/elements/shellConical2/ShellConical2Table.hpp"
#include "oor/model/cards/elements/curvedBeam2/CurvedBeam2Table.hpp"
#include "oor/model/cards/elements/damperDofProperty/DamperDofPropertyTable.hpp"
#include "oor/model/cards/elements/damperDofInline/DamperDofInlineTable.hpp"
#include "oor/model/cards/elements/damperScalarProperty/DamperScalarPropertyTable.hpp"
#include "oor/model/cards/elements/damperScalarInline/DamperScalarInlineTable.hpp"
#include "oor/model/cards/elements/fastener/FastenerTable.hpp"
#include "oor/model/cards/elements/gap2/Gap2Table.hpp"
#include "oor/model/cards/elements/generalMatrix/GeneralMatrixTable.hpp"
#include "oor/model/cards/elements/heatCapacity2/HeatCapacity2Table.hpp"
#include "oor/model/cards/elements/massDofProperty/MassDofPropertyTable.hpp"
#include "oor/model/cards/elements/massDofInline/MassDofInlineTable.hpp"
#include "oor/model/cards/elements/massScalarProperty/MassScalarPropertyTable.hpp"
#include "oor/model/cards/elements/massScalarInline/MassScalarInlineTable.hpp"
#include "oor/model/cards/elements/plotLine2/PlotLine2Table.hpp"
#include "oor/model/cards/elements/pointMassMatrix/PointMassMatrixTable.hpp"
#include "oor/model/cards/elements/planeStrainQuad/PlaneStrainQuadTable.hpp"
#include "oor/model/cards/elements/springScalarInline/SpringScalarInlineTable.hpp"
#include "oor/model/cards/elements/viscousDamper2/ViscousDamper2Table.hpp"
#include "oor/model/cards/elements/weld/WeldTable.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "oor/model/references/Renumbering.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
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

template <typename TableT>
std::size_t countOfCards(const Model& model)
{
    const TableT* table = model.findTable<TableT>();
    return table == nullptr ? 0 : table->rows().aliveCount();
}

std::size_t unresolvedIn(Model& model)
{
    base::DiagnosticSink sink(base::ReadMode::Tolerant);
    return ReferenceChecker::check(model, sink);
}

// The card record is an error in strict mode and in tolerant mode, where the first error names the field (when given).
void expectInvalid(const std::string& card, const std::string& field = "")
{
    INFO(card);
    REQUIRE_THROWS_AS(readModelText(card, true), base::DeckError);
    const Model tolerant = readModelText(card, false);
    REQUIRE(tolerant.diagnostics().hasErrors());
    if (!field.empty())
    {
        const auto error = tolerant.diagnostics().firstError();
        REQUIRE(error.has_value());
        INFO(error->message);
        REQUIRE(error->location.fieldName == field);
    }
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

TEST_CASE("CDAMP1, CMASS1 and CDAMP5 default PID to EID and take a blank point as ground", "[model][cards]")
{
    Model model =
        readBulk("DamperDofProperty;1;;8;1\nMassDofProperty;2;;8;1;9;3\nHeatCapacity2;3;;10;20\nMassDofProperty;4;7;14;;15");
    const DamperDofPropertyCard damper = cardAt<DamperDofPropertyTable>(model);
    REQUIRE(damper.propId == 1);
    REQUIRE(damper.node1 == 8);
    REQUIRE(damper.components1 == 1);
    REQUIRE(damper.node2 == 0);
    REQUIRE(damper.components2 == 0);
    const MassDofPropertyCard mass = cardAt<MassDofPropertyTable>(model, 0);
    REQUIRE(mass.propId == 2);
    REQUIRE(mass.node2 == 9);
    REQUIRE(mass.components2 == 3);
    REQUIRE(cardAt<MassDofPropertyTable>(model, 1).propId == 7);
    REQUIRE(cardAt<MassDofPropertyTable>(model, 1).components1 == 0); // a scalar point has no component
    const HeatCapacity2Card heat = cardAt<HeatCapacity2Table>(model);
    REQUIRE(heat.propId == 3);
    REQUIRE(heat.node1 == 10);
    REQUIRE(heat.node2 == 20);
    // The property is a reference; the points are grid or scalar points, which a scalar element may name without an
    // SPOINT: only the three missing properties (and the fourth) are reported.
    RecordingVisitor visitor;
    model.table<DamperDofPropertyTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propId", "node1"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::ScalarOrGridPoint});
    REQUIRE(unresolvedIn(model) == 4);
}

TEST_CASE("the scalar elements with a property refuse a point twice, a component above 6 and two grounded ends",
          "[model][cards]")
{
    expectInvalid("DamperDofProperty;1;;8;1;8;1", "node2");
    expectInvalid("MassDofProperty;1;;8;7", "components1");
    expectInvalid("DamperDofProperty;1;;0;0;0;0", "node2");
    expectInvalid("MassDofProperty;1;;8;1;9;-1", "components2");
    expectInvalid("HeatCapacity2;1;;10;10", "node2");
    expectInvalid("HeatCapacity2;1;;0;0", "node2");
    expectInvalid("DamperDofProperty;1;0;8;1", "propId");
    REQUIRE_NOTHROW(readBulk("DamperDofProperty;1;;8;1;8;2")); // the same point, another component
}

TEST_CASE("CDAMP2 and CMASS2 hold their own value, which is required", "[model][cards]")
{
    const Model model = readBulk(
        "DamperDofInline;1;3.46;27;1;23;4\nMassDofInline;2;7.75;6;1\nMassDofInline;3;4.5;6;1;7;1\nDamperDofInline;4;5.;10;;11");
    const DamperDofInlineCard damper = cardAt<DamperDofInlineTable>(model, 0);
    REQUIRE(damper.damping == 3.46);
    REQUIRE(damper.node1 == 27);
    REQUIRE(damper.components1 == 1);
    REQUIRE(damper.node2 == 23);
    REQUIRE(damper.components2 == 4);
    REQUIRE(cardAt<MassDofInlineTable>(model, 0).mass == 7.75);
    REQUIRE(cardAt<MassDofInlineTable>(model, 0).node2 == 0); // grounded
    REQUIRE(cardAt<MassDofInlineTable>(model, 1).node2 == 7);
    REQUIRE(cardAt<DamperDofInlineTable>(model, 1).node1 == 10);
    REQUIRE(cardAt<DamperDofInlineTable>(model, 1).components1 == 0);
    expectInvalid("DamperDofInline;1;;1;1", "damping");
    expectInvalid("MassDofInline;1;;1;1", "mass");
    expectInvalid("MassDofInline;1;2.;1;1;1;1", "node2");
    expectInvalid("DamperDofInline;1;2.;0;0", "node2");
    expectInvalid("DamperDofInline;1;2.;1;9", "components1");
}

TEST_CASE("CDAMP3 and CMASS3 connect two scalar points with a property that defaults to EID", "[model][cards]")
{
    Model model = readBulk("DamperScalarProperty;1;;14;15\nMassScalarProperty;2;47;58\nMassScalarProperty;3;;7;8");
    REQUIRE(cardAt<DamperScalarPropertyTable>(model).propId == 1);
    REQUIRE(cardAt<DamperScalarPropertyTable>(model).scalarPoint1 == 14);
    REQUIRE(cardAt<DamperScalarPropertyTable>(model).scalarPoint2 == 15);
    REQUIRE(cardAt<MassScalarPropertyTable>(model, 0).propId == 47);
    REQUIRE(cardAt<MassScalarPropertyTable>(model, 0).scalarPoint2 == 0);
    REQUIRE(cardAt<MassScalarPropertyTable>(model, 1).propId == 3);
    expectInvalid("DamperScalarProperty;1;;14;14", "scalarPoint2");
    expectInvalid("MassScalarProperty;1;;0", "scalarPoint2");
    expectInvalid("MassScalarProperty;1;;-4;5", "scalarPoint1");
    RecordingVisitor visitor;
    model.table<DamperScalarPropertyTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propId", "scalarPoint1", "scalarPoint2"});
    REQUIRE(visitor.targets[1] == ReferenceTarget::ScalarOrGridPoint);
}

TEST_CASE("CDAMP4, CMASS4 and CELAS4 may carry a second element in fields 5 to 8", "[model][cards]")
{
    const Model model = readBulk(
        "DamperScalarInline;1;2.5;5;6;2;3.5;7\n"
        "MassScalarInline;3;2.5;5;6;4;3.5;7;8\n"
        "SpringScalarInline;5;2.5;5;6;6;3.5;7;8\n"
        "DamperScalarInline;9;1.5;14\n"
        "SpringScalarInline;10;1.;3");
    REQUIRE(countOfCards<DamperScalarInlineTable>(model) == 3);
    REQUIRE(cardAt<DamperScalarInlineTable>(model, 0).id == 1);
    REQUIRE(cardAt<DamperScalarInlineTable>(model, 0).damping == 2.5);
    REQUIRE(cardAt<DamperScalarInlineTable>(model, 0).scalarPoint2 == 6);
    REQUIRE(cardAt<DamperScalarInlineTable>(model, 1).id == 2);
    REQUIRE(cardAt<DamperScalarInlineTable>(model, 1).damping == 3.5);
    REQUIRE(cardAt<DamperScalarInlineTable>(model, 1).scalarPoint1 == 7);
    REQUIRE(cardAt<DamperScalarInlineTable>(model, 1).scalarPoint2 == 0);
    REQUIRE(cardAt<DamperScalarInlineTable>(model, 2).id == 9);
    REQUIRE(countOfCards<MassScalarInlineTable>(model) == 2);
    REQUIRE(cardAt<MassScalarInlineTable>(model, 1).id == 4);
    REQUIRE(cardAt<MassScalarInlineTable>(model, 1).scalarPoint2 == 8);
    REQUIRE(countOfCards<SpringScalarInlineTable>(model) == 3);
    REQUIRE(cardAt<SpringScalarInlineTable>(model, 1).id == 6);
    REQUIRE(cardAt<SpringScalarInlineTable>(model, 1).stiffness == 3.5);
    REQUIRE(cardAt<SpringScalarInlineTable>(model, 2).id == 10);
    // Each element is a card of its own when written.

    oor::testing::requireModelFileRoundTrip(model);
    // The second element needs its EID and value; a third field group is not part of the card.
    expectInvalid("DamperScalarInline;1;2.5;5;6;2", "damping2");
    expectInvalid("DamperScalarInline;1;2.5;5;6;;3.5;7", "id2");
    REQUIRE_THROWS_AS(readBulk("DamperScalarInline;1;2.5;5;6;1;3.5;7"), base::DeckError); // the same element ID twice
    expectInvalid("DamperScalarInline;1;2.5;5;6;2;3.5;6;6", "scalarPoint2");
}

TEST_CASE("CVISC connects two different grid points with a property that defaults to EID", "[model][cards]")
{
    Model model = readBulk("ViscousDamper2;1;;26;34\nViscousDamper2;2;5418;26;34");
    REQUIRE(cardAt<ViscousDamper2Table>(model, 0).propId == 1);
    REQUIRE(cardAt<ViscousDamper2Table>(model, 0).node1 == 26);
    REQUIRE(cardAt<ViscousDamper2Table>(model, 1).propId == 5418);
    expectInvalid("ViscousDamper2;1;;26;26", "node2");
    expectInvalid("ViscousDamper2;1;;26", "node2");
    expectInvalid("ViscousDamper2;1;;0;5", "node1");
    RecordingVisitor visitor;
    model.table<ViscousDamper2Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propId", "node1", "node2"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Node, ReferenceTarget::Node});
    // Two properties and four grid points are missing (a CVISC has no scalar points).
    REQUIRE(unresolvedIn(model) == 2 + 4);
}

TEST_CASE("CONM1 keeps the lower triangle of its 6 x 6 matrix in the order of the card", "[model][cards]")
{
    const Model model = readBulk(
        "PointMassMatrix;1;24;3;1.1;2.1;2.2;3.1;3.2;3.3;4.1;4.2;4.3;4.4;5.1;5.2;5.3;5.4;5.5;6.1;6.2;6.3;6.4;6.5;6.6\n"
        "PointMassMatrix;2;5");
    const PointMassMatrixCard full = cardAt<PointMassMatrixTable>(model, 0);
    REQUIRE(full.node == 24);
    REQUIRE(full.frameId == 3);
    REQUIRE(full.massMatrix[PointMassMatrixCard::indexOf(1, 1)] == 1.1);
    REQUIRE(full.massMatrix[PointMassMatrixCard::indexOf(2, 1)] == 2.1);
    REQUIRE(full.massMatrix[PointMassMatrixCard::indexOf(2, 2)] == 2.2);
    REQUIRE(full.massMatrix[PointMassMatrixCard::indexOf(3, 1)] == 3.1);
    REQUIRE(full.massMatrix[PointMassMatrixCard::indexOf(3, 3)] == 3.3); // the first value of the continuation line
    REQUIRE(full.massMatrix[PointMassMatrixCard::indexOf(4, 4)] == 4.4);
    REQUIRE(full.massMatrix[PointMassMatrixCard::indexOf(5, 5)] == 5.5);
    REQUIRE(full.massMatrix[PointMassMatrixCard::indexOf(6, 1)] == 6.1);
    REQUIRE(full.massMatrix[PointMassMatrixCard::indexOf(6, 6)] == 6.6);
    REQUIRE(PointMassMatrixCard::indexOf(1, 1) == 0);
    REQUIRE(PointMassMatrixCard::indexOf(6, 6) == PointMassMatrixCard::VALUE_COUNT - 1);
    // The values are the fields 4 to 24 one after the other: they are what the card has when it is written.

    oor::testing::requireModelFileRoundTrip(model);
    // Blank is zero, and zero is left blank.
    const PointMassMatrixCard blank = cardAt<PointMassMatrixTable>(model, 1);
    REQUIRE(blank.frameId == 0);
    for (const double value : blank.massMatrix)
        REQUIRE(value == 0.0);
    expectInvalid("PointMassMatrix;1;0", "node");
    expectInvalid("PointMassMatrix;1;5;-1", "frameId");
}

TEST_CASE("CGAP takes its orientation as a vector or a grid point and a blank CID differs from 0", "[model][cards]")
{
    const Model model = readBulk(
        "Gap2;1;2;120;124;;4.6;0.4;-5.3\n"
        "Gap2;2;2;120;124;13\n"
        "Gap2;3;;120;124;;;;;5\n"
        "Gap2;4;2;120;124;;0.;1.;0.;0\n"
        "Gap2;5;2;120;124;;;;1.\n"
        "Gap2;6;2;120;124");
    const Gap2Card vector = cardAt<Gap2Table>(model, 0);
    REQUIRE(vector.orientation == std::array<double, 3>{4.6, 0.4, -5.3});
    REQUIRE(vector.orientationNode == 0);
    REQUIRE(vector.frameId == Gap2Card::NO_FRAME_ID);
    REQUIRE(cardAt<Gap2Table>(model, 1).orientationNode == 13);
    REQUIRE_FALSE(cardAt<Gap2Table>(model, 1).hasVector());
    const Gap2Card system = cardAt<Gap2Table>(model, 2);
    REQUIRE(system.propId == 3); // PID defaults to EID
    REQUIRE(system.frameId == 5);
    REQUIRE_FALSE(system.hasVector());
    REQUIRE(cardAt<Gap2Table>(model, 3).frameId == 0); // the basic system, not blank
    REQUIRE(cardAt<Gap2Table>(model, 3).orientation == std::array<double, 3>{0.0, 1.0, 0.0});
    // X2 and X3 are read when X1 is blank.
    REQUIRE(cardAt<Gap2Table>(model, 4).orientation == std::array<double, 3>{0.0, 0.0, 1.0});
    REQUIRE_FALSE(cardAt<Gap2Table>(model, 5).hasVector());

    oor::testing::requireModelFileRoundTrip(model);
    expectInvalid("Gap2;1;2;120;120", "nodeB");
    expectInvalid("Gap2;1;2;120", "nodeB");
    expectInvalid("Gap2;1;2", "nodeA");
    expectInvalid("Gap2;1;2;120;124;0", "orientationNode");
    expectInvalid("Gap2;1;2;120;124;;;;;-3", "frameId");
    // The two forms of the orientation are exclusive when the card is built from values.
    Gap2Card both;
    both.id = 1;
    both.propId = 1;
    both.nodeA = 1;
    both.nodeB = 2;
    both.orientationNode = 3;
    both.orientation = {0.0, 0.0, 1.0};
    REQUIRE(Gap2Format::check(both).has_value());
    both.orientation = {};
    REQUIRE_FALSE(Gap2Format::check(both).has_value());
}

TEST_CASE("CBUSH1D has an optional second point and an optional coordinate system", "[model][cards]")
{
    Model model = readBulk("Bushing1D;1;104;107;113\nBushing1D;2;;107;;5\nBushing1D;3;104;107;113;0");
    REQUIRE(cardAt<Bushing1DTable>(model, 0).frameId == Bushing1DCard::NO_FRAME_ID);
    REQUIRE(cardAt<Bushing1DTable>(model, 1).propId == 2);
    REQUIRE(cardAt<Bushing1DTable>(model, 1).nodeB == 0);
    REQUIRE(cardAt<Bushing1DTable>(model, 1).frameId == 5);
    REQUIRE(cardAt<Bushing1DTable>(model, 2).frameId == 0);
    expectInvalid("Bushing1D;1;104;107;107", "nodeB");
    expectInvalid("Bushing1D;1;104", "nodeA");
    expectInvalid("Bushing1D;1;104;107;113;-2", "frameId");
    RecordingVisitor visitor;
    model.table<Bushing1DTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propId", "nodeA", "frameId"}); // no GB: grounded
}

TEST_CASE("CFAST locates the fastener by GS, by the piercing points or by XS, YS and ZS", "[model][cards]")
{
    Model model = readBulk(
        "Fastener;1;30;Property;31;35;412\nFastener;2;;Elements;36;58;;613;614\nFastener;3;60;Elements;36;"
        "58;;;;1.5;2."
        "5;3.5");
    const FastenerCard property = cardAt<FastenerTable>(model, 0);
    REQUIRE(property.kind == "Property");
    REQUIRE(property.patchIdA == 31);
    REQUIRE(property.patchIdB == 35);
    REQUIRE(property.spotNode == 412);
    REQUIRE_FALSE(property.hasLocation());
    const FastenerCard element = cardAt<FastenerTable>(model, 1);
    REQUIRE(element.propId == 2);
    REQUIRE(element.nodeA == 613);
    REQUIRE(element.nodeB == 614);
    const FastenerCard located = cardAt<FastenerTable>(model, 2);
    REQUIRE(located.location == std::array<double, 3>{1.5, 2.5, 3.5});
    REQUIRE(located.spotNode == 0);
    expectInvalid("Fastener;1;30;WELD;31;35;412", "kind");
    expectInvalid("Fastener;1;30;Property;31;35", "spotNode");         // no location at all
    expectInvalid("Fastener;1;30;Property;31;35;;;;1.5", "locationX"); // a part of the location
    expectInvalid("Fastener;1;30;Property;31", "patchIdB");
    // IDA and IDB are two properties for PROP and two shell elements for ELEM.
    RecordingVisitor property_visitor;
    model.table<FastenerTable>().visitReferences(0, property_visitor);
    REQUIRE(property_visitor.fields == std::vector<std::string>{"propId", "patchIdA", "patchIdB", "spotNode"});
    REQUIRE(property_visitor.targets[1] == ReferenceTarget::Property);
    RecordingVisitor element_visitor;
    model.table<FastenerTable>().visitReferences(1, element_visitor);
    REQUIRE(element_visitor.targets[1] == ReferenceTarget::Element);
    REQUIRE(element_visitor.targets[2] == ReferenceTarget::Element);
}

TEST_CASE("CWELD has five formats selected by TYPE, each with its own fields", "[model][cards]")
{
    const Model model = readBulk(
        "Weld;1;8;207;PartPatch;405;406;9;24;36;1.5;2.5;3.5\n"
        "Weld;2;5;407;ElemPatch;;;;314;527\n"
        "Weld;3;5;407;ElemIdPatch;;;;314\n"
        "Weld;4;26;241;NodeIdPatch;;;QuadTriaPatch;;12;24;33;37;42;53;57;64;3;5;8\n"
        "Weld;5;26;;AlignedNodes;113;262;3");
    const WeldCard partpat = cardAt<WeldTable>(model, 0);
    REQUIRE(partpat.kind == "PartPatch");
    REQUIRE(partpat.propId == 8);
    REQUIRE(partpat.spotNode == 207);
    REQUIRE(partpat.nodeA == 405);
    REQUIRE(partpat.nodeB == 406);
    REQUIRE(partpat.matFrameId == 9);
    REQUIRE(partpat.patchIdA == 24);
    REQUIRE(partpat.patchIdB == 36);
    REQUIRE(partpat.location == std::array<double, 3>{1.5, 2.5, 3.5});
    const WeldCard elpat = cardAt<WeldTable>(model, 1);
    REQUIRE(elpat.matFrameId == WeldCard::NO_MAT_FRAME_ID); // blank is -1
    REQUIRE(elpat.patchIdA == 314);
    REQUIRE(elpat.patchIdB == 527);
    REQUIRE_FALSE(elpat.hasLocation());
    const WeldCard elemid = cardAt<WeldTable>(model, 2);
    REQUIRE(elemid.patchIdA == 314);
    REQUIRE(elemid.patchIdB == 0); // a point connected to the patch A
    const WeldCard grid = cardAt<WeldTable>(model, 3);
    REQUIRE(grid.patchShapes == "QuadTriaPatch");
    REQUIRE(grid.patchNodesA == std::array<base::EntityId, 8>{12, 24, 33, 37, 42, 53, 57, 64});
    REQUIRE(grid.patchNodesB == std::array<base::EntityId, 8>{3, 5, 8, 0, 0, 0, 0, 0});
    const WeldCard align = cardAt<WeldTable>(model, 4);
    REQUIRE(align.nodeA == 113);
    REQUIRE(align.nodeB == 262);
    REQUIRE(align.matFrameId == 3);
    REQUIRE(align.spotNode == 0);
    // MCID is in field 8 and the formats write what they use and nothing else.

    oor::testing::requireModelFileRoundTrip(model);
}

TEST_CASE("the CWELD formats refuse what they do not have and require what they need", "[model][cards]")
{
    // PARTPAT and ELPAT need two patches and a location.
    expectInvalid("Weld;1;8;207;PartPatch;;;;24", "propIdB");
    expectInvalid("Weld;1;8;207;ElemPatch;;;;24", "shellIdB");
    expectInvalid("Weld;1;8;207;PartPatch;;;;24;24", "patchIdB");
    expectInvalid("Weld;1;8;;PartPatch;;;;24;36", "spotNode");
    expectInvalid("Weld;1;8;;PartPatch;;;;24;36;1.5;2.5", "locationX");
    REQUIRE_NOTHROW(readBulk("Weld;1;8;;PartPatch;405;;;24;36")); // GA locates it
    // ELEMID needs only the first element.
    expectInvalid("Weld;1;8;207;ElemIdPatch", "shellIdA");
    // Fields of another format are an error, not silently dropped.
    // GRIDID: SPTYP, three grid points of patch A, three of patch B (none for a point to patch).
    expectInvalid("Weld;1;26;241;NodeIdPatch;;;XX;;12;24;33", "patchShapes");
    expectInvalid("Weld;1;26;241;NodeIdPatch;;;QuadQuadPatch;;12;24", "nodeA1");
    expectInvalid("Weld;1;26;241;NodeIdPatch;;;QuadQuadPatch;;12;24;33", "nodeB1");
    expectInvalid("Weld;1;26;241;NodeIdPatch;;;QuadPatch;;12;24;33;;;;;;3", "nodeB1");
    REQUIRE_NOTHROW(readBulk("Weld;1;26;241;NodeIdPatch;;;QuadPatch;;12;24;33;37"));
    // ALIGN needs GA and GB.
    expectInvalid("Weld;1;26;;AlignedNodes;113", "nodeA");
    expectInvalid("Weld;1;26;;AlignedNodes;113;113", "nodeB");
    expectInvalid("Weld;1;26;;AlignedNodes;113;262;-2", "matFrameId");
}

TEST_CASE("CWELD refers to a property, its points and, by TYPE, to two properties or two shell elements",
          "[model][cards]")
{
    Model model = readBulk(
        "Weld;1;8;207;PartPatch;405;406;9;24;36;1.5;2.5;3.5\n"
        "Weld;2;5;407;ElemPatch;;;;314;527\n"
        "Weld;4;26;241;NodeIdPatch;;;QuadTriaPatch;;12;24;33;37;42;53;57;64;3;5;8");
    RecordingVisitor partpat;
    model.table<WeldTable>().visitReferences(0, partpat);
    REQUIRE(partpat.fields ==
            std::vector<std::string>{"propId", "spotNode", "nodeA", "nodeB", "matFrameId", "patchIdA", "patchIdB"});
    REQUIRE(partpat.targets[5] == ReferenceTarget::Property);
    RecordingVisitor elpat;
    model.table<WeldTable>().visitReferences(1, elpat);
    REQUIRE(elpat.targets.back() == ReferenceTarget::Element);
    RecordingVisitor grid;
    model.table<WeldTable>().visitReferences(2, grid);
    REQUIRE(grid.fields.size() == 2 + 8 + 3); // PWID, GS, the 8 + 3 listed grid points
}

TEST_CASE("CBEAM3 reads its offsets, pretwist and warping points from the continuation lines", "[model][cards]")
{
    Model model = readBulk(
        "Beam3;1;2;211;342;1100;;1.;2.5;-3.;2.;4.;1.8;-1.5;3.5;12.;14.;18.;1.5;0.1;0.2;0.3;216;311;322\n"
        "Beam3;2;2;211;342;;;0.;1.;0.\n"
        "Beam3;3;2;211;342;1100;115");
    const Beam3Card full = cardAt<Beam3Table>(model, 0);
    REQUIRE(full.nodeC == 1100);
    REQUIRE(full.orientation == std::array<double, 3>{1.0, 2.5, -3.0});
    REQUIRE(full.offsetA == std::array<double, 3>{2.0, 4.0, 1.8});
    REQUIRE(full.offsetB == std::array<double, 3>{-1.5, 3.5, 12.0});
    REQUIRE(full.offsetC == std::array<double, 3>{14.0, 18.0, 1.5}); // W3C is the first field of the last line
    REQUIRE(full.twist == std::array<double, 3>{0.1, 0.2, 0.3});
    REQUIRE(full.warpingPointA == 216);
    REQUIRE(full.warpingPointB == 311);
    REQUIRE(full.warpingPointC == 322);
    REQUIRE(cardAt<Beam3Table>(model, 1).nodeC == 0);
    REQUIRE(cardAt<Beam3Table>(model, 2).orientationNode == 115);

    oor::testing::requireModelFileRoundTrip(model);
    // The property and the orientation are required, and GO is not one of the three points.
    expectInvalid("Beam3;1;;211;342;1100;;1.;0.;0.", "propId");
    expectInvalid("Beam3;1;2;211;342;1100", "orientationNode");
    expectInvalid("Beam3;1;2;211;342;1100;;0.;0.;0.", "orientationNode");
    expectInvalid("Beam3;1;2;211;342;1100;211", "orientationNode");
    expectInvalid("Beam3;1;2;211;342;1100;1100", "orientationNode");
    expectInvalid("Beam3;1;2;211;211;;;1.;0.;0.", "nodeB");
    expectInvalid("Beam3;1;2;211;342;211;;1.;0.;0.", "nodeB");
    // SA, SB and SC are grid or scalar points.
    RecordingVisitor visitor;
    model.table<Beam3Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propId", "nodeA", "nodeB", "nodeC", "warpingPointA",
                                                       "warpingPointB", "warpingPointC"});
    REQUIRE(visitor.targets.back() == ReferenceTarget::ScalarOrGridPoint);
}

TEST_CASE("CBEND requires its orientation and GEOM", "[model][cards]")
{
    const Model model = readBulk(
        "CurvedBeam2;1;43;18;22;;5.4;4.3;-1.6;3\nCurvedBeam2;2;;18;22;108;;;;1\nCurvedBeam2;3;4;1;2;;0.5;1.E+"
        "6;;1");
    REQUIRE(cardAt<CurvedBeam2Table>(model, 0).geometryOption == 3);
    REQUIRE(cardAt<CurvedBeam2Table>(model, 0).orientation == std::array<double, 3>{5.4, 4.3, -1.6});
    REQUIRE(cardAt<CurvedBeam2Table>(model, 1).propId == 2);
    REQUIRE(cardAt<CurvedBeam2Table>(model, 1).orientationNode == 108);
    REQUIRE(cardAt<CurvedBeam2Table>(model, 2).orientation == std::array<double, 3>{0.5, 1.0e6, 0.0});
    expectInvalid("CurvedBeam2;1;43;18;22;;5.4;4.3;-1.6", "geometryOption");
    expectInvalid("CurvedBeam2;1;43;18;22;;5.4;4.3;-1.6;5", "geometryOption");
    expectInvalid("CurvedBeam2;1;43;18;22;;;;;3", "orientationNode");
    expectInvalid("CurvedBeam2;1;43;18;22;18;;;;3", "orientationNode");
    expectInvalid("CurvedBeam2;1;43;18;18;;5.4;4.3;-1.6;3", "nodeB");
}

TEST_CASE("PLOTEL may carry a second element and has an ID space of its own", "[model][cards]")
{
    Model model =
        readBulk("PlotLine2;1;37;18\nPlotLine2;2;1;2;3;2;3\nBar2;3;1;2;3;0.;;0.;1.\nPlotLine2;4;5;6");
    REQUIRE(countOfCards<PlotLine2Table>(model) == 4);
    REQUIRE(cardAt<PlotLine2Table>(model, 0).node1 == 37);
    REQUIRE(cardAt<PlotLine2Table>(model, 1).id == 2);
    REQUIRE(cardAt<PlotLine2Table>(model, 2).id == 3);
    REQUIRE(cardAt<PlotLine2Table>(model, 2).node2 == 3);
    // A PLOTEL with the ID of a bar does not replace it (the format reference asks for distinct IDs; decks that ran in
    // a solver do not always keep them): both are there. Two PLOTELs cannot have the same ID.
    REQUIRE(countOfCards<Bar2Table>(model) == 1);
    REQUIRE_THROWS_AS(readBulk("PlotLine2;1;37;18\nPlotLine2;1;1;2"), base::DeckError);

    oor::testing::requireModelFileRoundTrip(model);
    expectInvalid("PlotLine2;1;37;37", "node2");
    expectInvalid("PlotLine2;1;37", "node2");
    expectInvalid("PlotLine2;1;1;2;3;2", "node4");
    // The grid points are references.
    RecordingVisitor visitor;
    model.table<PlotLine2Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"node1", "node2"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Node, ReferenceTarget::Node});
}

TEST_CASE("CBARAO lists up to six stations or gives equally spaced ones, and refers to a bar", "[model][cards]")
{
    Model model = readBulk(
        "Bar2Stations;1;FractionalLength;0.15;;0.35;0.55;0.85\nBar2Stations;2;ActualLength;;5;0.1;0."
        "2\nBar2Stations;3;FractionalLength;0.5;;;0."
        "9\nBar2;1;1;1;2;0.;;0.;1.");
    const Bar2StationsCard list = cardAt<Bar2StationsTable>(model, 0);
    REQUIRE(list.stationScale == "FractionalLength");
    REQUIRE(list.listedStations == std::vector<double>{0.15, 0.35, 0.55, 0.85});
    REQUIRE_FALSE(list.isSpaced());
    const Bar2StationsCard spaced = cardAt<Bar2StationsTable>(model, 1);
    REQUIRE(spaced.stationScale == "ActualLength");
    REQUIRE(spaced.isSpaced());
    REQUIRE(spaced.stationCount == 5);
    REQUIRE(spaced.first == 0.1);
    REQUIRE(spaced.stationSpacing == 0.2);
    REQUIRE(spaced.listedStations.empty());
    const std::vector<double> positions = spaced.positions();
    REQUIRE(positions.size() == 5);
    REQUIRE(std::abs(positions[4] - 0.9) < 1e-12);
    REQUIRE(cardAt<Bar2StationsTable>(model, 2).listedStations ==
            std::vector<double>{0.5, 0.9}); // a blank field is skipped, SCALE is upper case
    // Each form is written back as it is.

    oor::testing::requireModelFileRoundTrip(model);
    expectInvalid("Bar2Stations;1;XX;0.2", "stationScale");
    expectInvalid("Bar2Stations;1;FractionalLength", "station1");
    expectInvalid("Bar2Stations;1;FractionalLength;0.;;0.5", "station1");
    expectInvalid("Bar2Stations;1;FractionalLength;;-4;0.2;0.2", "stationCount");
    expectInvalid("Bar2Stations;1;FractionalLength;;4;0.2", "stationSpacing");
    expectInvalid("Bar2Stations;1;FractionalLength;;4;0.2;0.2;0.3", "stationSpacing");
    REQUIRE_THROWS_AS(readBulk("Bar2Stations;1;FractionalLength;0.2\nBar2Stations;1;ActualLength;0.3"),
                      base::DeckError);
    // It refers to the CBAR whose ID it carries; it does not define an element of that ID.
    RecordingVisitor visitor;
    model.table<Bar2StationsTable>().visitReferences(0, visitor);
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Element});
    // The CBARAO of the bar 1 resolves; those of the bars 2 and 3 do not, and the CBAR 1 misses its property and
    // points.
    REQUIRE(unresolvedIn(model) == 2 + 3);
}

TEST_CASE("renumbering the elements renumbers the CBAR that a CBARAO refers to", "[model][cards]")
{
    Model model =
        readBulk("Bar2;10;1;1;2;0.;;0.;1.\nBar2Stations;10;FractionalLength;0.5\nBar2;20;1;2;3;0.;;0.;1.");
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(100));
    REQUIRE(cardAt<Bar2Table>(model, 0).id == 110);
    REQUIRE(cardAt<Bar2StationsTable>(model).id == 110);
    REQUIRE(unresolvedIn(model) == 2 + 4); // the properties and the grid points are missing, not the bar of the CBARAO
}

TEST_CASE("CQUAD takes four corners, edge points, a centre point and THETA or MCID", "[model][cards]")
{
    Model model = readBulk(
        "PlaneStrainQuad;1;205;34;76;78;35\nPlaneStrainQuad;2;205;34;76;78;35;46;47;48;49;51;30.\nPlaneStrainQuad;3;205;34;"
        "76;78;35;;;;;;;7");
    REQUIRE(cardAt<PlaneStrainQuadTable>(model, 0).nodes == std::array<base::EntityId, 9>{34, 76, 78, 35, 0, 0, 0, 0, 0});
    REQUIRE(cardAt<PlaneStrainQuadTable>(model, 1).nodes ==
            std::array<base::EntityId, 9>{34, 76, 78, 35, 46, 47, 48, 49, 51});
    REQUIRE(cardAt<PlaneStrainQuadTable>(model, 1).thetaInDegrees == 30.0);
    REQUIRE(cardAt<PlaneStrainQuadTable>(model, 2).matFrameId == 7);
    expectInvalid("PlaneStrainQuad;1;205;34;76;78", "node4");
    expectInvalid("PlaneStrainQuad;1;;34;76;78;35", "propId");
    expectInvalid("PlaneStrainQuad;1;205;34;76;78;34", "node1");
    RecordingVisitor visitor;
    model.table<PlaneStrainQuadTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields.size() == 1 + 9);
    REQUIRE(visitor.targets.front() == ReferenceTarget::Property);
}

TEST_CASE("GENEL is laid out in parts that each start on a line of the card", "[model][cards]")
{
    const std::string deck =
        "GeneralMatrix;517;4;1;1;15;3;46;0;27;5;DependentDofs;2;8;4;36;0;Flexibility;10;1.;2.;3.;4.;5.;6.;7.;8.;9.;10.;"
        "Dependency;8;1.5;2.5;3.5;4.5;5."
        "5;6.5;7.5;8.5";
    const Model model = readBulk(deck);
    const GeneralMatrixCard card = cardAt<GeneralMatrixTable>(model);
    REQUIRE(card.independentDofs.size() == 4);
    REQUIRE(card.independentDofs[1] == std::array<base::EntityId, 2>{15, 3});
    REQUIRE(card.independentDofs[2] == std::array<base::EntityId, 2>{46, 0}); // a scalar point: component 0
    REQUIRE(card.independentDofs[3] == std::array<base::EntityId, 2>{27, 5}); // the first pair of the continuation line
    REQUIRE(card.dependentDofs.size() == 2);
    REQUIRE(card.dependentDofs[0] == std::array<base::EntityId, 2>{8, 4});
    REQUIRE(card.isFlexibility);
    REQUIRE(card.stiffnessMatrix.size() == 10);
    REQUIRE(card.stiffnessMatrix.back() == 10.0);
    REQUIRE(card.dependencyMatrix.size() == 8);
    REQUIRE(card.dependencyMatrix.back() == 8.5);
    // Written back, each part starts a line: "UD" at field 17, the matrix at field 25, "S" at field 41.

    oor::testing::requireModelFileRoundTrip(model);
    // The lower triangle must have the right number of values for the number of degrees of freedom.
    expectInvalid("GeneralMatrix;1;2;1;1;13;4;Stiffness;2;2.;0.5", "stiffness");
    expectInvalid("GeneralMatrix;1;2;1;1;13;4;Stiffness;4;2.;0.5;3.;4.", "stiffness");
    expectInvalid("GeneralMatrix;1;2;1;1;13;4;Stiffness;3;2.;0.5;3.;Dependency;2;1.;2.", "dependentPoint1");
    expectInvalid("GeneralMatrix;1;2;1;1;13;4;DependentDofs;2;5;1;5;2;Stiffness;3;2.;0.5;3.",
                  "dependentPoint1"); // UD without S needs six
    expectInvalid("GeneralMatrix;1;2;1;1;13;4;DependentDofs;2;5;1;5;2;Stiffness;3;2.;0.5;3.;Dependency;3;1.;2.;3.",
                  "dependencyMatrix");
    expectInvalid("GeneralMatrix;1;2;1;1;13;4", "stiffness");
    expectInvalid("GeneralMatrix;1;2;1;1;13;9;Stiffness;3;2.;0.5;3.", "independentPoint");
    expectInvalid("GeneralMatrix;1;2;1;1;13;4;Stiffness;3;2.;0.5;3.;Stiffness;1;1.", "stiffness");
    expectInvalid("GeneralMatrix;1;2;1;1;13;4;Flexibility;3;2.;0.5;3.;DependentDofs;1;5;1", "dependentPoint");
    expectInvalid("GeneralMatrix;1;2;1;1;13;4;Stiffness;3;2.;0.5;x", "stiffness");
}

TEST_CASE("the points of a GENEL are grid or scalar points that renumbering follows", "[model][cards]")
{
    Model model = readBulk("Node;1;;0.;0.;0.\nScalarPoint;13\nGeneralMatrix;1;2;1;1;13;0;Stiffness;3;2.;0.5;3.");
    RecordingVisitor visitor;
    model.table<GeneralMatrixTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"independentPoint", "independentPoint"});
    REQUIRE(visitor.targets.front() == ReferenceTarget::ScalarOrGridPoint);
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    const GeneralMatrixCard card = cardAt<GeneralMatrixTable>(model);
    REQUIRE(card.independentDofs[0] == std::array<base::EntityId, 2>{101, 1});
    REQUIRE(card.independentDofs[1] == std::array<base::EntityId, 2>{113, 0});
}

TEST_CASE("CCONEAX defaults PID to EID and names two rings", "[model][cards]")
{
    Model model = readBulk("ShellConical2;1;;1200;1201\nShellConical2;2;5;3;4");
    REQUIRE(cardAt<ShellConical2Table>(model, 0).propId == 1);
    REQUIRE(cardAt<ShellConical2Table>(model, 0).ringIdA == 1200);
    REQUIRE(cardAt<ShellConical2Table>(model, 1).propId == 5);
    expectInvalid("ShellConical2;1;;3;3", "ringIdB");
    expectInvalid("ShellConical2;1;;3", "ringIdB");
    // The rings are RINGAX points of the node ID space: the deck has none, so they are unresolved.
    RecordingVisitor visitor;
    model.table<ShellConical2Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propId", "ringIdA", "ringIdB"});
    REQUIRE(visitor.targets[1] == ReferenceTarget::Node);
    REQUIRE(unresolvedIn(model) == 6); // the two properties and the four rings
}
