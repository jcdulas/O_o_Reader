// Checks of the thermal, radiation and acoustic cards: field positions, defaults, variants, references, ID
// spaces, renumbering and rules of MAT4, MAT5, PHBDY, CHBDYE, CHBDYG, CHBDYP, CONV, CONVM, PCONV, PCONVM, TEMPBC, RADM,
// RADBC, RADCAV, RADLST, RADMTX, RADSET, VIEW, VIEW3D, QVOL, QBDY1, QBDY2, QBDY3, QHBDY, QVECT and the acoustic cards
// (ACMODL, MFLUID, PACABS, PACBAR, PAABSF, CAABSF, CHACAB, CHACBR, CFLUID2 to CFLUID4, FSLIST, BDYLIST, FLSYM, PANEL).

#include "oor/model/cards/elements/acousticAbsorberProp/AcousticAbsorberPropTable.hpp"
#include "oor/model/cards/materials/heatIsoMatTemperature/HeatIsoMatTemperatureTable.hpp"
#include "oor/model/cards/properties/solidProp/SolidPropTable.hpp"
#include "oor/model/cards/thermal/acousticAbsorberHexa/AcousticAbsorberHexaTable.hpp"
#include "oor/model/cards/thermal/acousticAbsorberBrickProp/AcousticAbsorberBrickPropTable.hpp"
#include "oor/model/cards/thermal/acousticAbsorber/AcousticAbsorberTable.hpp"
#include "oor/model/cards/thermal/acousticBarrierHexa/AcousticBarrierHexaTable.hpp"
#include "oor/model/cards/thermal/acousticBarrierProp/AcousticBarrierPropTable.hpp"
#include "oor/model/cards/thermal/axisymFluidBoundaryList/AxisymFluidBoundaryListTable.hpp"
#include "oor/model/cards/thermal/axisymFluid2/AxisymFluid2Table.hpp"
#include "oor/model/cards/thermal/axisymFluid3/AxisymFluid3Table.hpp"
#include "oor/model/cards/thermal/axisymFluid4/AxisymFluid4Table.hpp"
#include "oor/model/cards/thermal/axisymFluidFreeSurface/AxisymFluidFreeSurfaceTable.hpp"
#include "oor/model/cards/thermal/axisymFluidSymmetry/AxisymFluidSymmetryTable.hpp"
#include "oor/model/cards/thermal/fluidStructureInterface/FluidStructureInterfaceTable.hpp"
#include "oor/model/cards/thermal/fluidStructurePanels/FluidStructurePanelsTable.hpp"
#include "oor/model/cards/thermal/fluidVirtualMass/FluidVirtualMassTable.hpp"
#include "oor/model/cards/thermal/forcedConvection/ForcedConvectionTable.hpp"
#include "oor/model/cards/thermal/forcedConvectionProp/ForcedConvectionPropTable.hpp"
#include "oor/model/cards/thermal/freeConvection/FreeConvectionTable.hpp"
#include "oor/model/cards/thermal/freeConvectionProp/FreeConvectionPropTable.hpp"
#include "oor/model/cards/thermal/heatAnisoMat/HeatAnisoMatTable.hpp"
#include "oor/model/cards/thermal/heatFluxBoundaryLoad3/HeatFluxBoundaryLoad3Table.hpp"
#include "oor/model/cards/thermal/heatFluxNodalLoad2/HeatFluxNodalLoad2Table.hpp"
#include "oor/model/cards/thermal/heatFluxNodeSetLoad/HeatFluxNodeSetLoadTable.hpp"
#include "oor/model/cards/thermal/heatFluxSurfaceLoad1/HeatFluxSurfaceLoad1Table.hpp"
#include "oor/model/cards/thermal/heatFluxVectorLoad/HeatFluxVectorLoadTable.hpp"
#include "oor/model/cards/thermal/heatIsoMat/HeatIsoMatTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceNodes/HeatSurfaceNodesTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceProperty/HeatSurfacePropertyTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceSide/HeatSurfaceSideTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceProp/HeatSurfacePropTable.hpp"
#include "oor/model/cards/thermal/heatVolumeSourceLoad/HeatVolumeSourceLoadTable.hpp"
#include "oor/model/cards/thermal/radiationCavity/RadiationCavityTable.hpp"
#include "oor/model/cards/thermal/radiationCavityList/RadiationCavityListTable.hpp"
#include "oor/model/cards/thermal/radiationCavitySet/RadiationCavitySetTable.hpp"
#include "oor/model/cards/thermal/radiationExchangeMatrix/RadiationExchangeMatrixTable.hpp"
#include "oor/model/cards/thermal/radiationProp/RadiationPropTable.hpp"
#include "oor/model/cards/thermal/radiationToSpace/RadiationToSpaceTable.hpp"
#include "oor/model/cards/thermal/temperatureBoundary/TemperatureBoundaryTable.hpp"
#include "oor/model/cards/thermal/viewFactorFace/ViewFactorFaceTable.hpp"
#include "oor/model/cards/thermal/viewFactorIntegration/ViewFactorIntegrationTable.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "oor/model/references/Renumbering.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using Catch::Approx;
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
std::size_t rowCount(const Model& model)
{
    const TableT* table = model.findTable<TableT>();
    return table == nullptr ? 0 : table->rows().rowCount();
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

// Writes the model as a model file and reads that again: the same cards.
void requireRoundTrip(const Model& model)
{
    oor::testing::requireModelFileRoundTrip(model);
}

// A hexahedron (element 1) on grid points 1 to 8, with a PSOLID and a thermal material, the surface elements of its six
// sides, and a convection property: a small heat transfer model.
std::string heatBlock()
{
    return "Node;1;;1.;0.;1.\nNode;2;;1.;0.;0.\nNode;3;;0.;0.;0.\nNode;4;;0.;0.;1.\n"
           "Node;5;;1.;1.;1.\nNode;6;;1.;1.;0.\nNode;7;;0.;1.;0.\nNode;8;;0.;1.;1.\nNode;10;;10.;10.;10.\n"
           "SolidHexa;1;1;1;2;3;4;5;6;7;8\nSolidProp;1;1\nHeatIsoMat;1;1.;;;10.;;1.\n"
           "HeatSurfaceSide;1;1;1\nHeatSurfaceSide;2;1;2\nHeatSurfaceSide;3;1;3\n"
           "HeatSurfaceSide;4;1;4\nHeatSurfaceSide;5;1;5\nHeatSurfaceSide;6;1;6\n"
           "FreeConvection;1;1;;;10\nFreeConvection;2;1;;;10\nFreeConvectionProp;1;1;0;0.\n"
           "TemperatureBoundary;10;Static;0.;10\n";
}

} // namespace

// ------------------------------------------------------------------------------------------- heat transfer materials

TEST_CASE(
    "MAT4 has its fields in the order of the format reference, defaults for CP, RHO and HGEN, and the phase change "
    "group",
    "[model][cards][thermal]")
{
    const Model model = readBulk(
        "HeatIsoMat;16;5.84E-4;4.37E-5;512.6;;1.27E-6\n"
        "HeatIsoMat;18;200.;896.;2707.;100.;0.001;0.5;1.5E+4;10.;5.;3.3E+5\n"
        "HeatIsoMat;19;;;;;;2.\n"
        "HeatIsoMat;20");
    HeatIsoMatCard card = cardAt<HeatIsoMatTable>(model, 0);
    REQUIRE(card.id == 16);
    REQUIRE(card.conductivity.value() == Approx(5.84e-4));
    REQUIRE(card.specificHeat == Approx(4.37e-5));
    REQUIRE(card.density == Approx(512.6));
    REQUIRE_FALSE(card.convectionCoefficient.has_value());
    REQUIRE(card.viscosity.value() == Approx(1.27e-6));
    REQUIRE(card.heatGenerationFactor == 1.0);
    REQUIRE_FALSE(card.hasPhaseChange());
    card = cardAt<HeatIsoMatTable>(model, 1);
    REQUIRE(card.convectionCoefficient.value() == Approx(100.0));
    REQUIRE(card.heatGenerationFactor == Approx(0.5));
    REQUIRE(card.referenceEnthalpy.value() == Approx(1.5e4));
    REQUIRE((card.phaseChangeTemperature.value() == 10.0 && card.phaseChangeRange.value() == 5.0 &&
             card.latentHeat.value() == Approx(3.3e5)));
    REQUIRE(card.hasPhaseChange());
    // Blank values: CP is 0.0, RHO and HGEN 1.0, the others are absent.
    card = cardAt<HeatIsoMatTable>(model, 2);
    REQUIRE((card.specificHeat == 0.0 && card.density == 1.0 && card.heatGenerationFactor == 2.0 &&
             !card.conductivity.has_value()));
    card = cardAt<HeatIsoMatTable>(model, 3);
    REQUIRE((card.specificHeat == 0.0 && card.density == 1.0 && card.heatGenerationFactor == 1.0));
    // A default is not written, an absent value is not written.

    requireRoundTrip(model);

    expectInvalid("HeatIsoMat;0;1.", "id");
    expectInvalid("HeatIsoMat;1;-1.", "conductivity");
    expectInvalid("HeatIsoMat;1;1.;-1.", "specificHeat");
    expectInvalid("HeatIsoMat;1;1.;1.;-1.", "density");
    expectInvalid("HeatIsoMat;1;1.;1.;1.;1.;-1.", "viscosity");
    expectInvalid("HeatIsoMat;1;1.;1.;1.;1.;1.;-1.", "heatGenerationFactor");
    expectInvalid("HeatIsoMat;1;1.;1.;1.;1.;1.;1.;1.;1.;-1.", "phaseChangeRange");
}

TEST_CASE("MAT5 is a conductivity matrix, with CP, RHO and HGEN on its two lines", "[model][cards][thermal]")
{
    const Model model =
        readBulk("HeatAnisoMat;23;0.076;;;0.069;;0.18;0.3;2.4\nHeatAnisoMat;25;1.;0.1;0.2;2.;0.3;3.;900.;2700.;0.5");
    HeatAnisoMatCard card = cardAt<HeatAnisoMatTable>(model, 0);
    REQUIRE((card.conductivityXx == Approx(0.076) && card.conductivityXy == 0.0 && card.conductivityXz == 0.0 &&
             card.conductivityYy == Approx(0.069)));
    REQUIRE((card.conductivityYz == 0.0 && card.conductivityZz == Approx(0.18) && card.specificHeat == Approx(0.3) &&
             card.density == Approx(2.4)));
    REQUIRE(card.heatGenerationFactor == 1.0);
    card = cardAt<HeatAnisoMatTable>(model, 1);
    REQUIRE((card.conductivityXy == 0.1 && card.conductivityXz == 0.2 && card.conductivityYz == 0.3 &&
             card.conductivityZz == 3.0 && card.specificHeat == 900.0));
    REQUIRE((card.density == 2700.0 && card.heatGenerationFactor == 0.5));
    requireRoundTrip(model);
    expectInvalid("HeatAnisoMat;1;1.;;;;;;-1.", "specificHeat");
    expectInvalid("HeatAnisoMat;0;1.", "id");
}

TEST_CASE("MAT4 and MAT5 share one ID space, apart from the structural materials and the other way round",
          "[model][cards][thermal]")
{
    // A MAT1 and a MAT4 of one ID are two materials.
    Model model = readBulk("IsoMat;1;2.1E+11;;0.3\nHeatIsoMat;1;1.\nHeatAnisoMat;2;1.");
    REQUIRE(model.cardCount() == 3);
    REQUIRE(model.locate(base::IdSpace::Material, 1).has_value());
    REQUIRE(model.locate(base::IdSpace::ThermalMaterial, 1).has_value());
    // Two thermal materials of one ID clash, a MAT4 and a MAT5 included.
    REQUIRE_THROWS_AS(readBulk("HeatIsoMat;1;1.\nHeatAnisoMat;1;1."), base::DeckError);
    // The MATT4 and MATT5 of a thermal material resolve to it (and only to it).
    Model companions = readBulk(
        "HeatIsoMat;1;1.\n"
        "HeatAnisoMat;2;1.\n"
        "HeatIsoMatTemperature;1;5\n"
        "HeatAnisoMatTemperature;2;7\n"
        "TableMaterial1;5;;;0.;1.;100.;2.;EndTable\n"
        "TableMaterial1;7;;;0.;1.;100.;2.;EndTable");
    REQUIRE(unresolvedIn(companions) == 0);
    Model lacking =
        readBulk("IsoMat;1;2.1E+11;;0.3\nHeatIsoMatTemperature;1;5\nTableMaterial1;5;;;0.;1.;100.;2.;EndTable");
    REQUIRE(unresolvedIn(lacking) == 1); // a MATT4 needs a MAT4: the MAT1 of that ID is not one
    // A thermal property names its material in the same field: the material of a PSOLID is a MAT4 in a heat analysis.
    Model solid = readBulk("HeatIsoMat;3;1.\nSolidProp;1;3");
    REQUIRE(unresolvedIn(solid) == 0);
    // Renumbering the thermal materials follows into the companions and into the PSOLID that names one, and leaves the
    // MAT1 and the property that names it alone.
    Model both = readBulk(
        "IsoMat;1;2.1E+11;;0.3\nHeatIsoMat;2;1.\nSolidProp;1;1\nSolidProp;2;2\nHeatIsoMatTemperature;2;"
        "5\nTableMaterial1;5;;;0.;1.;100.;2.;EndTable");
    Renumbering::apply(both, RenumberSpace::ThermalMaterial, Renumbering::offset(100));
    REQUIRE(cardAt<HeatIsoMatTable>(both).id == 102);
    REQUIRE(cardAt<HeatIsoMatTemperatureTable>(both).id == 102);
    REQUIRE(cardAt<SolidPropTable>(both, 0).matId == 1);
    REQUIRE(cardAt<SolidPropTable>(both, 1).matId == 102);
    REQUIRE(unresolvedIn(both) == 0);
}

// ------------------------------------------------------------------------------------------------ surface elements

TEST_CASE("CHBDYE is a side of a conduction element and has no space with the conduction elements",
          "[model][cards][thermal]")
{
    Model model = readBulk(heatBlock() + "HeatSurfaceSide;20;1;3;4;5;6;7");
    REQUIRE(rowCount<HeatSurfaceSideTable>(model) == 7);
    // The CHBDYE 1 and the CHEXA 1 coexist: the surface elements have a space of their own (the verification deck of
    // the solver, htflw47, does it).
    REQUIRE(model.locate(base::IdSpace::Element, 1).has_value());
    REQUIRE(model.locate(base::IdSpace::SurfaceElement, 1).has_value());
    const HeatSurfaceSideCard card = cardAt<HeatSurfaceSideTable>(model, 6);
    REQUIRE((card.id == 20 && card.conductionElemId == 1 && card.sideCode == 3 && card.frontViewId == 4 &&
             card.backViewId == 5));
    REQUIRE((card.frontRadiationPropId == 6 && card.backRadiationPropId == 7));
    REQUIRE(unresolvedIn(model) == 4); // the VIEW 4 and 5 and the RADM 6 and 7 of the last card
    requireRoundTrip(model);
    // Surface elements of the three kinds share the surface element space.
    REQUIRE_THROWS_AS(readBulk("HeatSurfaceSide;1;1;1\nHeatSurfaceNodes;1;AreaTria3;;;;;1;2;3"),
                      base::DeckError);
    REQUIRE_THROWS_AS(readBulk("HeatSurfaceNodes;1;AreaTria3;;;;;1;2;3\nHeatSurfaceProperty;1;1;Point;;;1"),
                      base::DeckError);

    expectInvalid("HeatSurfaceSide;1;1;0", "sideCode");
    expectInvalid("HeatSurfaceSide;1;1;7", "sideCode");
    expectInvalid("HeatSurfaceSide;1;0;1", "conductionElemId");
    expectInvalid("HeatSurfaceSide;0;1;1", "id");
}

TEST_CASE("the surface element and the conduction element of a CHBDYE are references that renumber",
          "[model][cards][thermal]")
{
    Model model = readBulk(heatBlock());
    REQUIRE(unresolvedIn(model) == 0);
    RecordingVisitor visitor;
    model.table<HeatSurfaceSideTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"conductionElemId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Element});
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(100));
    REQUIRE(cardAt<HeatSurfaceSideTable>(model).conductionElemId == 101);
    REQUIRE(cardAt<HeatSurfaceSideTable>(model).id == 1); // a surface element is not an element of the other space
    // The CONV that applies to a surface element follows the surface element space.
    Renumbering::apply(model, RenumberSpace::SurfaceElement, Renumbering::offset(1000));
    REQUIRE(cardAt<HeatSurfaceSideTable>(model, 5).id == 1006);
    REQUIRE(cardAt<FreeConvectionTable>(model, 0).id == 1001);
    REQUIRE(unresolvedIn(model) == 0);
}

TEST_CASE("CHBDYG has a type and as many grid points as the type needs", "[model][cards][thermal]")
{
    const Model model = readBulk(
        "HeatSurfaceNodes;2;AreaQuad4;4;4;6;6;110;113;112;111\n"
        "HeatSurfaceNodes;3;AreaTria6;;;;;1;2;3;;5\n"
        "HeatSurfaceNodes;4;ReversedNormal;;;;;1;2\n"
        "HeatSurfaceNodes;5;ReversedNormal1;;;;;4;5;6\n"
        "HeatSurfaceNodes;6;AreaQuad8;;;;;1;2;3;4;5;6;7;8");
    const HeatSurfaceNodesCard card = cardAt<HeatSurfaceNodesTable>(model);
    REQUIRE(card.kind == "AreaQuad4");
    REQUIRE((card.frontViewId == 4 && card.backViewId == 4 && card.frontRadiationPropId == 6 &&
             card.backRadiationPropId == 6));
    REQUIRE((card.nodes[0] == 110 && card.nodes[3] == 111 && card.nodes[4] == 0));
    REQUIRE(cardAt<HeatSurfaceNodesTable>(model, 1).nodes[4] == 5); // AREA6: a mid-side point may be left out
    REQUIRE(cardAt<HeatSurfaceNodesTable>(model, 1).nodes[3] == 0);
    requireRoundTrip(model);
    // The type is a word of the list, in any case.
    REQUIRE(cardAt<HeatSurfaceNodesTable>(readBulk("HeatSurfaceNodes;7;AreaTria3;;;;;1;2;3")).kind ==
            "AreaTria3");

    expectInvalid("HeatSurfaceNodes;1;AREA9;;;;;1;2;3", "kind");
    // A blank TYPE takes the one of the BDYOR, checked with the grid points; with no
    // BDYOR, or a BDYOR type of a CHBDYP, it is missing.
    const Model fromBdyor = readBulk("HeatSurfaceDefaults;AreaTria3\nHeatSurfaceNodes;8;;;;;;1;2;3");
    REQUIRE(cardAt<HeatSurfaceNodesTable>(fromBdyor).kind == "AreaTria3");
    REQUIRE_FALSE(fromBdyor.diagnostics().hasErrors());
    expectInvalid("HeatSurfaceNodes;1;;;;;;1;2;3", "kind");
    expectInvalid("HeatSurfaceDefaults;Line\nHeatSurfaceNodes;1;;;;;;1;2;3", "kind");
    expectInvalid("HeatSurfaceDefaults;AreaQuad4\nHeatSurfaceNodes;1;;;;;;1;2;3", "node");
    expectInvalid("HeatSurfaceNodes;1;AreaTria3;;;;;1;2", "node");
    expectInvalid("HeatSurfaceNodes;1;AreaTria3;;;;;1;2;3;4", "node");
    expectInvalid("HeatSurfaceNodes;1;AreaQuad4;;;;;1;2;3;3", "node");
    expectInvalid("HeatSurfaceNodes;1;ReversedNormal;;;;;1", "node");
}

TEST_CASE("CHBDYP has a type, two grid points (one for a POINT), a PHBDY and an orientation", "[model][cards][thermal]")
{
    Model model = readBulk(
        "Node;111;;0.;0.;0.\n"
        "Node;520;;0.;0.;1.\n"
        "HeatSurfaceProp;7;0.03\n"
        "HeatSurfaceProperty;2;7;Point;4;4;111;;520;6;6;;;0.;0.;1.\n"
        "HeatSurfaceProperty;60;7;Line;;;111;520\n"
        "HeatSurfaceProperty;61;7;Line;1;2;111;520;;3;4;111;10;1.;0.;0.");
    HeatSurfacePropertyCard card = cardAt<HeatSurfacePropertyTable>(model, 0);
    REQUIRE((card.id == 2 && card.propId == 7 && card.kind == "Point" && card.node1 == 111 && card.node2 == 0));
    REQUIRE((card.frontViewId == 4 && card.backViewId == 4 && card.orientationNode == 520 &&
             card.frontRadiationPropId == 6 && card.backRadiationPropId == 6));
    REQUIRE((card.direction1.value() == 0.0 && card.direction3.value() == 1.0));
    card = cardAt<HeatSurfacePropertyTable>(model, 1);
    REQUIRE((card.kind == "Line" && card.node2 == 520 && card.orientationNode == 0 && !card.direction1.has_value()));
    card = cardAt<HeatSurfacePropertyTable>(model, 2);
    REQUIRE((card.kind == "Line" && card.midsideNode == 111 && card.directionFrameId == 10 &&
             card.direction1.value() == 1.0));
    REQUIRE(unresolvedIn(model) == 9); // the VIEW 4, 1, 2 and the RADM 6, 6, 3, 4 of the cards, and the CE 10
    requireRoundTrip(model);
    // The PID names a PHBDY, and a PHBDY of the same number as a PSHELL is not that PSHELL.
    Model withShell = readBulk(
        "IsoMat;1;2.1E+11;;0.3\nHeatSurfaceProp;5;0.02\nShellProp;5;1;0.1\nNode;1;;0.;0.;0.\nHeatSurfaceProperty;1;5;"
        "Point;;;1");
    REQUIRE(unresolvedIn(withShell) == 0);
    RecordingVisitor visitor;
    model.table<HeatSurfacePropertyTable>().visitReferences(1, visitor);
    REQUIRE(visitor.targets.front() == ReferenceTarget::HeatBoundaryProperty);
    Renumbering::apply(model, RenumberSpace::HeatBoundaryProperty, Renumbering::offset(50));
    REQUIRE(cardAt<HeatSurfacePropertyTable>(model).propId == 57);

    expectInvalid("HeatSurfaceProperty;1;1;Point;;;1;2", "node2");
    expectInvalid("HeatSurfaceProperty;1;1;Line;;;1", "node2");
    expectInvalid("HeatSurfaceProperty;1;1;Line;;;1;1", "node2");
    expectInvalid("HeatSurfaceProperty;1;1;PLANE;;;1;2", "kind");
    expectInvalid("HeatSurfaceProperty;1;0;Point;;;1", "propId");
    expectInvalid("HeatSurfaceProperty;1;1;Point;;;0", "node1");
}

TEST_CASE("PHBDY has an area factor and two diameters, and a blank D2 is D1", "[model][cards][thermal]")
{
    const Model model = readBulk(
        "HeatSurfaceProp;2;0.03;1.5;1.5\nHeatSurfaceProp;3;0.271;2.25\nHeatSurfaceProp;4\nHeatSurfaceProp;5;;3.;4.");
    HeatSurfacePropCard card = cardAt<HeatSurfacePropTable>(model, 1);
    REQUIRE((card.area.value() == Approx(0.271) && card.diameter1.value() == 2.25 && card.diameter2.value() == 2.25));
    card = cardAt<HeatSurfacePropTable>(model, 2);
    REQUIRE((!card.area.has_value() && !card.diameter1.has_value() && !card.diameter2.has_value()));
    card = cardAt<HeatSurfacePropTable>(model, 3);
    REQUIRE((!card.area.has_value() && card.diameter1.value() == 3.0 && card.diameter2.value() == 4.0));
    requireRoundTrip(model);
    // The PIDs of the PHBDY cards are a space of their own.
    REQUIRE_NOTHROW(readBulk("HeatSurfaceProp;1;0.1\nShellProp;1;1;0.1"));
    REQUIRE_THROWS_AS(readBulk("HeatSurfaceProp;1;0.1\nHeatSurfaceProp;1;0.2"), base::DeckError);
    expectInvalid("HeatSurfaceProp;1;-1.", "area");
    expectInvalid("HeatSurfaceProp;1;1.;-1.", "diameter1");
    expectInvalid("HeatSurfaceProp;1;1.;;2.", "diameter1");
}

// ------------------------------------------------------------------------------------------------------- convection

TEST_CASE("CONV resolves the ambient points of TA2 to TA8 from TA1 and refers to its surface element, PCONV and points",
          "[model][cards][thermal]")
{
    Model model = readBulk(heatBlock() +
                           "FreeConvection;3;1;;;1;2;6;5\nFreeConvection;4;1;;;1;0\nFreeConvection;5;1;8;9;"
                           "10\nFreeConvection;6;1;;;3;4;5;6;7;8;9;10");
    FreeConvectionCard card = cardAt<FreeConvectionTable>(model, 0);
    REQUIRE((card.id == 1 && card.convectionPropId == 1 && card.filmNode == 0 && card.controlNode == 0 &&
             card.ambientNodes[0] == 10));
    for (std::size_t index = 1; index < FreeConvectionCard::AMBIENT_COUNT; ++index)
        REQUIRE(card.ambientNodes[index] == 10); // a blank point is TA1
    card = cardAt<FreeConvectionTable>(model, 2);
    REQUIRE((card.id == 3 && card.ambientNodes == std::array<base::EntityId, 8>{1, 2, 6, 5, 1, 1, 1, 1}));
    card = cardAt<FreeConvectionTable>(model, 3);
    REQUIRE((card.ambientNodes[1] == 0 && card.ambientNodes[2] == 1)); // an explicit 0 is no point
    card = cardAt<FreeConvectionTable>(model, 4);
    REQUIRE((card.filmNode == 8 && card.controlNode == 9 && card.ambientNodes[0] == 10));
    REQUIRE(cardAt<FreeConvectionTable>(model, 5).ambientNodes ==
            std::array<base::EntityId, 8>{3, 4, 5, 6, 7, 8, 9, 10});
    // The points are all written, because the blank ones depend on TA1.

    requireRoundTrip(model);
    RecordingVisitor visitor;
    model.table<FreeConvectionTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.front() == "id");
    REQUIRE(visitor.targets.front() == ReferenceTarget::SurfaceElement);
    REQUIRE(visitor.targets[1] == ReferenceTarget::ConvectionProperty);
    // A scalar point may be named without an SPOINT.
    Model scalar = readBulk(
        "HeatSurfaceProperty;1;1;Point;;;1\n"
        "Node;1;;0.;0.;0.\n"
        "HeatSurfaceProp;1;0.1\n"
        "FreeConvectionProp;1;1\n"
        "HeatIsoMat;1;1.;;;5.\n"
        "FreeConvection;1;1;;;77");
    REQUIRE(unresolvedIn(scalar) == 0);
    Model lacking = readBulk("Node;1;;0.;0.;0.\nFreeConvection;9;2;;;1");
    REQUIRE(unresolvedIn(lacking) == 2); // the surface element 9 and the PCONV 2
    // The ID of a CONV is the surface element it applies to: renumbering the surface elements renumbers it.
    Renumbering::apply(model, RenumberSpace::SurfaceElement, Renumbering::offset(100));
    REQUIRE(cardAt<FreeConvectionTable>(model, 0).id == 101);
    Renumbering::apply(model, RenumberSpace::ConvectionProperty, Renumbering::offset(30));
    REQUIRE(cardAt<FreeConvectionTable>(model, 0).convectionPropId == 31);
    REQUIRE(cardAt<FreeConvectionPropTable>(model).id == 31);

    expectInvalid("FreeConvection;1;1", "ambientNode1");
    expectInvalid("FreeConvection;1;0;;;1", "convectionPropId");
    expectInvalid("FreeConvection;0;1;;;1", "id");
    expectInvalid("FreeConvection;1;1;;;0", "ambientNode1");
    expectInvalid("FreeConvection;1;1;-1;;1", "filmNode");
}

TEST_CASE("CONVM has a mass flow point or a multiplier, and a blank TA2 is TA1", "[model][cards][thermal]")
{
    const Model model =
        readBulk("ForcedConvection;105;1;207;309;24;25\nForcedConvection;102;2;;;5;;3.5\nForcedConvection;103;3;;7;6");
    ForcedConvectionCard card = cardAt<ForcedConvectionTable>(model, 0);
    REQUIRE((card.id == 105 && card.convectionPropId == 1 && card.filmNode == 207 && card.massFlowNode == 309 &&
             card.ambientNode1 == 24));
    REQUIRE((card.ambientNode2 == 25 && card.massFlowFactor == 1.0));
    card = cardAt<ForcedConvectionTable>(model, 1);
    REQUIRE((card.ambientNode1 == 5 && card.ambientNode2 == 5 && card.massFlowFactor == 3.5));
    // TA2 is always written (it depends on TA1).

    requireRoundTrip(model);
    expectInvalid("ForcedConvection;1;1;;;5;;0.", "massFlowFactor");
    expectInvalid("ForcedConvection;1;1", "ambientNode1");
    expectInvalid("ForcedConvection;1;1;;;5;;-1.", "massFlowFactor");
}

TEST_CASE("PCONV has the layout of FTYPE 0 to 2 or the alternate layout of FTYPE 3", "[model][cards][thermal]")
{
    const Model model = readBulk(
        "FreeConvectionProp;57;2;0;0.3\n"
        "FreeConvectionProp;4;;;;1;107\n"
        "FreeConvectionProp;36;23;;;2;58;1.5;237;0;1.;0.;0.\n"
        "FreeConvectionProp;7;;;;3;11.24;11.06;11.18;11.31\n"
        "FreeConvectionProp;20;;;;3;10.\n"
        "FreeConvectionProp;300;3000");
    FreeConvectionPropCard card = cardAt<FreeConvectionPropTable>(model, 0);
    REQUIRE((card.id == 57 && card.matId == 2 && card.matrixForm == 0 && card.convectionExponent == 0.3 &&
             card.formulaKind == 0));
    card = cardAt<FreeConvectionPropTable>(model, 1);
    REQUIRE((card.formulaKind == 1 && card.tableId == 107 && card.matId == 0));
    card = cardAt<FreeConvectionPropTable>(model, 2);
    REQUIRE((card.formulaKind == 2 && card.characteristicLength.value() == 1.5 && card.inletNode == 237 &&
             card.directionFrameId == 0));
    REQUIRE((card.direction1.value() == 1.0 && card.direction2.value() == 0.0));
    // The alternate layout: a blank H2 to H8 is H1.
    card = cardAt<FreeConvectionPropTable>(model, 3);
    REQUIRE(card.formulaKind == 3);
    REQUIRE(card.convectionCoefficient ==
            std::array<double, 8>{11.24, 11.06, 11.18, 11.31, 11.24, 11.24, 11.24, 11.24});
    REQUIRE(cardAt<FreeConvectionPropTable>(model, 4).convectionCoefficient ==
            std::array<double, 8>{10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0});
    requireRoundTrip(model);

    // A PCONV may name a thermal material, a table, an inlet point and a coordinate system.
    Model references = readBulk(
        "Node;237;;0.;0.;0.\n"
        "HeatIsoMat;23;1.\n"
        "TableHeat1;9;0.;1.;EndTable\n"
        "TableHeatTwoVariables;58;0.;9;EndTable\n"
        "FreeConvectionProp;36;23;;;2;58;1.5;237");
    RecordingVisitor visitor;
    references.table<FreeConvectionPropTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId", "tableId", "inletNode"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::ThermalMaterial,
                                                            ReferenceTarget::HeatTable, ReferenceTarget::Node});
    REQUIRE(unresolvedIn(references) == 0);

    expectInvalid("FreeConvectionProp;1;1;5", "matrixForm");
    expectInvalid("FreeConvectionProp;1;;;;0", "matId");
    expectInvalid("FreeConvectionProp;1;;;;1", "tableId");
    expectInvalid("FreeConvectionProp;1;1;;;4", "formulaKind");
    expectInvalid("FreeConvectionProp;1;1;;-1.", "convectionExponent");
    expectInvalid("FreeConvectionProp;1;1;;;0;;-2.", "characteristicLength");
}

TEST_CASE("PCONVM has a fluid material, a formula, a flag and four coefficients", "[model][cards][thermal]")
{
    Model model =
        readBulk("HeatIsoMat;2;1.\nForcedConvectionProp;3;2;1;1;0.027;0.75;0.35;0.25\nForcedConvectionProp;4;2;;;1.5");
    ForcedConvectionPropCard card = cardAt<ForcedConvectionPropTable>(model, 0);
    REQUIRE((card.id == 3 && card.matId == 2 && card.matrixForm == 1 && card.massFlowConvection == 1 &&
             card.coefficient == Approx(0.027)));
    REQUIRE((card.reynoldsExponent == 0.75 && card.prandtlExponentInner == 0.35 && card.prandtlExponentOuter == 0.25));
    card = cardAt<ForcedConvectionPropTable>(model, 1);
    REQUIRE((card.matrixForm == 0 && card.massFlowConvection == 0 && card.reynoldsExponent == 0.0));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
    // A PCONV and a PCONVM may have one number (a CONV names the first, a CONVM the second).
    REQUIRE_NOTHROW(readBulk("FreeConvectionProp;1;1\nForcedConvectionProp;1;1;;;1."));
    expectInvalid("ForcedConvectionProp;1;1;2;;1.", "matrixForm");
    expectInvalid("ForcedConvectionProp;1;1;;2;1.", "massFlowConvection");
    expectInvalid("ForcedConvectionProp;1;1", "coefficient");
    expectInvalid("ForcedConvectionProp;1;1;;;-1.", "coefficient");
    expectInvalid("ForcedConvectionProp;1;0;;;1.", "matId");
}

// ------------------------------------------------------------------------------------------------------- heat loads

TEST_CASE("QVOL, QBDY1, QBDY3 and QVECT list their elements with THRU and BY", "[model][cards][thermal]")
{
    const Model model = readBulk(
        "HeatVolumeSourceLoad;5;12.5;107;10;12;11;9\n"
        "HeatVolumeSourceLoad;123;0.005;;1;Through;9\n"
        "HeatVolumeSourceLoad;124;1.5;;1;Through;50;By;2;60\n"
        "HeatFluxSurfaceLoad1;108;2.E-5;723\n"
        "HeatFluxSurfaceLoad1;108;2.E-5;727;Through;739\n"
        "HeatFluxSurfaceLoad1;110;2.5;-3;4\n"
        "HeatFluxBoundaryLoad3;2;17.5;12;1;Through;40;By;3\n"
        "HeatFluxBoundaryLoad3;20;0.1666666;;1;2;3;4;5;6\n"
        "HeatFluxVectorLoad;10;15.;800.;;1.;;1.;;1.;;103;24;25;26;27\n"
        "HeatFluxVectorLoad;11;;;0;;5;;;;7;;20;Through;30");
    HeatVolumeSourceLoadCard qvol = cardAt<HeatVolumeSourceLoadTable>(model, 0);
    REQUIRE((qvol.id == 5 && qvol.heatGeneration == 12.5 && qvol.controlNode == 107));
    REQUIRE(qvol.elements == std::vector<ThermalRun>{{10, 10, 1}, {12, 12, 1}, {11, 11, 1}, {9, 9, 1}});
    REQUIRE(cardAt<HeatVolumeSourceLoadTable>(model, 1).elements == std::vector<ThermalRun>{{1, 9, 1}});
    REQUIRE(cardAt<HeatVolumeSourceLoadTable>(model, 2).elements == std::vector<ThermalRun>{{1, 50, 2}, {60, 60, 1}});
    REQUIRE(cardAt<HeatFluxSurfaceLoad1Table>(model, 1).elements == std::vector<ThermalRun>{{727, 739, 1}});
    REQUIRE(cardAt<HeatFluxSurfaceLoad1Table>(model, 2).elements ==
            std::vector<ThermalRun>{{-3, -3, 1}, {4, 4, 1}}); // -3: a back face
    const HeatFluxBoundaryLoad3Card qbdy3 = cardAt<HeatFluxBoundaryLoad3Table>(model, 0);
    REQUIRE(
        (qbdy3.heatFlux == 17.5 && qbdy3.controlNode == 12 && qbdy3.elements == std::vector<ThermalRun>{{1, 40, 3}}));
    REQUIRE(cardAt<HeatFluxBoundaryLoad3Table>(model, 1).elements.size() == 6);
    HeatFluxVectorLoadCard qvect = cardAt<HeatFluxVectorLoadTable>(model, 0);
    REQUIRE((qvect.heatFlux.value() == 15.0 && qvect.sourceTemperature.value() == 800.0 &&
             !qvect.directionFrameId.has_value()));
    REQUIRE((qvect.direction == std::array<double, 3>{1.0, 1.0, 1.0} && qvect.controlNode == 103 &&
             qvect.elements.size() == 4));
    qvect = cardAt<HeatFluxVectorLoadTable>(model, 1);
    REQUIRE((!qvect.heatFlux.has_value() && qvect.directionFrameId.value() == 0 &&
             qvect.tableId == std::array<base::EntityId, 3>{5, 0, 7}));
    REQUIRE((qvect.direction == std::array<double, 3>{0.0, 0.0, 0.0} &&
             qvect.elements == std::vector<ThermalRun>{{20, 30, 1}}));
    requireRoundTrip(model);
    // A range with BY is written on one line, with its THRU: never split by the end of a line.

    // With compress_thru nothing changes (the lists are kept as written).
    expectInvalid("HeatVolumeSourceLoad;1;1.;;0", "elements");
    expectInvalid("HeatVolumeSourceLoad;1;1.;;3;Through;2", "elements");
    expectInvalid("HeatVolumeSourceLoad;1;1.", "elements");
    expectInvalid("HeatVolumeSourceLoad;1;1.;;1;Through", "Through");
    expectInvalid("HeatVolumeSourceLoad;1;1.;;Through;5", "Through");
    expectInvalid("HeatVolumeSourceLoad;1;1.;;1;By;2", "By");
    expectInvalid("HeatVolumeSourceLoad;1;1.;;1;Through;9;By;0", "By");
    expectInvalid("HeatVolumeSourceLoad;1;1.;-1;1", "controlNode");
    expectInvalid("HeatVolumeSourceLoad;0;1.;;1", "id");
    expectInvalid("HeatFluxSurfaceLoad1;1;1.;0", "elements");
    expectInvalid("HeatFluxSurfaceLoad1;1;1.;1;Through;9;By;2", "elements");
    expectInvalid("HeatFluxBoundaryLoad3;1;1.;;-5;Through;9", "elements");
    expectInvalid("HeatFluxVectorLoad;1;1.", "elements");
    expectInvalid("HeatFluxVectorLoad;1;1.;;-2;;;;;;;;1", "directionFrameId");
    expectInvalid("HeatFluxVectorLoad;1;1.;;;;;;;;;;1;Through;5;By;2", "elements");
}

TEST_CASE("the elements of the heat loads are surface elements (QVOL: conduction elements) and renumber with them",
          "[model][cards][thermal]")
{
    Model model = readBulk(heatBlock() +
                           "HeatFluxSurfaceLoad1;20;5.;1;Through;4\n"
                           "HeatFluxSurfaceLoad1;21;5.;-5;6\n"
                           "HeatFluxNodalLoad2;22;3;1.;2.;3.\n"
                           "HeatFluxBoundaryLoad3;23;1.;10;1;2\n"
                           "HeatVolumeSourceLoad;24;1.;10;1\n"
                           "HeatFluxVectorLoad;25;1.;300.;;1.;;0.;;0.;;10;1;Through;6\n"
                           "RadiationToSpace;10;1.;10;1;Through;3");
    REQUIRE(unresolvedIn(model) == 0);
    RecordingVisitor visitor;
    model.table<HeatFluxSurfaceLoad1Table>().visitReferences(0, visitor);
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::SurfaceElementInRange,
                                                            ReferenceTarget::SurfaceElementInRange});
    RecordingVisitor volume;
    model.table<HeatVolumeSourceLoadTable>().visitReferences(0, volume);
    REQUIRE(volume.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::ScalarOrGridPoint, ReferenceTarget::Element});
    // A single missing surface element is reported, the middle of a THRU range may be missing.
    Model lacking = readBulk(heatBlock() + "HeatFluxSurfaceLoad1;20;5.;1;Through;40\nHeatFluxSurfaceLoad1;21;5.;99");
    REQUIRE(unresolvedIn(lacking) == 1);
    // A renumbering of the surface elements: the single IDs (a negative one keeps its sign) and the ends of the ranges.
    Renumbering::apply(model, RenumberSpace::SurfaceElement, Renumbering::offset(1000));
    REQUIRE(cardAt<HeatFluxSurfaceLoad1Table>(model, 0).elements == std::vector<ThermalRun>{{1001, 1004, 1}});
    REQUIRE(cardAt<HeatFluxSurfaceLoad1Table>(model, 1).elements ==
            std::vector<ThermalRun>{{-1005, -1005, 1}, {1006, 1006, 1}});
    REQUIRE(cardAt<HeatFluxNodalLoad2Table>(model).elemId == 1003);
    REQUIRE(cardAt<RadiationToSpaceTable>(model).elements == std::vector<ThermalRun>{{1001, 1003, 1}});
    REQUIRE(cardAt<HeatVolumeSourceLoadTable>(model).elements ==
            std::vector<ThermalRun>{{1, 1, 1}}); // a conduction element: not moved
    REQUIRE(unresolvedIn(model) == 0);
    // The set IDs renumber with the load sets.
    Renumbering::apply(model, RenumberSpace::LoadCombinationSet, Renumbering::offset(500));
    REQUIRE(cardAt<HeatFluxSurfaceLoad1Table>(model, 0).id == 520);
    REQUIRE(cardAt<HeatVolumeSourceLoadTable>(model).id == 524);
    REQUIRE(cardAt<HeatFluxVectorLoadTable>(model).id == 525);
}

TEST_CASE("the heat loads are load sets: a Case Control LOAD and a TLOAD1 EXCITEID find them",
          "[model][cards][thermal]")
{
    Model model = readModelText(
        "[executive]\nSolutionSequence;153\n[case]\nSubcaseStart;1\nStaticLoadSelection;20\nTemperatureSelection;21;"
        "TemperatureLoadPart\nSubcaseStart;2\nStaticLoadSelection;30\n[model]\n" +
            heatBlock() +
            "HeatFluxSurfaceLoad1;20;5.;1;Through;4\n"
            "HeatVolumeSourceLoad;30;1.;;1\n"
            "DefaultTemperatureLoad;21;100.\n"
            "TransientTableLoad1;40;20;;;;1\n"
            "TableDynamic1;1;;;0.;1.;1.;1.;EndTable\n",
        true);
    base::DiagnosticSink sink(base::ReadMode::Tolerant);
    REQUIRE(ReferenceChecker::check(model, sink) == 0);
    REQUIRE(cardsOfSet(model, CardCategory::Load, 20).size() == 1);
    REQUIRE(cardsOfSet(model, CardCategory::Load, 30).size() == 1);
}

TEST_CASE("QBDY2 holds the flux at each point, QHBDY a face of grid points of a type", "[model][cards][thermal]")
{
    Model model = readBulk(
        "Node;101;;0.;0.;0.\n"
        "Node;102;;1.;0.;0.\n"
        "Node;103;;1.;1.;0.\n"
        "Node;104;;0.;1.;0.\n"
        "HeatFluxNodalLoad2;108;723;3.E-5;3.E-5;4.E-5;4.E-5\n"
        "HeatFluxNodalLoad2;110;5;1.;2.;3.;4.;5.;6.;7.;8.\n"
        "HeatFluxNodeSetLoad;2;AreaQuad4;17.5;;101;102;104;103\n"
        "HeatFluxNodeSetLoad;3;Point;5.;0.1;101\n"
        "HeatFluxNodeSetLoad;4;AreaTria6;1.;;101;102;103;104\n"
        "HeatFluxNodeSetLoad;5;Line;2.;;101;102");
    HeatFluxNodalLoad2Card flux = cardAt<HeatFluxNodalLoad2Table>(model, 0);
    REQUIRE(
        (flux.id == 108 && flux.elemId == 723 && flux.heatFlux[0] == Approx(3e-5) && flux.heatFlux[3] == Approx(4e-5)));
    REQUIRE((flux.heatFlux[4] == 0.0 && flux.heatFlux[7] == 0.0));
    REQUIRE(cardAt<HeatFluxNodalLoad2Table>(model, 1).heatFlux[7] == 8.0);
    HeatFluxNodeSetLoadCard face = cardAt<HeatFluxNodeSetLoadTable>(model, 0);
    REQUIRE((face.faceKind == "AreaQuad4" && face.heatFlux == 17.5 && !face.area.has_value() && face.nodes[3] == 103));
    REQUIRE(cardAt<HeatFluxNodeSetLoadTable>(model, 1).area.value() == Approx(0.1));
    REQUIRE(cardAt<HeatFluxNodeSetLoadTable>(model, 2).nodes[3] == 104); // AREA6 has four to six points
    REQUIRE(unresolvedIn(model) == 2);                                   // the elements 723 and 5 of the QBDY2 cards
    requireRoundTrip(model);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(10));
    REQUIRE(cardAt<HeatFluxNodeSetLoadTable>(model, 0).nodes[2] == 114);

    expectInvalid("HeatFluxNodeSetLoad;1;AreaQuad4;1.;;1;2;3", "node");
    expectInvalid("HeatFluxNodeSetLoad;1;AreaTria3;1.;;1;2;3;4", "node");
    expectInvalid("HeatFluxNodeSetLoad;1;Point;1.;;1;2", "node");
    expectInvalid("HeatFluxNodeSetLoad;1;CUBE;1.;;1", "faceKind");
    expectInvalid("HeatFluxNodeSetLoad;1;AreaQuad8;1.;;1;2;3;4", "node");
    expectInvalid("HeatFluxNodeSetLoad;1;Line;1.;-1.;1;2", "area");
    expectInvalid("HeatFluxNodalLoad2;1;0;1.", "elemId");
}

TEST_CASE("QVECT components are reals or tables of time", "[model][cards][thermal]")
{
    Model model =
        readBulk("TableDynamic1;5;;;0.;1.;1.;1.;EndTable\nHeatFluxVectorLoad;11;5.;;0;;5;;;;5;;20;Through;30");
    REQUIRE(unresolvedIn(model) == 0); // the ends of a THRU range may be missing
    RecordingVisitor visitor;
    model.table<HeatFluxVectorLoadTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"tableId", "tableId", "elements", "elements"});
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(10));
    REQUIRE(cardAt<HeatFluxVectorLoadTable>(model).tableId == std::array<base::EntityId, 3>{15, 0, 15});
    expectInvalid("HeatFluxVectorLoad;1;1.;;;;-5;;;;;;1", "tableId");
}

// ----------------------------------------------------------------------------------------------------- temperatures

TEST_CASE("TEMPBC has a type and pairs of a temperature and a grid point or a range of them", "[model][cards][thermal]")
{
    Model model = readBulk(
        "Node;1;;0.;0.;0.\n"
        "Node;2;;0.;0.;0.\n"
        "Node;3;;0.;0.;0.\n"
        "Node;4;;0.;0.;0.\n"
        "TemperatureBoundary;10;Static;120.;1;120.;2;120.;3\n"
        "TemperatureBoundary;20;Static;120.;4;Through;40;By;3\n"
        "TemperatureBoundary;300;Transient;450.;99\n"
        "TemperatureBoundary;11;;25.;1;50.;2\n"
        "TemperatureBoundary;21;Static;100.;1;Through;5;200.;10;Through;20");
    TemperatureBoundaryCard card = cardAt<TemperatureBoundaryTable>(model, 0);
    REQUIRE((card.id == 10 && card.kind == "Static" && card.groups.size() == 3));
    REQUIRE((card.groups[1].temperature == 120.0 && card.groups[2].grids.first == 3));
    card = cardAt<TemperatureBoundaryTable>(model, 1);
    REQUIRE((card.groups.size() == 1 && card.groups[0].grids == ThermalRun{4, 40, 3}));
    REQUIRE(cardAt<TemperatureBoundaryTable>(model, 2).kind == "Transient");
    REQUIRE(cardAt<TemperatureBoundaryTable>(model, 3).kind == "Static"); // a blank type is STAT
    card = cardAt<TemperatureBoundaryTable>(model, 4);
    REQUIRE((card.groups.size() == 2 && card.groups[0].grids == ThermalRun{1, 5, 1} &&
             card.groups[1].grids == ThermalRun{10, 20, 1}));
    requireRoundTrip(model);
    // A group is written whole on one line; the type STAT is not written.

    // A TEMPBC STAT is the SPC set of a subcase; a TRAN one is the EXCITEID of a TLOADi.
    Model decks = readModelText(
        "[executive]\n"
        "SolutionSequence;153\n"
        "[case]\n"
        "SubcaseStart;1\n"
        "SinglePointConstrSelection;10\n"
        "DynamicLoadSelection;12\n"
        "[model]\n"
        "Node;1;;0.;0.;0.\n"
        "TemperatureBoundary;10;Static;0.;1\n"
        "TemperatureBoundary;300;Transient;500.;1\n"
        "TransientTableLoad1;12;300;;;;1\n"
        "TableDynamic1;1;;;0.;1.;1.;1.;EndTable\n",
        true);
    base::DiagnosticSink sink(base::ReadMode::Tolerant);
    REQUIRE(ReferenceChecker::check(decks, sink) == 0);
    REQUIRE(cardsOfSet(decks, CardCategory::Constraint, 10).size() == 1);
    Model lacking = readBulk(
        "Node;1;;0.;0.;0.\n"
        "TransientTableLoad1;12;301;;;;1\n"
        "TableDynamic1;1;;;0.;1.;1.;1.;EndTable\n"
        "TemperatureBoundary;300;Transient;500.;1\n"
        "TemperatureBoundary;302;Static;500.;1");
    REQUIRE(unresolvedIn(lacking) == 1); // 301 is the excitation of no card; the STAT set 302 is not one
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<TemperatureBoundaryTable>(model, 0).groups[2].grids.first == 103);
    REQUIRE(cardAt<TemperatureBoundaryTable>(model, 1).groups[0].grids == ThermalRun{104, 140, 3});

    expectInvalid("TemperatureBoundary;1;Static;100.", "nodeId");
    expectInvalid("TemperatureBoundary;1;SLOW;100.;1", "kind");
    expectInvalid("TemperatureBoundary;1;STATIC;100.;1", "kind");
    expectInvalid("TemperatureBoundary;1;static;100.;1", "kind");
    expectInvalid("TemperatureBoundary;1;Static;;1", "temperature");
    expectInvalid("TemperatureBoundary;1;Static;100.;1;Through", "nodeId");
    expectInvalid("TemperatureBoundary;1;Static;100.;5;Through;2", "nodeId");
    expectInvalid("TemperatureBoundary;1;Static;100.;1;Through;5;By;0", "By");
    expectInvalid("TemperatureBoundary;1;Static;100.;1;By;2", "temperature");
    expectInvalid("TemperatureBoundary;0;Static;100.;1", "id");
    expectInvalid("TemperatureBoundary;1", "temperature");
}

// -------------------------------------------------------------------------------------------------------- radiation

TEST_CASE("RADM has an absorptivity and any number of emissivities", "[model][cards][thermal]")
{
    const Model model = readBulk(
        "RadiationProp;11;0.42;0.35;0.31;0.24;0.19;0.11\nRadiationProp;2000;;0.9\nRadiationProp;6;0.8\nRadiationProp;7;"
        "0.5;0.1;0.2;0.3;0.4;0.5;0.6;0.7;0.8;0.9");
    RadiationPropCard card = cardAt<RadiationPropTable>(model, 0);
    REQUIRE((card.id == 11 && card.absorptivity.value() == Approx(0.42) && card.emissivity.size() == 5));
    REQUIRE(card.emissivity[4] == Approx(0.11));
    card = cardAt<RadiationPropTable>(model, 1);
    REQUIRE((!card.absorptivity.has_value() && card.emissivity == std::vector<double>{0.9}));
    REQUIRE(cardAt<RadiationPropTable>(model, 2).emissivity.empty());
    REQUIRE(cardAt<RadiationPropTable>(model, 3).emissivity.size() == 9);
    requireRoundTrip(model);
    expectInvalid("RadiationProp;0;0.5", "id");
    expectInvalid("RadiationProp;1;-0.5", "absorptivity");
    expectInvalid("RadiationProp;1;0.5;-0.1", "emissivity");
}

TEST_CASE("the surface elements name their RADM and VIEW, the VIEW and RADCAV their cavity, and they renumber",
          "[model][cards][thermal]")
{
    Model model = readBulk(
        "Node;1;;0.;0.;0.\n"
        "Node;2;;1.;0.;0.\n"
        "Node;3;;1.;1.;0.\n"
        "RadiationProp;2000;;0.9\n"
        "ViewFactorFace;10;1;None;3;3\n"
        "RadiationCavity;1;1;;;;;0.1;;3;5\n"
        "IdListSet1;3;1;2\n"
        "IdListSet1;5;2;3\n"
        "HeatSurfaceNodes;1;AreaTria3;10;;2000;;1;2;3\n"
        "RadiationCavityList;1;;1\n"
        "RadiationCavitySet;1\n"
        "ViewFactorIntegration;1\n"
        "RadiationExchangeMatrix;1;1;0.\n"
        "RadiationToSpace;1;1.;;1");
    REQUIRE(unresolvedIn(model) == 0);
    RecordingVisitor visitor;
    model.table<HeatSurfaceNodesTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.front() == "frontViewId");
    REQUIRE(visitor.targets.front() == ReferenceTarget::ViewFactorFaceFactor);
    REQUIRE(visitor.targets[1] == ReferenceTarget::RadiationMaterial);
    // The cavity of a VIEW, a RADSET, a RADLST, a RADMTX: the cards of one cavity share its number.
    Model lacking = readBulk("ViewFactorFace;10;4\nRadiationCavitySet;7");
    REQUIRE(unresolvedIn(lacking) == 2);
    Renumbering::apply(model, RenumberSpace::RadiationCavity, Renumbering::offset(40));
    REQUIRE(cardAt<RadiationCavityTable>(model).id == 41);
    REQUIRE(cardAt<ViewFactorFaceTable>(model).cavityId == 41);
    REQUIRE(cardAt<RadiationCavityListTable>(model).cavityId == 41);
    REQUIRE(cardAt<RadiationExchangeMatrixTable>(model).cavityId == 41);
    REQUIRE(cardAt<ViewFactorIntegrationTable>(model).cavityId == 41);
    REQUIRE(cardAt<RadiationCavitySetTable>(model).cavities == std::vector<base::EntityId>{41});
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::RadiationMaterial, Renumbering::offset(1));
    REQUIRE(cardAt<RadiationPropTable>(model).id == 2001);
    REQUIRE(cardAt<HeatSurfaceNodesTable>(model).frontRadiationPropId == 2001);
    Renumbering::apply(model, RenumberSpace::ViewFactorFaceFactor, Renumbering::offset(5));
    REQUIRE(cardAt<ViewFactorFaceTable>(model).id == 15);
    REQUIRE(cardAt<HeatSurfaceNodesTable>(model).frontViewId == 15);
    // The SETs of the RADCAV are Bulk Data sets.
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(100));
    REQUIRE(cardAt<RadiationCavityTable>(model).sets == std::vector<base::EntityId>{103, 105});
}

TEST_CASE("VIEW and VIEW3D have their words and ranges", "[model][cards][thermal]")
{
    const Model model = readBulk(
        "ViewFactorFace;1;1;Both;3;4;0.3\n"
        "ViewFactorFace;12;2;CanShade\n"
        "ViewFactorFace;13;;CanBeShaded\n"
        "ViewFactorIntegration;1;3;3;5;2.E-6\n"
        "ViewFactorIntegration;3;10;6;20;0.2;1.E-8;0.05;12");
    ViewFactorFaceCard view = cardAt<ViewFactorFaceTable>(model, 0);
    REQUIRE((view.id == 1 && view.cavityId == 1 && view.shading == "Both" && view.betaSubdivisions == 3 &&
             view.gammaSubdivisions == 4));
    REQUIRE(view.surfaceOffset == 0.3);
    REQUIRE(cardAt<ViewFactorFaceTable>(model, 1).shading == "CanShade");
    view = cardAt<ViewFactorFaceTable>(model, 2);
    REQUIRE((view.cavityId == 0 && view.shading == "CanBeShaded" && view.betaSubdivisions == 1));
    ViewFactorIntegrationCard view3d = cardAt<ViewFactorIntegrationTable>(model, 0);
    REQUIRE((view3d.blockingIntegrationOrder == 3 && view3d.shadowingIntegrationOrder == 3 &&
             view3d.contourIntegrationLevel == 5 && view3d.errorTolerance == Approx(2e-6)));
    REQUIRE((view3d.zeroTolerance == 1.0e-10 && view3d.warpTolerance == 0.01 && view3d.radiationCheck == 3));
    view3d = cardAt<ViewFactorIntegrationTable>(model, 1);
    REQUIRE((view3d.blockingIntegrationOrder == 10 && view3d.shadowingIntegrationOrder == 6 &&
             view3d.contourIntegrationLevel == 20 && view3d.radiationCheck == 12));
    requireRoundTrip(model);
    expectInvalid("ViewFactorFace;1;1;MAYBE", "shading");
    expectInvalid("ViewFactorFace;1;1;Both;0", "betaSubdivisions");
    expectInvalid("ViewFactorFace;1;-1", "cavityId");
    expectInvalid("ViewFactorIntegration;1;7", "blockingIntegrationOrder");
    expectInvalid("ViewFactorIntegration;1;;7", "shadowingIntegrationOrder");
    expectInvalid("ViewFactorIntegration;1;;;21", "contourIntegrationLevel");
    expectInvalid("ViewFactorIntegration;1;;;;0.", "errorTolerance");
    expectInvalid("ViewFactorIntegration;1;;;;;;1.", "warpTolerance");
}

TEST_CASE("RADCAV, RADLST, RADMTX and RADSET have their lists", "[model][cards][thermal]")
{
    const Model model = readBulk(
        "RadiationCavity;1;1;;;;;0.95;;3;6;4;6;8;6\n"
        "RadiationCavity;2;10;No;0.5;3;FiniteDifferenceMethod;0.2;64\n"
        "RadiationCavity;3\n"
        "RadiationCavity;4;;;;0;Contour\n"
        "RadiationCavityList;2;1;2;3;4;5;6;7\n"
        "RadiationCavityList;3;;1;Through;10;-20;30\n"
        "RadiationCavityList;4;-2;5;6\n"
        "RadiationExchangeMatrix;2;1;0.;0.15;0.25;0.1;0.35;0.15\n"
        "RadiationCavitySet;1;2;3;4;5;6;7;8;9");
    RadiationCavityCard cavity = cardAt<RadiationCavityTable>(model, 0);
    REQUIRE((cavity.id == 1 && cavity.ambientElemId == 1 && cavity.shadowing == "Yes" &&
             cavity.subdivisionAreaRatio == Approx(0.95)));
    REQUIRE((cavity.componentCount == 32 && !cavity.viewFactorOutput.has_value() && cavity.viewFactorMethod.empty()));
    REQUIRE(cavity.sets == std::vector<base::EntityId>{3, 6, 4, 6, 8, 6});
    cavity = cardAt<RadiationCavityTable>(model, 1);
    REQUIRE((cavity.shadowing == "No" && cavity.scaleFactor == 0.5 && cavity.viewFactorOutput.value() == 3 &&
             cavity.viewFactorMethod == "FiniteDifferenceMethod"));
    REQUIRE((cavity.subdivisionAreaRatio == 0.2 && cavity.componentCount == 64));
    REQUIRE(cardAt<RadiationCavityTable>(model, 2).subdivisionAreaRatio == 0.1); // the default of the format reference
    REQUIRE(cardAt<RadiationCavityTable>(model, 3).viewFactorOutput.value() == 0); // a PRTPCH of 0 is not a blank one
    RadiationCavityListCard list = cardAt<RadiationCavityListTable>(model, 0);
    REQUIRE((list.cavityId == 2 && list.matrixKind.value() == 1 && list.elements.size() == 6));
    list = cardAt<RadiationCavityListTable>(model, 1);
    REQUIRE((!list.matrixKind.has_value() &&
             list.elements == std::vector<ThermalRun>{{1, 10, 1}, {-20, -20, 1}, {30, 30, 1}}));
    REQUIRE(cardAt<RadiationCavityListTable>(model, 2).matrixKind.value() == -2);
    REQUIRE(cardAt<RadiationExchangeMatrixTable>(model).factors.size() == 6);
    REQUIRE(cardAt<RadiationCavitySetTable>(model).cavities.size() == 9);
    requireRoundTrip(model);
    // The columns of a matrix are cards of one cavity.
    REQUIRE_NOTHROW(readBulk("RadiationExchangeMatrix;1;1;0.\nRadiationExchangeMatrix;1;2;0.1;0.2"));

    expectInvalid("RadiationCavity;1;;MAYBE", "shadowing");
    expectInvalid("RadiationCavity;1;;;1.5", "scaleFactor");
    expectInvalid("RadiationCavity;1;;;;6", "viewFactorOutput");
    expectInvalid("RadiationCavity;1;;;;;XX", "viewFactorMethod");
    expectInvalid("RadiationCavity;1;;;;;;;;3", "pairSetId");
    expectInvalid("RadiationCavity;1;;;;;;;;3;0", "pairSetId");
    expectInvalid("RadiationCavityList;1;5;1", "matrixKind");
    expectInvalid("RadiationCavityList;1;0;1", "matrixKind");
    expectInvalid("RadiationCavityList;1;1", "elements");
    expectInvalid("RadiationCavityList;1;1;5;Through;2", "elements");
    expectInvalid("RadiationCavityList;1;1;1;Through;9;By;2", "elements");
    expectInvalid("RadiationExchangeMatrix;1;0;1.", "columnIndex");
    expectInvalid("RadiationExchangeMatrix;1;1", "exchangeFactor");
    expectInvalid("RadiationExchangeMatrix;1;1;-1.", "exchangeFactor");
    expectInvalid("RadiationCavitySet", "cavityId");
    expectInvalid("RadiationCavitySet;0", "cavityId");
}

TEST_CASE("RADBC applies to surface elements, with an ambient point", "[model][cards][thermal]")
{
    Model model = readBulk(
        "RadiationToSpace;7;1.;107;12\nRadiationToSpace;6;0.5;;1;Through;9;By;2\nRadiationToSpace;5;0.25;;11;12");
    RadiationToSpaceCard card = cardAt<RadiationToSpaceTable>(model, 0);
    REQUIRE((card.ambientNode == 7 && card.ambientViewFactor == 1.0 && card.controlNode == 107 &&
             card.elements.size() == 1));
    REQUIRE(cardAt<RadiationToSpaceTable>(model, 1).elements == std::vector<ThermalRun>{{1, 9, 2}});
    // Several RADBC cards may share an ambient point: it is not an ID.
    REQUIRE(rowCount<RadiationToSpaceTable>(model) == 3);
    requireRoundTrip(model);
    expectInvalid("RadiationToSpace;5;0.;;1", "ambientViewFactor");
    expectInvalid("RadiationToSpace;5;1.", "elements");
    expectInvalid("RadiationToSpace;0;1.;;1", "ambientNode");
}

// ------------------------------------------------------------------------------------------------------- acoustics

TEST_CASE("PACABS keeps field 6 and has the tables or the values", "[model][cards][acoustic]")
{
    Model model = readBulk(
        "TableDynamic1;1;;;0.;1.;1.;1.;EndTable\n"
        "TableDynamic1;2;;;0.;1.;1.;1.;EndTable\n"
        "TableDynamic1;3;;;0.;1.;1.;1.;EndTable\n"
        "AcousticAbsorberBrickProp;12;;1;2;3;4.5;450.\n"
        "AcousticAbsorberBrickProp;13;No;;;;;;2.5;3.5;4.5\n"
        "AcousticAbsorberBrickProp;14;Yes;1;2;;;500.");
    AcousticAbsorberBrickPropCard card = cardAt<AcousticAbsorberBrickPropTable>(model, 0);
    REQUIRE((card.id == 12 && card.synthesize == "Yes" && card.resistanceTableId == 1 && card.reactanceTableId == 2 &&
             card.weightingTableId == 3));
    REQUIRE((card.reservedValue.value() == 4.5 && card.cutoffFrequency.value() == 450.0 && !card.damping.has_value()));
    card = cardAt<AcousticAbsorberBrickPropTable>(model, 1);
    REQUIRE((card.synthesize == "No" && card.damping.value() == 2.5 && card.stiffness.value() == 3.5 &&
             card.mass.value() == 4.5));
    REQUIRE(cardAt<AcousticAbsorberBrickPropTable>(model, 2).weightingTableId == 0);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(10));
    REQUIRE(cardAt<AcousticAbsorberBrickPropTable>(model, 0).reactanceTableId == 12);
    expectInvalid("AcousticAbsorberBrickProp;1;MAYBE", "synthesize");
    expectInvalid("AcousticAbsorberBrickProp;1;Yes;;2;;;500.", "resistanceTableId");
    expectInvalid("AcousticAbsorberBrickProp;1;Yes;1;;;;500.", "resistanceTableId");
    expectInvalid("AcousticAbsorberBrickProp;1;Yes;1;2", "cutoffFrequency");
    expectInvalid("AcousticAbsorberBrickProp;1;No;;;;;-5.", "cutoffFrequency");
    expectInvalid("AcousticAbsorberBrickProp;1;No;;;;;;-1.", "damping");
}

TEST_CASE("PACBAR gives the resonance as a frequency or a stiffness, PAABSF the impedance", "[model][cards][acoustic]")
{
    const Model model = readBulk(
        "AcousticBarrierProp;22;1.5;0.02;350.\nAcousticBarrierProp;23;2.;0.02;;5.E+6\nAcousticAbsorberProp;44;36;"
        "49\nAcousticAbsorberProp;45;;;2.;3.;0.1;0.2;500.");
    AcousticBarrierPropCard bar = cardAt<AcousticBarrierPropTable>(model, 0);
    REQUIRE((bar.backingMass == 1.5 && bar.septumMass == 0.02 && bar.resonanceFrequency.value() == 350.0 &&
             !bar.resonanceStiffness.has_value()));
    REQUIRE(cardAt<AcousticBarrierPropTable>(model, 1).resonanceStiffness.value() == 5.0e6);
    AcousticAbsorberPropCard absorber = cardAt<AcousticAbsorberPropTable>(model, 0);
    REQUIRE((absorber.resistanceTableId == 36 && absorber.reactanceTableId == 49 && absorber.impedanceScale == 1.0 &&
             absorber.area == 1.0));
    REQUIRE((absorber.damping == 0.0 && absorber.stiffness == 0.0 && absorber.characteristicImpedance == 1.0));
    absorber = cardAt<AcousticAbsorberPropTable>(model, 1);
    REQUIRE((absorber.impedanceScale == 2.0 && absorber.area == 3.0 && absorber.damping == 0.1 &&
             absorber.stiffness == 0.2 && absorber.characteristicImpedance == 500.0));
    requireRoundTrip(model);
    expectInvalid("AcousticBarrierProp;1;1.;1.;5.;6.", "resonanceStiffness");
    expectInvalid("AcousticBarrierProp;1;1.;1.", "resonanceFrequency");
    expectInvalid("AcousticBarrierProp;1;0.;1.;5.", "backingMass");
    expectInvalid("AcousticAbsorberProp;1", "resistanceTableId");
    expectInvalid("AcousticAbsorberProp;1;;;;0.;1.", "area");
    expectInvalid("AcousticAbsorberProp;1;;;;;-1.", "damping");
}

TEST_CASE("CAABSF has one to four fluid points, CHACAB and CHACBR eight corners and edge points",
          "[model][cards][acoustic]")
{
    Model model = readBulk(
        "AcousticAbsorber;44;36;1;11;21\n"
        "AcousticAbsorber;45;;5\n"
        "AcousticAbsorber;46;3;1;2;3;4\n"
        "AcousticAbsorberHexa;95;12;1;2;6;7;8;10;25;23\n"
        "AcousticAbsorberHexa;96;12;1;2;3;4;5;6;7;8;9;10;11;12;17;18;19;20\n"
        "AcousticBarrierHexa;195;22;1;2;6;7;8;10;25;23");
    AcousticAbsorberCard absorber = cardAt<AcousticAbsorberTable>(model, 0);
    REQUIRE(
        (absorber.id == 44 && absorber.propId == 36 && absorber.nodes == std::array<base::EntityId, 4>{1, 11, 21, 0}));
    REQUIRE(cardAt<AcousticAbsorberTable>(model, 1).propId == 45); // a blank PID is the EID
    REQUIRE(cardAt<AcousticAbsorberTable>(model, 2).nodes[3] == 4);
    AcousticAbsorberHexaCard brick = cardAt<AcousticAbsorberHexaTable>(model, 0);
    REQUIRE(
        (brick.id == 95 && brick.propId == 12 && brick.nodes[0] == 1 && brick.nodes[7] == 23 && brick.nodes[8] == 0));
    brick = cardAt<AcousticAbsorberHexaTable>(model, 1);
    REQUIRE((brick.nodes[11] == 12 && brick.nodes[12] == 17 && brick.nodes[15] == 20));
    REQUIRE(cardAt<AcousticBarrierHexaTable>(model).propId == 22);
    requireRoundTrip(model);
    // The acoustic elements are elements of the common space and have properties (PACABS, PACBAR, PAABSF) of the common
    // space.
    REQUIRE_THROWS_AS(readBulk("AcousticAbsorber;1;1;1\nAcousticAbsorberHexa;1;1;1;2;3;4;5;6;7;8"),
                      base::DeckError);
    REQUIRE_THROWS_AS(readBulk("AcousticBarrierProp;1;1.;1.;5.\nAcousticAbsorberProp;1;1"), base::DeckError);
    Model references = readBulk("Node;1;;0.;0.;0.\nAcousticAbsorberProp;5;1\nAcousticAbsorber;1;5;1");
    RecordingVisitor visitor;
    references.table<AcousticAbsorberTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.front() == "propId");
    REQUIRE(visitor.targets.front() == ReferenceTarget::Property);
    expectInvalid("AcousticAbsorber;1;1", "node1");
    expectInvalid("AcousticAbsorber;1;1;1;;3", "node");
    expectInvalid("AcousticAbsorberHexa;1;1;1;2;3;4;5;6;7", "node8");
    expectInvalid("AcousticAbsorberHexa;1;1;1;2;3;4;5;6;7;0", "node");
    expectInvalid("AcousticAbsorberHexa;1;1;1;2;3;4;5;6;7;7", "node");
}

TEST_CASE("ACMODL has the layout of BW and BWKD or the shorter one of CP, and NORMAL depends on INTER",
          "[model][cards][acoustic]")
{
    const Model model = readBulk("FluidStructureInterface;IdenticalMesh;All;1212;3434;0.001");
    const FluidStructureInterfaceCard card = cardAt<FluidStructureInterfaceTable>(model);
    REQUIRE((card.interfaceKind == "IdenticalMesh" && card.interfaceInput == "All" && card.fluidSetId == 1212 &&
             card.structureSetId == 3434));
    REQUIRE((card.normalTolerance == 0.001 && card.methodName == "SurfaceCoupling" && card.skinTolerance == 0.5 &&
             card.skinToleranceIncrement == 0.75));
    REQUIRE((card.interfaceTolerance == 0.5 && card.wholeStructureSet == "No" && card.searchUnit == "Relative"));
    // A blank NORMAL is 1.0, or 0.001 for IDENT: stored resolved and always written.
    REQUIRE(cardAt<FluidStructureInterfaceTable>(readBulk("FluidStructureInterface;DifferenceForm")).normalTolerance ==
            1.0);
    REQUIRE(cardAt<FluidStructureInterfaceTable>(readBulk("FluidStructureInterface;IdenticalMesh")).normalTolerance ==
            0.001);

    const FluidStructureInterfaceCard full =
        cardAt<FluidStructureInterfaceTable>(readBulk("FluidStructureInterface;DifferenceForm;NodesList;10;20;0.5;"
                                                      "ScaledSurfaceCoupling;0.4;0.6;0.3;Yes;Absolute"));
    REQUIRE((full.methodName == "ScaledSurfaceCoupling" && full.skinTolerance == 0.4 &&
             full.skinToleranceIncrement == 0.6 && full.interfaceTolerance == 0.3));
    REQUIRE((full.wholeStructureSet == "Yes" && full.searchUnit == "Absolute" && full.interfaceInput == "NodesList"));
    const FluidStructureInterfaceCard pressure =
        cardAt<FluidStructureInterfaceTable>(readBulk("FluidStructureInterface;DifferenceForm;All;;;;ClosedPressure"));
    REQUIRE((pressure.methodName == "ClosedPressure" && pressure.interfaceInput == "All"));
    requireRoundTrip(model);
    // The sets of the interface are SET1 cards.
    Model sets = readBulk("IdListSet1;10;1;2\nIdListSet1;20;3\nFluidStructureInterface;DifferenceForm;NodesList;10;20");
    REQUIRE(unresolvedIn(sets) == 0);
    Model lacking = readBulk("FluidStructureInterface;DifferenceForm;NodesList;10;20");
    REQUIRE(unresolvedIn(lacking) == 2);
    Renumbering::apply(sets, RenumberSpace::Set, Renumbering::offset(5));
    REQUIRE((cardAt<FluidStructureInterfaceTable>(sets).fluidSetId == 15 &&
             cardAt<FluidStructureInterfaceTable>(sets).structureSetId == 25));
    // Only one ACMODL is allowed.
    const Model twice = readModelText(
        "[model]\nFluidStructureInterface;IdenticalMesh\nFluidStructureInterface;DifferenceForm\n", false);
    REQUIRE(twice.diagnostics().firstError()->code == "deck.duplicate-fluid-structure-interface");

    expectInvalid("FluidStructureInterface;SAME", "interfaceKind");
    expectInvalid("FluidStructureInterface;DifferenceForm;SOME", "interfaceInput");
    expectInvalid("FluidStructureInterface;DifferenceForm;;;;;SW", "methodName");
    expectInvalid("FluidStructureInterface;DifferenceForm;NodesList;;;;ClosedPressure", "interfaceInput");
    expectInvalid("FluidStructureInterface;DifferenceForm;;;;;ClosedPressure;0.4", "");
    expectInvalid("FluidStructureInterface;DifferenceForm;;;;0.", "normalTolerance");
    expectInvalid("FluidStructureInterface;DifferenceForm;;;;;;-1.", "skinTolerance");
    expectInvalid("FluidStructureInterface;DifferenceForm;;;;;;;;;MAYBE", "wholeStructureSet");
}

TEST_CASE("MFLUID, FLSYM and PANEL have their words and lists", "[model][cards][acoustic]")
{
    Model model = readBulk(
        "RectFrame2;2\n"
        "FluidVirtualMass;3;2;12.65;998.;3;4;Symmetric;NoSymmetry;;80.\n"
        "FluidVirtualMass;4;;;1.E+3;5;;NoSymmetry;NoSymmetry\n"
        "FluidMassElemList;3;1\n"
        "FluidMassElemList;4;2\n"
        "FluidMassElemList;5;-3\n"
        "AxisymFluidSymmetry;8;Symmetric;Antisymmetric\n"
        "FluidStructurePanels;REARLID;107\n"
        "FluidStructurePanels;A;1;B;2;C;3;D;4;E;5\n"
        "IdListSet1;107;1;2\n"
        "IdListSet1;1;1\n"
        "IdListSet1;2;2\n"
        "IdListSet1;3;3\n"
        "IdListSet1;4;4\n"
        "IdListSet1;5;5");
    FluidVirtualMassCard fluid = cardAt<FluidVirtualMassTable>(model, 0);
    REQUIRE(
        (fluid.id == 3 && fluid.frameId == 2 && fluid.freeSurfaceZ.value() == Approx(12.65) && fluid.density == 998.0));
    REQUIRE((fluid.oneSideElemListId == 3 && fluid.bothSidesElemListId == 4 && fluid.planeSymmetry1 == "Symmetric" &&
             fluid.planeSymmetry2 == "NoSymmetry"));
    REQUIRE((fluid.characteristicLength == 1.0e10 && fluid.exactIntegrationRatio == 80.0));
    fluid = cardAt<FluidVirtualMassTable>(model, 1);
    REQUIRE((!fluid.freeSurfaceZ.has_value() && fluid.frameId == 0 && fluid.exactIntegrationRatio == 1.0e-15));
    REQUIRE(cardAt<AxisymFluidSymmetryTable>(model).sectionCount == 8);
    const FluidStructurePanelsCard panel = cardAt<FluidStructurePanelsTable>(model, 1);
    REQUIRE((panel.names == std::vector<std::string>{"A", "B", "C", "D", "E"} &&
             panel.sets == std::vector<base::EntityId>{1, 2, 3, 4, 5}));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
    const Model twice = readModelText(
        "[model]\nAxisymFluidSymmetry;4;Symmetric;Symmetric\nAxisymFluidSymmetry;4;Antisymmetric;Antisymmetric\n",
        false);
    REQUIRE(twice.diagnostics().firstError()->code == "deck.duplicate-axisym-fluid-symmetry");
    expectInvalid("FluidVirtualMass;1;;;1.E+3;5;;X;NoSymmetry", "planeSymmetry1");
    expectInvalid("FluidVirtualMass;1;;;1.E+3;;;NoSymmetry;NoSymmetry", "oneSideElemListId");
    expectInvalid("FluidVirtualMass;1;;;1.E+3;5;;NoSymmetry", "planeSymmetry2");
    expectInvalid("FluidVirtualMass;1;;;1.E+3;5;;NoSymmetry;NoSymmetry;0.", "characteristicLength");
    expectInvalid("AxisymFluidSymmetry;3;Symmetric;Symmetric", "sectionCount");
    expectInvalid("AxisymFluidSymmetry;4;Symmetric", "planeSymmetry2");
    expectInvalid("AxisymFluidSymmetry;4;Symmetric;X", "planeSymmetry2");
    expectInvalid("FluidStructurePanels;REARLID", "panelSetId");
    expectInvalid("FluidStructurePanels;;3", "panelName");
    expectInvalid("FluidStructurePanels", "panelName");
}

TEST_CASE("FSLIST and BDYLIST list the rings of an axisymmetric fluid, with AXIS first or LAXIS last",
          "[model][cards][acoustic]")
{
    const Model model = readBulk(
        "AxisymFluidFreeSurface;2.E-4;Axis;431;327;418;205;258;177\n"
        "AxisymFluidFreeSurface;;431;327;LocalAxis\n"
        "AxisymFluidBoundaryList;0.041;Axis;431;327\n"
        "AxisymFluidBoundaryList;;2;8;13\n"
        "AxisymFluidBoundaryList;0.;1;2\n"
        "AxisymFluid2;101;1;7\n"
        "AxisymFluid2;100;11;14;0.031;0.\n"
        "AxisymFluid3;103;7;8;13\n"
        "AxisymFluid3;110;15;13;12;1.1;4.E+5\n"
        "AxisymFluid4;104;1;2;7;8\n"
        "AxisymFluid4;120;11;15;12;14;1.1");
    AxisymFluidFreeSurfaceCard list = cardAt<AxisymFluidFreeSurfaceTable>(model, 0);
    REQUIRE((list.density.value() == Approx(2e-4) && list.points.front() == AXIS_POINT && list.points.size() == 7));
    REQUIRE(cardAt<AxisymFluidFreeSurfaceTable>(model, 1).points.back() == LAST_AXIS_POINT);
    REQUIRE(cardAt<AxisymFluidBoundaryListTable>(model, 0).points.front() == AXIS_POINT);
    REQUIRE(cardAt<AxisymFluidBoundaryListTable>(model, 2).density.value() == 0.0);
    REQUIRE(cardAt<AxisymFluid2Table>(model, 1).bulkModulus.value() == 0.0);
    REQUIRE(cardAt<AxisymFluid3Table>(model, 1).rings == std::array<base::EntityId, 3>{15, 13, 12});
    REQUIRE(cardAt<AxisymFluid4Table>(model, 1).density.value() == 1.1);
    requireRoundTrip(model);
    // The fluid elements are elements of the common space.
    REQUIRE_THROWS_AS(readBulk("AxisymFluid2;1;1;2\nAxisymFluid3;1;1;2;3"), base::DeckError);
    expectInvalid("AxisymFluidFreeSurface;1.;431;Axis", "fluidPointId");
    expectInvalid("AxisymFluidFreeSurface;1.;LocalAxis;431", "fluidPointId");
    expectInvalid("AxisymFluidFreeSurface;1.", "fluidPointId");
    expectInvalid("AxisymFluidFreeSurface;-1.;1;2", "density");
    expectInvalid("AxisymFluidBoundaryList;-1.;1;2", "density");
    expectInvalid("AxisymFluid2;1;1;1", "fluidPointId");
    expectInvalid("AxisymFluid2;1;1;500000", "fluidPointId");
    expectInvalid("AxisymFluid3;1;1;2", "fluidPointId3");
    expectInvalid("AxisymFluid4;1;1;2;3", "fluidPointId4");
}
