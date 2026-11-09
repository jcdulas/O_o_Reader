// Checks of the Euler-Lagrange coupling and porosity cards of the explicit solver (SOL 700): COUPLE,
// COUCOHF, COHFRIC, COUP1FL, COUPINT, FFCONTR, LEAKAGE and the porosity models PERMEAB, PERMGBG, PORFCPL, PORFGBG,
// PORFLOW, PORFLWT, PORHOLE, PORHYDS and PORUDS. Per card: the fields in their format reference positions, the
// defaults, the references it lists and their renumbering, the refused values and the round trip through the three
// formats.

#include "oor/model/cards/explicit/cohesiveFriction/CohesiveFrictionTable.hpp"
#include "oor/model/cards/explicit/couplingCohesiveFriction/CouplingCohesiveFrictionTable.hpp"
#include "oor/model/cards/explicit/couplingFailedSegmentFlow/CouplingFailedSegmentFlowTable.hpp"
#include "oor/model/cards/explicit/couplingHoleToCoupling/CouplingHoleToCouplingTable.hpp"
#include "oor/model/cards/explicit/couplingHoleToGasBag/CouplingHoleToGasBagTable.hpp"
#include "oor/model/cards/explicit/couplingInteraction/CouplingInteractionTable.hpp"
#include "oor/model/cards/explicit/couplingOptions/CouplingOptionsTable.hpp"
#include "oor/model/cards/explicit/eulerLagrangeCoupling/EulerLagrangeCouplingTable.hpp"
#include "oor/model/cards/explicit/fluidFilledContainer/FluidFilledContainerTable.hpp"
#include "oor/model/cards/explicit/gasBagLeakage/GasBagLeakageTable.hpp"
#include "oor/model/cards/explicit/gasBagPermeableLink/GasBagPermeableLinkTable.hpp"
#include "oor/model/cards/explicit/gasBagVentHole/GasBagVentHoleTable.hpp"
#include "oor/model/cards/explicit/porousFlowBoundary/PorousFlowBoundaryTable.hpp"
#include "oor/model/cards/explicit/porousHydrostaticBoundary/PorousHydrostaticBoundaryTable.hpp"
#include "oor/model/cards/explicit/porousTimeFlowBoundary/PorousTimeFlowBoundaryTable.hpp"
#include "oor/model/cards/explicit/porousUserBoundary/PorousUserBoundaryTable.hpp"
#include "oor/model/cards/explicit/surfacePermeability/SurfacePermeabilityTable.hpp"
#include "support/ExplicitTestSupport.hpp"
#include "support/SlotRecord.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The element 1 that every BSURF of these decks names (a BSURF lists elements, which must exist): one quadrilateral,
// with ID 9000 for its property and its material so that they do not meet the IDs the tests renumber.
const std::string SURFACE_ELEMENT =
    "\n"
    "Node;1\n"
    "Node;2;;1.\n"
    "Node;3;;1.;1.\n"
    "Node;4;;0.;1.\n"
    "ShellQuad4;1;9000;1;2;3;4\n"
    "ShellProp;9000;9000;0.1\n"
    "IsoMat;9000;2.1E+11;;0.3";

using GivenFields = std::vector<std::pair<int, std::string>>;

// The record of a card with the given slots (format reference numbering, slotRecord) and every other slot blank, so
// that a test names the field it means and not a count of separators.
std::string cardText(const std::string& name, const GivenFields& given)
{
    int last = 0;
    for (const auto& [field, value] : given)
        last = std::max(last, field);
    std::vector<std::string> fields(static_cast<std::size_t>(last));
    for (const auto& [field, value] : given)
        fields[static_cast<std::size_t>(field - 1)] = value;
    return slotRecord(name, fields);
}

// A COUPLE with its two required fields and one more.
std::string coupleWith(int field, const std::string& value)
{
    return cardText("EulerLagrangeCoupling", {{1, "1"}, {2, "10"}, {field, value}});
}

} // namespace

TEST_CASE("a COUPLE has its 23 fields in the format reference positions over three lines and writes the ones it has",
          "[model][cards][explicit]")
{
    const Model model = readBulk(
        "EulerLagrangeCoupling;1;10\n"
        "EulerLagrangeCoupling;2;20;Outside;Off;Off;30;31;AirbagCoupling;32;33;0.4;0.3;5.;34;35;;0.0025;36;37;0.05;38\n"
        "EulerLagrangeCoupling;3;21;None;;;;;;42;;;;;;;45");
    const EulerLagrangeCouplingCard bare = cardAt<EulerLagrangeCouplingTable>(model, 0);
    REQUIRE((bare.id == 1 && bare.surfaceId == 10));
    REQUIRE((bare.cover == CoverSide::Inside && bare.reverseNormals == Switch::On &&
             bare.checkNormals == Switch::On)); // the defaults
    REQUIRE((bare.porosityId == 0 && bare.optionId == 0 && bare.couplingType == EulerLagrangeCouplingType::Standard &&
             bare.inflatorId == 0));
    REQUIRE((bare.heatTransferId == 0 && bare.staticFriction == 0.0 && bare.kineticFriction == 0.0 &&
             bare.frictionDecay == 0.0 && bare.initialGasId == 0));
    REQUIRE((bare.elemSetId == 0 && bare.meshId == 0 &&
             bare.deactivationTime == EULER_LAGRANGE_COUPLING_DEFAULT_DEACTIVATION_TIME &&
             bare.failedSegmentFlowId == 0));
    REQUIRE((bare.hydrostaticId == 0 && bare.skinFriction == 0.0 && bare.cohesiveFrictionSetId == 0));

    const EulerLagrangeCouplingCard full = cardAt<EulerLagrangeCouplingTable>(model, 1);
    REQUIRE((full.id == 2 && full.surfaceId == 20 && full.cover == CoverSide::Outside &&
             full.reverseNormals == Switch::Off));
    REQUIRE((full.checkNormals == Switch::Off && full.porosityId == 30 && full.optionId == 31 &&
             full.couplingType == EulerLagrangeCouplingType::Airbag));
    REQUIRE((full.inflatorId == 32 && full.heatTransferId == 33 && full.staticFriction == 0.4 &&
             full.kineticFriction == 0.3 && full.frictionDecay == 5.0));
    REQUIRE((full.initialGasId == 34 && full.elemSetId == 35 && full.meshId == 0 && full.deactivationTime == 0.0025));
    REQUIRE((full.failedSegmentFlowId == 36 && full.hydrostaticId == 37 && full.skinFriction == 0.05 &&
             full.cohesiveFrictionSetId == 38));

    const EulerLagrangeCouplingCard meshed = cardAt<EulerLagrangeCouplingTable>(model, 2);
    REQUIRE(
        (meshed.cover == CoverSide::None && meshed.inflatorId == 42 && meshed.elemSetId == 0 && meshed.meshId == 45));
    requireRoundTrip(model);
}

TEST_CASE("a COUPLE refuses what its values and its grid do not allow", "[model][cards][explicit]")
{
    expectInvalid("EulerLagrangeCoupling;0;10", "id");
    expectInvalid("EulerLagrangeCoupling;1", "surfaceId");
    expectInvalid("EulerLagrangeCoupling;1;0", "surfaceId");
    expectInvalid("EulerLagrangeCoupling;1;10;SIDE", "cover");
    expectInvalid("EulerLagrangeCoupling;1;10;;YES", "reverseNormals");
    expectInvalid("EulerLagrangeCoupling;1;10;;;MAYBE", "checkNormals");
    expectInvalid("EulerLagrangeCoupling;1;10;;;;-1", "porosityId");
    expectInvalid("EulerLagrangeCoupling;1;10;;;;;-1", "optionId");
    expectInvalid("EulerLagrangeCoupling;1;10;;;;;;SPECIAL", "couplingType");
    expectInvalid(coupleWith(9, "-1"), "inflatorId");
    expectInvalid(coupleWith(10, "-1"), "heatTransferId");
    expectInvalid(coupleWith(11, "-0.5"), "staticFriction");
    expectInvalid(coupleWith(12, "-0.5"), "kineticFriction");
    expectInvalid(coupleWith(13, "-0.5"), "frictionDecay");
    expectInvalid(coupleWith(14, "-1"), "initialGasId");
    expectInvalid(coupleWith(17, "-1"), "elemSetId");
    expectInvalid(coupleWith(18, "-1"), "meshId");
    expectInvalid(coupleWith(19, "0."), "deactivationTime");
    expectInvalid(coupleWith(20, "-1"), "failedSegmentFlowId");
    expectInvalid(coupleWith(21, "-1"), "hydrostaticId");
    expectInvalid(coupleWith(22, "-0.5"), "skinFriction");
    expectInvalid(coupleWith(23, "-1"), "cohesiveFrictionSetId");
    // The Euler domain is tied to the surface by its elements or by its mesh, not by both.
    expectInvalid(cardText("EulerLagrangeCoupling", {{1, "1"}, {2, "10"}, {17, "35"}, {18, "45"}}), "meshId");
    // A number is unique among the COUPLE cards.
    REQUIRE_THROWS_AS(readBulk("EulerLagrangeCoupling;1;10\nEulerLagrangeCoupling;1;11"), base::DeckError);
}

TEST_CASE("a COUPLE names the cards of its coupling, and each reference follows its renumbering",
          "[model][cards][explicit]")
{
    const std::string coupling = "EulerLagrangeCoupling;1;10;;;;3;4;;;99;;;;;11;;;8;;;5";
    Model model = readBulk(
        "ContactElemSurface;10;1\n"
        "IdListSet1;11;1;2;3\n"
        "CouplingOptions;50;4;10\n"
        "GasBagLeakage;60;3;10;GasBagVentHole;9\n"
        "GasBagVentHole;9;;;1.E+5;1.;2.E+5\n"
        "CouplingCohesiveFriction;70;5;10;6\n"
        "CohesiveFriction;6\n"
        "CouplingFailedSegmentFlow;8;1.2\n"
        "GasBagHeatTransfer;1;99;;GasBagConvection;2\n"
        "GasBagConvection;2;12.5;;300.\n" +
        coupling + SURFACE_ELEMENT);
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<EulerLagrangeCouplingTable>().visitReferences(0, visitor);
    // HTRID names a set of HEATLOS cards.
    REQUIRE(visitor.fields == std::vector<std::string>{"surfaceId", "porosityId", "optionId", "heatTransferId",
                                                       "elemSetId", "failedSegmentFlowId", "cohesiveFrictionSetId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{
                                   ReferenceTarget::ContactSurface, ReferenceTarget::Leakage,
                                   ReferenceTarget::CouplingOptions, ReferenceTarget::HeatLossSet, ReferenceTarget::Set,
                                   ReferenceTarget::CouplingFailure, ReferenceTarget::CohesiveFrictionSet});

    // The other references, on a card that has them.
    Model other = readBulk(
        cardText("EulerLagrangeCoupling", {{1, "2"}, {2, "10"}, {9, "32"}, {14, "34"}, {18, "45"}, {21, "37"}}));
    RecordingVisitor otherVisitor;
    other.table<EulerLagrangeCouplingTable>().visitReferences(0, otherVisitor);
    REQUIRE(otherVisitor.fields ==
            std::vector<std::string>{"surfaceId", "inflatorId", "initialGasId", "meshId", "hydrostaticId"});
    REQUIRE(otherVisitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::ContactSurface, ReferenceTarget::InflatorSet,
                                         ReferenceTarget::InitialGas, ReferenceTarget::EulerMesh,
                                         ReferenceTarget::HydrostaticPreset});

    Renumbering::apply(model, RenumberSpace::Coupling, Renumbering::offset(100));
    Renumbering::apply(model, RenumberSpace::Leakage, Renumbering::offset(1000));
    Renumbering::apply(model, RenumberSpace::CouplingOptions, Renumbering::offset(10));
    Renumbering::apply(model, RenumberSpace::CohesiveFrictionSet, Renumbering::offset(20));
    Renumbering::apply(model, RenumberSpace::CohesiveFriction, Renumbering::offset(30));
    Renumbering::apply(model, RenumberSpace::CouplingFailure, Renumbering::offset(40));
    Renumbering::apply(model, RenumberSpace::Porosity, Renumbering::offset(50));
    Renumbering::apply(model, RenumberSpace::ContactSurface, Renumbering::offset(60));
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(70));
    Renumbering::apply(model, RenumberSpace::HeatLossSet, Renumbering::offset(1));
    const EulerLagrangeCouplingCard renumbered = cardAt<EulerLagrangeCouplingTable>(model);
    REQUIRE((renumbered.id == 101 && renumbered.surfaceId == 70 && renumbered.porosityId == 1003 &&
             renumbered.optionId == 14));
    REQUIRE(
        (renumbered.elemSetId == 81 && renumbered.failedSegmentFlowId == 48 && renumbered.cohesiveFrictionSetId == 25));
    REQUIRE(renumbered.heatTransferId == 100); // the set of HEATLOS cards (its cards are renumbered with it)
    const GasBagLeakageCard leakage = cardAt<GasBagLeakageTable>(model);
    REQUIRE((leakage.setId == 1003 && leakage.subsurfaceId == 70 && leakage.porosityModelId == 59));
    const CouplingCohesiveFrictionCard friction = cardAt<CouplingCohesiveFrictionTable>(model);
    REQUIRE((friction.setId == 25 && friction.subsurfaceId == 70 && friction.cohesiveFrictionModelId == 36));
    REQUIRE(
        (cardAt<CouplingOptionsTable>(model).setId == 14 && cardAt<CouplingOptionsTable>(model).subsurfaceId == 70));
    REQUIRE(cardAt<CohesiveFrictionTable>(model).id == 36);
    REQUIRE(cardAt<CouplingFailedSegmentFlowTable>(model).id == 48);
    REQUIRE(cardAt<GasBagVentHoleTable>(model).id == 59);
    REQUIRE(unresolvedIn(model) == 0);

    Model dangling = readBulk(coupling);
    REQUIRE(unresolvedIn(dangling) == 7);
    requireRoundTrip(model);
}

TEST_CASE("a COUCOHF is filed under the set ID of its COHFRID, and a COHFRIC holds the friction",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "CouplingCohesiveFriction;1;14;12;16\n"
        "CouplingCohesiveFriction;2;14;;17\n"
        "CouplingCohesiveFriction;3;15;;16\n"
        "ContactElemSurface;12;1\n"
        "CohesiveFriction;16;6.E+10;4.E+5;3.\n"
        "CohesiveFriction;17" +
        SURFACE_ELEMENT);
    const CouplingCohesiveFrictionCard part = cardAt<CouplingCohesiveFrictionTable>(model, 0);
    REQUIRE((part.id == 1 && part.setId == 14 && part.subsurfaceId == 12 && part.cohesiveFrictionModelId == 16));
    REQUIRE(cardAt<CouplingCohesiveFrictionTable>(model, 1).subsurfaceId == 0); // the whole coupling surface
    REQUIRE(rowCount<CouplingCohesiveFrictionTable>(model) == 3);
    REQUIRE(model.table<CouplingCohesiveFrictionTable>().idAtRow(0) ==
            model.table<CouplingCohesiveFrictionTable>().idAtRow(1));
    REQUIRE(model.table<CouplingCohesiveFrictionTable>().idAtRow(2) == 15);
    const CohesiveFrictionCard strong = cardAt<CohesiveFrictionTable>(model, 0);
    REQUIRE(
        (strong.id == 16 && strong.maxStress == 6.0E10 && strong.friction == 4.0E5 && strong.referenceVelocity == 3.0));
    const CohesiveFrictionCard bare = cardAt<CohesiveFrictionTable>(model, 1);
    REQUIRE((bare.maxStress == 0.0 && bare.friction == 0.0 && bare.referenceVelocity == 0.0)); // the defaults
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<CouplingCohesiveFrictionTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"subsurfaceId", "cohesiveFrictionModelId"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::ContactSurface, ReferenceTarget::CohesiveFriction});
    visitor = RecordingVisitor{};
    model.table<CouplingCohesiveFrictionTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"cohesiveFrictionModelId"});

    Renumbering::apply(model, RenumberSpace::CohesiveFrictionSet, Renumbering::offset(100));
    REQUIRE((cardAt<CouplingCohesiveFrictionTable>(model, 0).setId == 114 &&
             cardAt<CouplingCohesiveFrictionTable>(model, 2).setId == 115));
    Renumbering::apply(model, RenumberSpace::CohesiveFriction, Renumbering::offset(1000));
    REQUIRE((cardAt<CouplingCohesiveFrictionTable>(model, 0).cohesiveFrictionModelId == 1016 &&
             cardAt<CohesiveFrictionTable>(model, 1).id == 1017));
    Renumbering::apply(model, RenumberSpace::ContactSurface, Renumbering::offset(500));
    REQUIRE(cardAt<CouplingCohesiveFrictionTable>(model, 0).subsurfaceId == 512);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    Model dangling = readBulk("CouplingCohesiveFriction;1;14;99;98");
    REQUIRE(unresolvedIn(dangling) == 2);

    expectInvalid("CouplingCohesiveFriction;0;14;;12", "id");
    expectInvalid("CouplingCohesiveFriction;1;0;;12", "setId");
    expectInvalid("CouplingCohesiveFriction;1;14;-1;12", "subsurfaceId");
    expectInvalid("CouplingCohesiveFriction;1;14", "cohesiveFrictionModelId");
    expectInvalid("CohesiveFriction;0", "id");
    expectInvalid("CohesiveFriction;1;-1.", "maxStress");
    expectInvalid("CohesiveFriction;1;;-1.", "friction");
    expectInvalid("CohesiveFriction;1;;;-1.", "referenceVelocity");
    REQUIRE_THROWS_AS(readBulk("CohesiveFriction;1\nCohesiveFriction;1"), base::DeckError);
}

TEST_CASE("an FFCONTR names its surface and two tables, and keeps TACTIVE blank", "[model][cards][explicit]")
{
    Model model = readBulk(
        "FluidFilledContainer;1;2;0.0015;1.E+5\n"
        "FluidFilledContainer;2;3;0.0015;1.E+5;10;20\n"
        "FluidFilledContainer;3;4;0.002;1.E+5;11;21;0.05\n"
        "ContactElemSurface;2;1\n"
        "ContactElemSurface;3;1\n"
        "ContactElemSurface;4;1\n"
        "TableDynamic1;10;;;0.;1.;1.;1.;EndTable\n"
        "TableDynamic1;20;;;0.;1.;1.;1.;EndTable\n"
        "TableDynamic1;11;;;0.;1.;1.;1.;EndTable\n"
        "TableDynamic1;21;;;0.;1.;1.;1.;EndTable" +
        SURFACE_ELEMENT);
    const FluidFilledContainerCard minimal = cardAt<FluidFilledContainerTable>(model, 0);
    REQUIRE((minimal.id == 1 && minimal.surfaceId == 2 && minimal.fluidVolume == 1.5E-3 &&
             minimal.atmosphericPressure == 1.0E5));
    REQUIRE((minimal.temperatureTableId == 0 && minimal.densityTableId == 0 && !minimal.activationTime));
    const FluidFilledContainerCard tables = cardAt<FluidFilledContainerTable>(model, 1);
    REQUIRE((tables.temperatureTableId == 10 && tables.densityTableId == 20 && !tables.activationTime));
    REQUIRE(cardAt<FluidFilledContainerTable>(model, 2).activationTime == 0.05);
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<FluidFilledContainerTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"surfaceId", "temperatureTableId", "densityTableId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::ContactSurface,
                                                            ReferenceTarget::DynamicTable,
                                                            ReferenceTarget::DynamicTable});
    Renumbering::apply(model, RenumberSpace::ContactSurface, Renumbering::offset(100));
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(1000));
    REQUIRE((cardAt<FluidFilledContainerTable>(model, 1).surfaceId == 103 &&
             cardAt<FluidFilledContainerTable>(model, 1).temperatureTableId == 1010));
    REQUIRE(cardAt<FluidFilledContainerTable>(model, 2).densityTableId == 1021);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    expectInvalid("FluidFilledContainer;0;2;1.;1.E+5", "id");
    expectInvalid("FluidFilledContainer;1;0;1.;1.E+5", "surfaceId");
    expectInvalid("FluidFilledContainer;1;2", "fluidVolume");
    expectInvalid("FluidFilledContainer;1;2;-1.;1.E+5", "fluidVolume");
    expectInvalid("FluidFilledContainer;1;2;1.", "atmosphericPressure");
    expectInvalid("FluidFilledContainer;1;2;1.;0.", "atmosphericPressure");
    expectInvalid("FluidFilledContainer;1;2;1.;1.E+5;-1", "temperatureTableId");
    expectInvalid("FluidFilledContainer;1;2;1.;1.E+5;;-1", "densityTableId");
    expectInvalid("FluidFilledContainer;1;2;1.;1.E+5;;;SOON", "activationTime");
    REQUIRE_THROWS_AS(readBulk("FluidFilledContainer;1;2;1.;1.E+5\nFluidFilledContainer;1;3;1.;1.E+5"),
                      base::DeckError);
}

TEST_CASE("a LEAKAGE is filed under the set ID of its PORID and its coefficient is a number or a table",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "GasBagLeakage;1;100;;PorousFlowBoundary;5\n"
        "GasBagLeakage;2;100;342;SurfacePermeability;57;;0.96\n"
        "GasBagLeakage;3;101;347;GasBagVentHole;58;Table;70\n"
        "GasBagLeakage;4;102;;PorousUserBoundary;59;Constant\n"
        "ContactElemSurface;342;1\n"
        "ContactElemSurface;347;1\n"
        "TableDynamic1;70;;;0.;1.;1.;1.;EndTable\n"
        "PorousFlowBoundary;5;VelocityX;1.\n"
        "SurfacePermeability;57;0.5;;;1.E+5;1.;2.E+5\n"
        "GasBagVentHole;58;;;1.E+5;1.;2.E+5\n"
        "PorousUserBoundary;59;GRP" +
        SURFACE_ELEMENT);
    const GasBagLeakageCard whole = cardAt<GasBagLeakageTable>(model, 0);
    REQUIRE((whole.id == 1 && whole.setId == 100 && whole.subsurfaceId == 0 &&
             whole.porosityKind == PorosityType::PorousFlow));
    REQUIRE(whole.porosityModelId == 5);
    REQUIRE((whole.coefficient.source == ValueSource::Constant && whole.coefficient.constant == 1.0)); // the default
    const GasBagLeakageCard part = cardAt<GasBagLeakageTable>(model, 1);
    REQUIRE((part.subsurfaceId == 342 && part.porosityKind == PorosityType::Permeability &&
             part.coefficient.constant == 0.96));
    const GasBagLeakageCard timed = cardAt<GasBagLeakageTable>(model, 2);
    REQUIRE((timed.porosityKind == PorosityType::Hole && timed.coefficient.source == ValueSource::Table));
    REQUIRE(timed.coefficient.table == 70);
    const GasBagLeakageCard user = cardAt<GasBagLeakageTable>(model, 3);
    REQUIRE((user.porosityKind == PorosityType::UserDefined && user.coefficient.constant == 1.0));

    // Two cards of one set share the ID; the other types of the porosity cards are named by their PORTYPID.
    REQUIRE(rowCount<GasBagLeakageTable>(model) == 4);
    REQUIRE(model.table<GasBagLeakageTable>().idAtRow(0) == model.table<GasBagLeakageTable>().idAtRow(1));
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<GasBagLeakageTable>().visitReferences(2, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"subsurfaceId", "porosityModelId", "coefficient"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::ContactSurface, ReferenceTarget::Porosity,
                                                            ReferenceTarget::DynamicTable});
    visitor = RecordingVisitor{};
    model.table<GasBagLeakageTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields ==
            std::vector<std::string>{"subsurfaceId", "porosityModelId"}); // a constant visits no table

    Renumbering::apply(model, RenumberSpace::Leakage, Renumbering::offset(10));
    REQUIRE((cardAt<GasBagLeakageTable>(model, 0).setId == 110 && cardAt<GasBagLeakageTable>(model, 2).setId == 111));
    Renumbering::apply(model, RenumberSpace::Porosity, Renumbering::offset(1000));
    REQUIRE((cardAt<GasBagLeakageTable>(model, 0).porosityModelId == 1005 &&
             cardAt<PorousFlowBoundaryTable>(model).id == 1005));
    Renumbering::apply(model, RenumberSpace::ContactSurface, Renumbering::offset(500));
    REQUIRE(cardAt<GasBagLeakageTable>(model, 1).subsurfaceId == 842);
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(30));
    REQUIRE(cardAt<GasBagLeakageTable>(model, 2).coefficient.table == 100);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    Model dangling = readBulk("GasBagLeakage;1;100;99;GasBagVentHole;98;Table;97");
    REQUIRE(unresolvedIn(dangling) == 3);

    expectInvalid("GasBagLeakage;0;100;;PorousFlowBoundary;5", "id");
    expectInvalid("GasBagLeakage;1;0;;PorousFlowBoundary;5", "setId");
    expectInvalid("GasBagLeakage;1;100;-1;PorousFlowBoundary;5", "subsurfaceId");
    expectInvalid("GasBagLeakage;1;100;;;5", "porosityKind");
    expectInvalid("GasBagLeakage;1;100;;PORCYL;5", "porosityKind");
    expectInvalid("GasBagLeakage;1;100;;PorousFlowBoundary", "porosityModelId");
    expectInvalid("GasBagLeakage;1;100;;PorousFlowBoundary;5;Element", "coefficientSource");
    expectInvalid("GasBagLeakage;1;100;;PorousFlowBoundary;5;;1.5", "coefficient");
    expectInvalid("GasBagLeakage;1;100;;PorousFlowBoundary;5;;-0.1", "coefficient");
    expectInvalid("GasBagLeakage;1;100;;PorousFlowBoundary;5;Table", "coefficient");
    expectInvalid("GasBagLeakage;1;100;;PorousFlowBoundary;5;Table;0", "coefficient");
}

TEST_CASE("a PERMEAB and a PERMGBG give the permeability as a linear law or a table, one of the two",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "SurfacePermeability;201;0.5;;Out;1.E+5;1.17;2.05E+5;985.\n"
        "SurfacePermeability;202;;12;;1.E+5;1.17;2.05E+5\n"
        "GasBagPermeableLink;12;;10;;2\n"
        "GasBagPermeableLink;13;0.25;;In;3\n"
        "TableDynamic1;12;;;0.;1.;1.;1.;EndTable\n"
        "TableDynamic1;10;;;0.;1.;1.;1.;EndTable");
    const SurfacePermeabilityCard law = cardAt<SurfacePermeabilityTable>(model, 0);
    REQUIRE(
        (law.id == 201 && law.permeability == 0.5 && law.permeabilityTableId == 0 && law.flow == FlowDirection::Out));
    REQUIRE((law.environmentPressure == 1.0E5 && law.environmentDensity == 1.17 && law.environmentEnergy == 2.05E5 &&
             law.environmentSpecificHeat == 985.0));
    const SurfacePermeabilityCard table = cardAt<SurfacePermeabilityTable>(model, 1);
    REQUIRE((!table.permeability && table.permeabilityTableId == 12 && table.flow == FlowDirection::Both &&
             !table.environmentSpecificHeat)); // the defaults
    const GasBagPermeableLinkCard linked = cardAt<GasBagPermeableLinkTable>(model, 0);
    REQUIRE((linked.id == 12 && !linked.permeability && linked.permeabilityTableId == 10 &&
             linked.flow == FlowDirection::Both));
    REQUIRE(linked.gasBagId == 2);
    const GasBagPermeableLinkCard inflow = cardAt<GasBagPermeableLinkTable>(model, 1);
    REQUIRE((inflow.permeability == 0.25 && inflow.permeabilityTableId == 0 && inflow.flow == FlowDirection::In &&
             inflow.gasBagId == 3));

    // The two GBAG numbers name cards of the gas bag category, which these decks do not have.
    REQUIRE(unresolvedIn(model) == 2);
    RecordingVisitor visitor;
    model.table<SurfacePermeabilityTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"permeabilityTableId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::DynamicTable});
    visitor = RecordingVisitor{};
    model.table<GasBagPermeableLinkTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"permeabilityTableId", "gasBagId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::DynamicTable, ReferenceTarget::GasBag});
    visitor = RecordingVisitor{};
    model.table<GasBagPermeableLinkTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"gasBagId"});

    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(100));
    REQUIRE((cardAt<SurfacePermeabilityTable>(model, 1).permeabilityTableId == 112 &&
             cardAt<GasBagPermeableLinkTable>(model, 0).permeabilityTableId == 110));
    requireRoundTrip(model);

    expectInvalid("SurfacePermeability;0;0.5;;;1.E+5;1.;2.E+5", "id");
    expectInvalid("SurfacePermeability;1;-0.5;;;1.E+5;1.;2.E+5", "permeability");
    expectInvalid("SurfacePermeability;1;0.5;-3;;1.E+5;1.;2.E+5", "permeabilityTableId");
    expectInvalid("SurfacePermeability;1;;;;1.E+5;1.;2.E+5", "permeability"); // neither the law nor the table
    expectInvalid("SurfacePermeability;1;0.5;;SIDEWAYS;1.E+5;1.;2.E+5", "flowDirection");
    expectInvalid("SurfacePermeability;1;0.5", "environmentPressure");
    expectInvalid("SurfacePermeability;1;0.5;;;0.;1.;2.E+5", "environmentPressure");
    expectInvalid("SurfacePermeability;1;0.5;;;1.E+5;-1.;2.E+5", "environmentDensity");
    expectInvalid("SurfacePermeability;1;0.5;;;1.E+5;1.", "environmentEnergy");
    expectInvalid("SurfacePermeability;1;0.5;;;1.E+5;1.;0.", "environmentEnergy");
    expectInvalid("SurfacePermeability;1;0.5;;;1.E+5;1.;2.E+5;0.", "environmentSpecificHeat");
    expectInvalid("GasBagPermeableLink;0;0.5;;;2", "id");
    expectInvalid("GasBagPermeableLink;1;-0.5;;;2", "permeability");
    expectInvalid("GasBagPermeableLink;1;;;;2", "permeability"); // neither the law nor the table
    expectInvalid("GasBagPermeableLink;1;;10;UP;2", "flowDirection");
    expectInvalid("GasBagPermeableLink;1;;10", "gasBagId");
    expectInvalid("GasBagPermeableLink;1;;10;;0", "gasBagId");

    Model dangling = readBulk("GasBagPermeableLink;12;;10;;2");
    REQUIRE(unresolvedIn(dangling) == 2);
}

TEST_CASE("a PORFCPL, a PORFGBG and a PORHOLE have a hole size, a flow direction and a gas", "[model][cards][explicit]")
{
    Model model = readBulk(
        "CouplingHoleToCoupling;1;Small;Both;1\n"
        "CouplingHoleToCoupling;2;;;3\n"
        "CouplingHoleToCoupling;3;Large;In;4;25\n"
        "CouplingHoleToGasBag;1;Small;Both;1\n"
        "CouplingHoleToGasBag;2;;;3\n"
        "CouplingHoleToGasBag;3;Large;Out;4;26\n"
        "GasBagVentHole;1;;;1.E+5;1.17;2.05E+5\n"
        "GasBagVentHole;301;Large;In;1.E+5;1.17;2.05E+5;985.;27\n"
        "EulerLagrangeCoupling;1;10\n"
        "EulerLagrangeCoupling;3;11\n"
        "EulerLagrangeCoupling;4;12\n"
        "ContactElemSurface;10;1\n"
        "ContactElemSurface;11;1\n"
        "ContactElemSurface;12;1\n"
        "IsoMat;25;2.1E+11;;0.3\n"
        "IsoMat;26;2.1E+11;;0.3\n"
        "IsoMat;27;2.1E+11;;0.3" +
        SURFACE_ELEMENT);
    const CouplingHoleToCouplingCard small = cardAt<CouplingHoleToCouplingTable>(model, 0);
    REQUIRE(
        (small.id == 1 && small.size == HoleSize::Small && small.flow == FlowDirection::Both && small.couplingId == 1));
    REQUIRE(small.matId == 0);
    const CouplingHoleToCouplingCard bare = cardAt<CouplingHoleToCouplingTable>(model, 1);
    REQUIRE((bare.size == HoleSize::Small && bare.flow == FlowDirection::Both && bare.couplingId == 3)); // the defaults
    const CouplingHoleToCouplingCard large = cardAt<CouplingHoleToCouplingTable>(model, 2);
    REQUIRE((large.size == HoleSize::Large && large.flow == FlowDirection::In && large.couplingId == 4 &&
             large.matId == 25));
    const CouplingHoleToGasBagCard gbg = cardAt<CouplingHoleToGasBagTable>(model, 2);
    REQUIRE((gbg.id == 3 && gbg.size == HoleSize::Large && gbg.flow == FlowDirection::Out && gbg.gasBagId == 4));
    REQUIRE(gbg.matId == 26);
    const GasBagVentHoleCard minimal = cardAt<GasBagVentHoleTable>(model, 0);
    REQUIRE((minimal.id == 1 && minimal.size == HoleSize::Small && minimal.flow == FlowDirection::Both));
    REQUIRE((minimal.environmentPressure == 1.0E5 && minimal.environmentDensity == 1.17 &&
             minimal.environmentEnergy == 2.05E5 && !minimal.environmentSpecificHeat));
    REQUIRE(minimal.matId == 0);
    const GasBagVentHoleCard vent = cardAt<GasBagVentHoleTable>(model, 1);
    REQUIRE((vent.id == 301 && vent.size == HoleSize::Large && vent.flow == FlowDirection::In &&
             vent.environmentSpecificHeat == 985.0));
    REQUIRE(vent.matId == 27);

    // The three GBID of the PORFGBG name gas bags, which this deck does not have.
    REQUIRE(unresolvedIn(model) == 3);
    RecordingVisitor visitor;
    model.table<CouplingHoleToCouplingTable>().visitReferences(2, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"couplingId", "matId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Coupling, ReferenceTarget::Material});
    visitor = RecordingVisitor{};
    model.table<CouplingHoleToGasBagTable>().visitReferences(2, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"gasBagId", "matId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::GasBag, ReferenceTarget::Material});
    visitor = RecordingVisitor{};
    model.table<GasBagVentHoleTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId"});
    visitor = RecordingVisitor{};
    model.table<GasBagVentHoleTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());

    Renumbering::apply(model, RenumberSpace::Coupling, Renumbering::offset(100));
    REQUIRE((cardAt<CouplingHoleToCouplingTable>(model, 0).couplingId == 101 &&
             cardAt<CouplingHoleToCouplingTable>(model, 2).couplingId == 104));
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(1000));
    REQUIRE((cardAt<CouplingHoleToCouplingTable>(model, 2).matId == 1025 &&
             cardAt<CouplingHoleToGasBagTable>(model, 2).matId == 1026));
    REQUIRE(cardAt<GasBagVentHoleTable>(model, 1).matId == 1027);
    requireRoundTrip(model);

    expectInvalid("CouplingHoleToCoupling;0;Small;Both;1", "id");
    expectInvalid("CouplingHoleToCoupling;1;HUGE;Both;1", "holeSize");
    expectInvalid("CouplingHoleToCoupling;1;;AROUND;1", "flowDirection");
    expectInvalid("CouplingHoleToCoupling;1", "couplingId");
    expectInvalid("CouplingHoleToCoupling;1;;;0", "couplingId");
    expectInvalid("CouplingHoleToCoupling;1;;;1;-5", "matId");
    expectInvalid("CouplingHoleToGasBag;0;Small;Both;1", "id");
    expectInvalid("CouplingHoleToGasBag;1;HUGE;Both;1", "holeSize");
    expectInvalid("CouplingHoleToGasBag;1;;AROUND;1", "flowDirection");
    expectInvalid("CouplingHoleToGasBag;1", "gasBagId");
    expectInvalid("CouplingHoleToGasBag;1;;;0", "gasBagId");
    expectInvalid("CouplingHoleToGasBag;1;;;1;-5", "matId");
    expectInvalid("GasBagVentHole;0;;;1.E+5;1.;2.E+5", "id");
    expectInvalid("GasBagVentHole;1;TINY;;1.E+5;1.;2.E+5", "holeSize");
    expectInvalid("GasBagVentHole;1;;UP;1.E+5;1.;2.E+5", "flowDirection");
    expectInvalid("GasBagVentHole;1", "environmentPressure");
    expectInvalid("GasBagVentHole;1;;;0.;1.;2.E+5", "environmentPressure");
    expectInvalid("GasBagVentHole;1;;;1.E+5;-1.;2.E+5", "environmentDensity");
    expectInvalid("GasBagVentHole;1;;;1.E+5;1.", "environmentEnergy");
    expectInvalid("GasBagVentHole;1;;;1.E+5;1.;0.", "environmentEnergy");
    expectInvalid("GasBagVentHole;1;;;1.E+5;1.;2.E+5;0.", "environmentSpecificHeat");
    expectInvalid(cardText("GasBagVentHole", {{1, "1"}, {5, "1.E5"}, {6, "1."}, {7, "2.E5"}, {9, "-3"}}), "matId");
}

TEST_CASE("a PORHYDS holds its number only and a PORUDS names a group and a subroutine", "[model][cards][explicit]")
{
    const Model model = readBulk(
        "PorousHydrostaticBoundary;130\nPorousHydrostaticBoundary;131\nPorousUserBoundary;12;mypor;"
        "ExternalPorosity\nPorousUserBoundary;13;GRP\nPorousUserBoundary;14;GRP;MYSUB");
    REQUIRE((cardAt<PorousHydrostaticBoundaryTable>(model, 0).id == 130 &&
             cardAt<PorousHydrostaticBoundaryTable>(model, 1).id == 131));
    const PorousUserBoundaryCard lower = cardAt<PorousUserBoundaryTable>(model, 0);
    REQUIRE((lower.id == 12 && lower.groupName == "mypor" &&
             lower.userRoutineName == "ExternalPorosity")); // exact input spelling
    const PorousUserBoundaryCard standard = cardAt<PorousUserBoundaryTable>(model, 1);
    REQUIRE((standard.groupName == "GRP" && standard.userRoutineName == "ExternalPorosity")); // the default subroutine
    REQUIRE(cardAt<PorousUserBoundaryTable>(model, 2).userRoutineName == "MYSUB");
    requireRoundTrip(model);

    // The porosity cards are numbered per card type: a PORHYDS and a PORHOLE may share a number.
    REQUIRE_NOTHROW(readBulk("PorousHydrostaticBoundary;1\nGasBagVentHole;1;;;1.E+5;1.;2.E+5"));
    REQUIRE_THROWS_AS(readBulk("PorousHydrostaticBoundary;1\nPorousHydrostaticBoundary;1"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("PorousUserBoundary;1;GRP\nPorousUserBoundary;1;OTHER"), base::DeckError);

    expectInvalid("PorousHydrostaticBoundary;0", "id");
    expectInvalid("PorousUserBoundary;0;GRP", "id");
    expectInvalid("PorousUserBoundary;1", "groupName");
    REQUIRE_NOTHROW(readBulk("PorousUserBoundary;1;TOOLONGGROUP"));
    REQUIRE_NOTHROW(readBulk("PorousUserBoundary;1;GRP;TOOLONGNAME"));
}

TEST_CASE("a PORFLOW keeps its pairs of TYPE and VALUE in order and writes them back", "[model][cards][explicit]")
{
    Model model = readBulk(
        "PorousFlowBoundary;130;VelocityX;80.;Material;1\n"
        "PorousFlowBoundary;121;Material;5;VelocityX;10.;VelocityY;-20.;VelocityZ;30.;PressureQuantity;1.E+5;Density;1."
        "2;SpecificInternalEnergy;2.E+5;FlowDirection;In;"
        "Method;PressureQuantity\n"
        "PorousFlowBoundary;122;FlowDirection;;Method\n"
        "IsoMat;1;2.1E+11;;0.3\n"
        "IsoMat;5;2.1E+11;;0.3");
    const PorousFlowBoundaryCard velocity = cardAt<PorousFlowBoundaryTable>(model, 0);
    REQUIRE((velocity.id == 130 && velocity.entries.size() == 2));
    REQUIRE((velocity.entries[0].property == PorousFlowProperty::XVelocity && velocity.entries[0].number == 80.0));
    REQUIRE((velocity.entries[1].property == PorousFlowProperty::Material && velocity.entries[1].material == 1));

    const PorousFlowBoundaryCard all = cardAt<PorousFlowBoundaryTable>(model, 1);
    REQUIRE(all.entries.size() == 9);
    const std::vector<PorousFlowProperty> order{
        PorousFlowProperty::Material,       PorousFlowProperty::XVelocity, PorousFlowProperty::YVelocity,
        PorousFlowProperty::ZVelocity,      PorousFlowProperty::Pressure,  PorousFlowProperty::Density,
        PorousFlowProperty::SpecificEnergy, PorousFlowProperty::Flow,      PorousFlowProperty::Method};
    for (std::size_t index = 0; index < order.size(); ++index)
        REQUIRE(all.entries[index].property == order[index]);
    REQUIRE((all.entries[0].material == 5 && all.entries[1].number == 10.0 && all.entries[2].number == -20.0));
    REQUIRE((all.entries[3].number == 30.0 && all.entries[4].number == 1.0E5 && all.entries[5].number == 1.2));
    REQUIRE((all.entries[6].number == 2.0E5 && all.entries[7].flow == FlowDirection::In));
    REQUIRE(all.entries[8].method == PorousFlowMethod::Pressure);

    // A blank FLOW is BOTH and a blank METHOD is VELOCITY.
    const PorousFlowBoundaryCard defaults = cardAt<PorousFlowBoundaryTable>(model, 2);
    REQUIRE((defaults.entries.size() == 2 && defaults.entries[0].flow == FlowDirection::Both));
    REQUIRE(defaults.entries[1].method == PorousFlowMethod::Velocity);
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<PorousFlowBoundaryTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propertyValue2"}); // the MATERIAL is the second pair
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material});
    visitor = RecordingVisitor{};
    model.table<PorousFlowBoundaryTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propertyValue1"});
    visitor = RecordingVisitor{};
    model.table<PorousFlowBoundaryTable>().visitReferences(2, visitor);
    REQUIRE(visitor.fields.empty());

    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(100));
    REQUIRE((cardAt<PorousFlowBoundaryTable>(model, 0).entries[1].material == 101 &&
             cardAt<PorousFlowBoundaryTable>(model, 1).entries[0].material == 105));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    expectInvalid("PorousFlowBoundary;1", "propertyKind1");
    expectInvalid("PorousFlowBoundary;0;VelocityX;1.", "id");
    expectInvalid("PorousFlowBoundary;1;SPEED;1.", "propertyKind1");
    expectInvalid("PorousFlowBoundary;1;;5.", "propertyKind1");
    expectInvalid("PorousFlowBoundary;1;VelocityX", "propertyValue1");
    expectInvalid("PorousFlowBoundary;1;VelocityX;FAST", "propertyValue1");
    expectInvalid("PorousFlowBoundary;1;Material;1.5", "propertyValue1");
    expectInvalid("PorousFlowBoundary;1;Material;0", "propertyValue1");
    expectInvalid("PorousFlowBoundary;1;FlowDirection;UP", "propertyValue1");
    expectInvalid("PorousFlowBoundary;1;Method;SLOW", "propertyValue1");
    expectInvalid("PorousFlowBoundary;1;VelocityX;1.;VelocityY;2.;PressureQuantity", "propertyValue3");
    expectInvalid("PorousFlowBoundary;1;VelocityX;1.;VelocityX;2.", "propertyKind2"); // a property is given once
    expectInvalid(
        "PorousFlowBoundary;1;Material;1;VelocityX;1.;VelocityY;1.;VelocityZ;1.;PressureQuantity;1.;Density;1.;"
        "SpecificInternalEnergy;1.;FlowDirection;In;Method;PressureQuantity;"
        "VelocityX;2.",
        ""); // a tenth pair
}

TEST_CASE("a PORFLWT has quantities that are the element value, a number or a table, and a MID with conditions",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "PorousTimeFlowBoundary;1\n"
        "PorousTimeFlowBoundary;2;In;Table;101;Table;102;91;Table;105;Table;107\n"
        "PorousTimeFlowBoundary;3;Out;Constant;-5.\n"
        "PorousTimeFlowBoundary;4;Both;Constant;5.;Constant;1.E+5;92;Constant;1.2;Constant;2.E+5\n"
        "PorousTimeFlowBoundary;5;In;Constant;2.;;;94;Element;;Element\n"
        "TableDynamic1;101;;;0.;1.;1.;1.;EndTable\n"
        "TableDynamic1;102;;;0.;1.;1.;1.;EndTable\n"
        "TableDynamic1;105;;;0.;1.;1.;1.;EndTable\n"
        "TableDynamic1;107;;;0.;1.;1.;1.;EndTable\n"
        "IsoMat;91;2.1E+11;;0.3\n"
        "IsoMat;92;2.1E+11;;0.3\n"
        "IsoMat;94;2.1E+11;;0.3");
    const PorousTimeFlowBoundaryCard bare = cardAt<PorousTimeFlowBoundaryTable>(model, 0);
    REQUIRE((bare.id == 1 && bare.type == FlowDirection::Both && bare.matId == 0)); // the defaults
    REQUIRE((bare.velocity.source == ValueSource::Element && bare.pressure.source == ValueSource::Element));
    REQUIRE((bare.density.source == ValueSource::Element && bare.energy.source == ValueSource::Element));

    const PorousTimeFlowBoundaryCard tables = cardAt<PorousTimeFlowBoundaryTable>(model, 1);
    REQUIRE((tables.type == FlowDirection::In && tables.matId == 91));
    REQUIRE((tables.velocity.source == ValueSource::Table && tables.velocity.table == 101));
    REQUIRE((tables.pressure.source == ValueSource::Table && tables.pressure.table == 102));
    REQUIRE((tables.density.source == ValueSource::Table && tables.density.table == 105));
    REQUIRE((tables.energy.source == ValueSource::Table && tables.energy.table == 107));
    const PorousTimeFlowBoundaryCard outflow = cardAt<PorousTimeFlowBoundaryTable>(model, 2);
    REQUIRE((outflow.type == FlowDirection::Out && outflow.velocity.source == ValueSource::Constant));
    REQUIRE(outflow.velocity.constant == -5.0);
    const PorousTimeFlowBoundaryCard constants = cardAt<PorousTimeFlowBoundaryTable>(model, 3);
    REQUIRE((constants.velocity.constant == 5.0 && constants.pressure.constant == 1.0E5 && constants.matId == 92));
    REQUIRE((constants.density.constant == 1.2 && constants.energy.constant == 2.0E5));
    const PorousTimeFlowBoundaryCard element = cardAt<PorousTimeFlowBoundaryTable>(model, 4);
    REQUIRE((element.matId == 94 && element.density.source == ValueSource::Element));
    REQUIRE(element.energy.source == ValueSource::Element);
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<PorousTimeFlowBoundaryTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"velocity", "pressure", "matId", "density", "specificEnergy"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::DynamicTable,
                                                            ReferenceTarget::DynamicTable, ReferenceTarget::Material,
                                                            ReferenceTarget::DynamicTable,
                                                            ReferenceTarget::DynamicTable});
    visitor = RecordingVisitor{};
    model.table<PorousTimeFlowBoundaryTable>().visitReferences(3, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId"}); // the constants visit no table

    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(1000));
    REQUIRE((cardAt<PorousTimeFlowBoundaryTable>(model, 1).velocity.table == 1101 &&
             cardAt<PorousTimeFlowBoundaryTable>(model, 1).energy.table == 1107));
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(10));
    REQUIRE((cardAt<PorousTimeFlowBoundaryTable>(model, 1).matId == 101 &&
             cardAt<PorousTimeFlowBoundaryTable>(model, 4).matId == 104));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    expectInvalid("PorousTimeFlowBoundary;0", "id");
    expectInvalid("PorousTimeFlowBoundary;1;SIDEWAYS", "flowDirection");
    expectInvalid(cardText("PorousTimeFlowBoundary", {{1, "1"}, {9, "SPEED"}, {10, "1."}}), "velocitySource");
    expectInvalid(cardText("PorousTimeFlowBoundary", {{1, "1"}, {9, "Element"}, {10, "1."}}), "velocity");
    expectInvalid(cardText("PorousTimeFlowBoundary", {{1, "1"}, {10, "1."}}), "velocity"); // a blank VELTYPE is ELEMENT
    expectInvalid(cardText("PorousTimeFlowBoundary", {{1, "1"}, {9, "Table"}}), "velocity");
    expectInvalid(cardText("PorousTimeFlowBoundary", {{1, "1"}, {9, "Table"}, {10, "0"}}), "velocity");
    expectInvalid(cardText("PorousTimeFlowBoundary", {{1, "1"}, {9, "Table"}, {10, "1.5"}}), "velocity");
    expectInvalid(cardText("PorousTimeFlowBoundary", {{1, "1"}, {11, "SPEED"}, {12, "1."}}), "pressureSource");
    expectInvalid(cardText("PorousTimeFlowBoundary", {{1, "1"}, {11, "Constant"}, {12, "x"}}), "pressure");
    // An outflow takes no inflowing material (MID).
    expectInvalid(
        cardText("PorousTimeFlowBoundary",
                 {{1, "1"}, {2, "Out"}, {17, "5"}, {18, "Constant"}, {19, "1."}, {20, "Constant"}, {21, "1."}}),
        "matId");
    expectInvalid(cardText("PorousTimeFlowBoundary", {{1, "1"}, {17, "-5"}}), "matId");
    // With a MID, the density and the energy of the material must be given.
    expectInvalid(cardText("PorousTimeFlowBoundary", {{1, "1"}, {17, "5"}}), "densitySource");
    expectInvalid(cardText("PorousTimeFlowBoundary", {{1, "1"}, {17, "5"}, {18, "Constant"}}), "density");
    expectInvalid(cardText("PorousTimeFlowBoundary", {{1, "1"}, {17, "5"}, {18, "Constant"}, {19, "1."}}),
                  "energySource");
    expectInvalid(cardText("PorousTimeFlowBoundary", {{1, "1"}, {17, "5"}, {18, "Element"}, {20, "Constant"}}),
                  "specificEnergy");
}
