// Checks of the shell, plate, plane, axisymmetric and solid elements: variants, field positions, blank
// edge points, defaults and references.

#include "oor/model/cards/elements/axisymQuad/AxisymQuadTable.hpp"
#include "oor/model/cards/elements/axisymQuad4/AxisymQuad4Table.hpp"
#include "oor/model/cards/elements/axisymQuad8/AxisymQuad8Table.hpp"
#include "oor/model/cards/elements/axisymTria/AxisymTriaTable.hpp"
#include "oor/model/cards/elements/axisymTria6/AxisymTria6Table.hpp"
#include "oor/model/cards/elements/planeStrainTria3/PlaneStrainTria3Table.hpp"
#include "oor/model/cards/elements/planeStrainQuad4/PlaneStrainQuad4Table.hpp"
#include "oor/model/cards/elements/planeStrainTria6/PlaneStrainTria6Table.hpp"
#include "oor/model/cards/elements/planeStrainQuad8/PlaneStrainQuad8Table.hpp"
#include "oor/model/cards/elements/planeStressTria3/PlaneStressTria3Table.hpp"
#include "oor/model/cards/elements/planeStressQuad4/PlaneStressQuad4Table.hpp"
#include "oor/model/cards/elements/planeStressTria6/PlaneStressTria6Table.hpp"
#include "oor/model/cards/elements/planeStressQuad8/PlaneStressQuad8Table.hpp"
#include "oor/model/cards/elements/solidPyramid/SolidPyramidTable.hpp"
#include "oor/model/cards/elements/shellQuad8/ShellQuad8Table.hpp"
#include "oor/model/cards/elements/shellQuad4Drilling/ShellQuad4DrillingTable.hpp"
#include "oor/model/cards/elements/rod2/Rod2Table.hpp"
#include "oor/model/cards/elements/rod2InlineSection/Rod2InlineSectionTable.hpp"
#include "oor/model/cards/elements/shearPanel4/ShearPanel4Table.hpp"
#include "oor/model/cards/elements/shellTria6/ShellTria6Table.hpp"
#include "oor/model/cards/elements/shellTria3Drilling/ShellTria3DrillingTable.hpp"
#include "oor/model/cards/elements/tube2/Tube2Table.hpp"
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

std::size_t countOf(const std::vector<std::string>& fields, const std::string& name)
{
    std::size_t count = 0;
    for (const std::string& field : fields)
        count += field == name ? 1 : 0;
    return count;
}

} // namespace

TEST_CASE("CTRIA6 has three corners, optional edge points and the shell fields after them", "[model][cards]")
{
    const Model model =
        readBulk("ShellTria6;1;3;34;36;76;35;;57;40.;;0.04;0.03;0.035;0.035;1\nShellTria6;2;3;1;2;3;;;;;7");
    const ShellTria6Card full = cardAt<ShellTria6Table>(model, 0);
    REQUIRE(full.propId == 3);
    REQUIRE(full.nodes == std::array<base::EntityId, 6>{34, 36, 76, 35, 0, 57});
    REQUIRE(full.hasEdgeNodes());
    REQUIRE(full.thetaInDegrees == 40.0);
    REQUIRE_FALSE(full.usesMaterialSystem());
    REQUIRE(full.offset == 0.04);
    REQUIRE(full.cornerThickness == std::array<double, 3>{0.03, 0.035, 0.035});
    REQUIRE(full.thicknessFlag == 1);
    const ShellTria6Card byMcid = cardAt<ShellTria6Table>(model, 1);
    REQUIRE(byMcid.nodes == std::array<base::EntityId, 6>{1, 2, 3, 0, 0, 0});
    REQUIRE_FALSE(byMcid.hasEdgeNodes());
    REQUIRE(byMcid.usesMaterialSystem());
    REQUIRE(byMcid.matFrameId == 7);
    REQUIRE(byMcid.cornerThickness == std::array<double, 3>{-1.0, -1.0, -1.0});
}

TEST_CASE("a blank edge point is a blank field when a CTRIA6 is written back", "[model][cards]")
{
    const Model model = readBulk("ShellTria6;1;3;34;36;76;;53");

    oor::testing::requireModelFileRoundTrip(model);
    REQUIRE(cardAt<ShellTria6Table>(readBulk("ShellTria6;1;3;34;36;76;;53")).nodes ==
            std::array<base::EntityId, 6>{34, 36, 76, 0, 53, 0});
}

TEST_CASE("CTRIA6 and CQUAD8 refer to their property, their grid points and a material system", "[model][cards]")
{
    Model model = readBulk("ShellTria6;1;3;34;36;76;;53\nShellQuad8;2;3;1;2;3;4;;;;;;;;;;9");
    // PID 3, nodes 34 36 76 53 (and 1 2 3 4), no coordinate system on the first.
    RecordingVisitor tria;
    model.table<ShellTria6Table>().visitReferences(0, tria);
    REQUIRE(tria.fields.front() == "propId");
    REQUIRE(countOf(tria.fields, "nodes") == 4); // the absent edge points are not references
    REQUIRE(countOf(tria.fields, "matFrameId") == 0);
    RecordingVisitor quad;
    model.table<ShellQuad8Table>().visitReferences(0, quad);
    REQUIRE(countOf(quad.fields, "nodes") == 4);
    REQUIRE(countOf(quad.fields, "matFrameId") == 1);
    REQUIRE(quad.targets.back() == ReferenceTarget::Coordinate);
    // Every one of them is missing: 2 properties, 8 grid points, 1 coordinate system.
    REQUIRE(unresolvedIn(model) == 2 + 8 + 1);
}

TEST_CASE("CQUAD8 lists the corner thicknesses before THETA, ZOFFS and TFLAG", "[model][cards]")
{
    const Model model = readBulk(
        "ShellQuad8;1;3;34;36;78;76;35;53;58;77;0.115;0.035;0.04;0.035;25.;;0.04;1\n"
        "ShellQuad8;2;3;34;36;78;76;;;;;;;;;;7");
    const ShellQuad8Card first = cardAt<ShellQuad8Table>(model, 0);
    REQUIRE(first.nodes == std::array<base::EntityId, 8>{34, 36, 78, 76, 35, 53, 58, 77});
    REQUIRE(first.cornerThickness == std::array<double, 4>{0.115, 0.035, 0.04, 0.035});
    REQUIRE(first.thetaInDegrees == 25.0);
    REQUIRE(first.offset == 0.04);
    REQUIRE(first.thicknessFlag == 1);
    const ShellQuad8Card second = cardAt<ShellQuad8Table>(model, 1);
    REQUIRE(second.usesMaterialSystem());
    REQUIRE(second.matFrameId == 7);
    REQUIRE(second.nodes == std::array<base::EntityId, 8>{34, 36, 78, 76, 0, 0, 0, 0});
}

TEST_CASE("the corners of CTRIA6 and CQUAD8 are required and every point must differ", "[model][cards]")
{
    REQUIRE_THROWS_AS(readBulk("ShellTria6;1;3;1;2"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("ShellTria6;1;3;1;2;3;3"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("ShellTria6;1;3;1;2;3;;;;;;;;;;2"), base::DeckError); // TFLAG 2
    REQUIRE_THROWS_AS(readBulk("ShellTria6;1;;1;2;3"), base::DeckError);             // PID is required
    REQUIRE_THROWS_AS(readBulk("ShellQuad8;1;3;1;2;3"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("ShellQuad8;1;3;1;2;3;4;5;5"), base::DeckError);
    REQUIRE_NOTHROW(readBulk("ShellQuad8;1;3;1;2;3;4;0;0"));
}

TEST_CASE("CQUADR and CTRIAR are the plate elements of CQUAD4 and CTRIA3 with PID defaulting to EID", "[model][cards]")
{
    const Model model = readBulk(
        "ShellQuad4Drilling;1;;1;2;3;4;30.;;0.5;1;0.1;0.2;0.3;0.4\n"
        "ShellQuad4Drilling;2;5;1;2;3;4;;7\n"
        "ShellTria3Drilling;3;;1;2;3\n"
        "ShellTria3Drilling;4;6;1;2;3;2.;;1.65;1;1.94;1.98;2.02");
    const ShellQuad4DrillingCard byTheta = cardAt<ShellQuad4DrillingTable>(model, 0);
    REQUIRE(byTheta.propId == 1);
    REQUIRE(byTheta.thetaInDegrees == 30.0);
    REQUIRE(byTheta.offset == 0.5);
    REQUIRE(byTheta.thicknessFlag == 1);
    REQUIRE(byTheta.cornerThickness == std::array<double, 4>{0.1, 0.2, 0.3, 0.4});
    REQUIRE(cardAt<ShellQuad4DrillingTable>(model, 1).matFrameId == 7);
    REQUIRE(cardAt<ShellTria3DrillingTable>(model, 0).propId == 3);
    const ShellTria3DrillingCard full = cardAt<ShellTria3DrillingTable>(model, 1);
    REQUIRE(full.propId == 6);
    REQUIRE(full.cornerThickness == std::array<double, 3>{1.94, 1.98, 2.02});
}

TEST_CASE("CSHEAR and CTUBE default PID to EID", "[model][cards]")
{
    const Model model = readBulk("ShearPanel4;3;;2;6;4;8\nTube2;4;;1;2");
    REQUIRE(cardAt<ShearPanel4Table>(model).propId == 3);
    REQUIRE(cardAt<ShearPanel4Table>(model).nodes == std::array<base::EntityId, 4>{2, 6, 4, 8});
    REQUIRE(cardAt<Tube2Table>(model).propId == 4);
    REQUIRE_THROWS_AS(readBulk("ShearPanel4;3;;2;6;4;6"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("Tube2;4;;1;1"), base::DeckError);
}

TEST_CASE("CONROD is an element with its own material, area and constants and no property", "[model][cards]")
{
    Model model = readBulk("Rod2InlineSection;2;18;19;4;2.43;0.5;0.1;0.01\nRod2InlineSection;3;1;2;4");
    const Rod2InlineSectionCard full = cardAt<Rod2InlineSectionTable>(model, 0);
    REQUIRE(full.nodes == std::array<base::EntityId, 2>{18, 19});
    REQUIRE(full.matId == 4);
    REQUIRE(full.area == 2.43);
    REQUIRE(full.torsionConstant == 0.5);
    REQUIRE(full.torsionStressCoefficient == 0.1);
    REQUIRE(full.nonStructMass == 0.01);
    const Rod2InlineSectionCard minimal = cardAt<Rod2InlineSectionTable>(model, 1);
    REQUIRE(minimal.area == 0.0);
    REQUIRE(minimal.nonStructMass == 0.0);
    // It shares the element ID space with the other elements.
    REQUIRE(model.locate(base::IdSpace::Element, 2).has_value());
    REQUIRE_THROWS_AS(readBulk("Rod2InlineSection;2;1;2;4\nRod2;2;1;3;4"), base::DeckError);
    // The material and the four distinct grid points are references; there is no property.
    RecordingVisitor visitor;
    model.table<Rod2InlineSectionTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"nodes", "nodes", "matId"});
    REQUIRE(visitor.targets.back() == ReferenceTarget::Material);
    REQUIRE(unresolvedIn(model) == 3 + 3);
}

TEST_CASE("CTRIAX6 has its corners in G1, G3 and G5 and a material instead of a property", "[model][cards]")
{
    Model model = readBulk("AxisymTria6;1;888;14;;16;;26\nAxisymTria6;2;888;14;15;16;25;26;36;7.5");
    REQUIRE(cardAt<AxisymTria6Table>(model, 0).nodes == std::array<base::EntityId, 6>{14, 0, 16, 0, 26, 0});
    REQUIRE(cardAt<AxisymTria6Table>(model, 0).thetaInDegrees == 0.0);
    REQUIRE(cardAt<AxisymTria6Table>(model, 1).thetaInDegrees == 7.5);
    REQUIRE(cardAt<AxisymTria6Table>(model, 1).matId == 888);
    REQUIRE_THROWS_AS(readBulk("AxisymTria6;1;888;14;15;16;25"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("AxisymTria6;1;888;14;;16;;26;26"), base::DeckError);
    RecordingVisitor visitor;
    model.table<AxisymTria6Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.front() == "matId");
    REQUIRE(visitor.targets.front() == ReferenceTarget::Material);
    REQUIRE(countOf(visitor.fields, "nodes") == 3);
}

TEST_CASE("CTRIAX and CQUADX take an angle or a material system, with corners and optional points", "[model][cards]")
{
    const Model model = readBulk(
        "AxisymTria;1;205;34;76;78;46;47;48;30.\n"
        "AxisymTria;2;205;34;76;78;;;;;7\n"
        "AxisymQuad;3;205;34;76;78;35;46;47;48;49;51;30.\n"
        "AxisymQuad;4;205;34;76;78;35;;;;;;;7");
    REQUIRE(cardAt<AxisymTriaTable>(model, 0).nodes == std::array<base::EntityId, 6>{34, 76, 78, 46, 47, 48});
    REQUIRE(cardAt<AxisymTriaTable>(model, 0).thetaInDegrees == 30.0);
    REQUIRE(cardAt<AxisymTriaTable>(model, 1).matFrameId == 7);
    REQUIRE(cardAt<AxisymTriaTable>(model, 1).nodes == std::array<base::EntityId, 6>{34, 76, 78, 0, 0, 0});
    const AxisymQuadCard full = cardAt<AxisymQuadTable>(model, 0);
    REQUIRE(full.nodes == std::array<base::EntityId, 9>{34, 76, 78, 35, 46, 47, 48, 49, 51});
    REQUIRE(full.thetaInDegrees == 30.0);
    REQUIRE(cardAt<AxisymQuadTable>(model, 1).matFrameId == 7);
    REQUIRE_THROWS_AS(readBulk("AxisymTria;1;205;34;76"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("AxisymQuad;1;205;34;76;78"), base::DeckError);
}

TEST_CASE("CPYRAM has five corners and up to eight edge points", "[model][cards]")
{
    const Model model = readBulk("SolidPyramid;1;2;86;121;93;33;178\nSolidPyramid;2;4;15;16;17;18;40;21;;;24;;32");
    const SolidPyramidCard five = cardAt<SolidPyramidTable>(model, 0);
    REQUIRE(five.nodes == std::array<base::EntityId, 13>{86, 121, 93, 33, 178});
    REQUIRE_FALSE(five.hasEdgeNodes());
    const SolidPyramidCard some = cardAt<SolidPyramidTable>(model, 1);
    REQUIRE(some.nodes == std::array<base::EntityId, 13>{15, 16, 17, 18, 40, 21, 0, 0, 24, 0, 32});
    REQUIRE(some.hasEdgeNodes());
    REQUIRE_THROWS_AS(readBulk("SolidPyramid;1;2;86;121;93;33"), base::DeckError);    // the apex is required
    REQUIRE_THROWS_AS(readBulk("SolidPyramid;1;;86;121;93;33;178"), base::DeckError); // PID is required
    REQUIRE_THROWS_AS(readBulk("SolidPyramid;1;2;86;121;93;33;178;86"), base::DeckError);
}

TEST_CASE("the plane strain and plane stress elements of another implementation read THETA after their grid points",
          "[model][cards]")
{
    const Model model = readBulk(
        "PlaneStrainTria3;1;;1;2;3;20.\n"
        "PlaneStrainQuad4;2;;1;2;3;4;10.\n"
        "PlaneStrainTria6;3;3;1;2;3;4;5;6;12.\n"
        "PlaneStrainQuad8;4;4;47;45;124;125;64;65;126;66;10.\n"
        "PlaneStressTria6;5;3;1;2;3;4;5;6;12.\n"
        "PlaneStressQuad8;6;4;47;45;124;125;64;65;126;66;10.");
    REQUIRE(cardAt<PlaneStrainTria3Table>(model).propId == 1);
    REQUIRE(cardAt<PlaneStrainTria3Table>(model).thetaInDegrees == 20.0);
    REQUIRE(cardAt<PlaneStrainQuad4Table>(model).propId == 2);
    REQUIRE(cardAt<PlaneStrainQuad4Table>(model).thetaInDegrees == 10.0);
    REQUIRE(cardAt<PlaneStrainTria6Table>(model).thetaInDegrees == 12.0);
    REQUIRE(cardAt<PlaneStrainTria6Table>(model).nodes == std::array<base::EntityId, 6>{1, 2, 3, 4, 5, 6});
    REQUIRE(cardAt<PlaneStrainQuad8Table>(model).thetaInDegrees == 10.0);
    REQUIRE(cardAt<PlaneStrainQuad8Table>(model).nodes ==
            std::array<base::EntityId, 8>{47, 45, 124, 125, 64, 65, 126, 66});
    REQUIRE(cardAt<PlaneStressTria6Table>(model).thetaInDegrees == 12.0);
    REQUIRE(cardAt<PlaneStressQuad8Table>(model).thetaInDegrees == 10.0);
    REQUIRE_THROWS_AS(readBulk("PlaneStrainTria6;1;;1;2;3"),
                      base::DeckError); // PID is required for the six and eight node ones
    REQUIRE_THROWS_AS(readBulk("PlaneStrainQuad8;1;;1;2;3;4"), base::DeckError);
}

TEST_CASE("CPLSTS3 and CPLSTS4 carry a thickness flag and corner thicknesses", "[model][cards]")
{
    const Model model =
        readBulk("PlaneStressTria3;1;3;1;2;3;10.;1;0.5;0.6;0.7\nPlaneStressQuad4;2;3;1;2;3;4;10.;1;0.5;0.6;0.7;0.8");
    const PlaneStressTria3Card tria = cardAt<PlaneStressTria3Table>(model);
    REQUIRE(tria.thetaInDegrees == 10.0);
    REQUIRE(tria.thicknessFlag == 1);
    REQUIRE(tria.cornerThickness == std::array<double, 3>{0.5, 0.6, 0.7});
    const PlaneStressQuad4Card quad = cardAt<PlaneStressQuad4Table>(model);
    REQUIRE(quad.thetaInDegrees == 10.0);
    REQUIRE(quad.thicknessFlag == 1);
    REQUIRE(quad.cornerThickness == std::array<double, 4>{0.5, 0.6, 0.7, 0.8});
    REQUIRE(cardAt<PlaneStressTria3Table>(readBulk("PlaneStressTria3;1;;1;2;3")).cornerThickness ==
            std::array<double, 3>{-1.0, -1.0, -1.0});
}

TEST_CASE("CQUADX4 and CQUADX8 need a property and read THETA", "[model][cards]")
{
    const Model model =
        readBulk("AxisymQuad4;1;205;34;76;78;35;15.\nAxisymQuad8;2;205;34;76;78;35;46;47;48;49;15.");
    REQUIRE(cardAt<AxisymQuad4Table>(model).thetaInDegrees == 15.0);
    REQUIRE(cardAt<AxisymQuad8Table>(model).thetaInDegrees == 15.0);
    REQUIRE(cardAt<AxisymQuad8Table>(model).nodes == std::array<base::EntityId, 8>{34, 76, 78, 35, 46, 47, 48, 49});
    REQUIRE_THROWS_AS(readBulk("AxisymQuad4;1;;34;76;78;35"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("AxisymQuad8;1;;34;76;78;35"), base::DeckError);
}
