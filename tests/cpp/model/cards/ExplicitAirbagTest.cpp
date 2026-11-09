// Checks of the airbag cards of the explicit solver (SOL 700): the gas bag GBAG, its inflators (ABINFL, INFLTR, INFLHB,
// INFLCG, INFLTNK), the gases (INFLGAS, INFLFRC, INITGAS) and the belt property PBELT. Each card has its fields in the
// positions of the format reference, its defaults, the references it lists, its refused values and the round trip
// through a model file; one deck with every card checks that the references resolve and follow a renumbering.

#include "oor/model/cards/explicit/beltProp/BeltPropTable.hpp"
#include "oor/model/cards/explicit/coldGasInflator/ColdGasInflatorTable.hpp"
#include "oor/model/cards/explicit/gasBag/GasBagTable.hpp"
#include "oor/model/cards/explicit/hybridInflator/HybridInflatorTable.hpp"
#include "oor/model/cards/explicit/hybridInflatorFractions/HybridInflatorFractionsTable.hpp"
#include "oor/model/cards/explicit/inflatorGas/InflatorGasTable.hpp"
#include "oor/model/cards/explicit/initialGasComposition/InitialGasCompositionTable.hpp"
#include "oor/model/cards/explicit/standardInflator/StandardInflatorTable.hpp"
#include "oor/model/cards/explicit/tankTestInflator/TankTestInflatorTable.hpp"
#include "oor/model/cards/loads/inflatorInlet/InflatorInletTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The cards of the group and the cards they name (a material and its equation of state, surfaces made of one element,
// tables), numbered so that every reference resolves: the gas bag 101 takes the inflator set 12 (four ABINFL cards, one
// for each kind of inflator), the initial gas 14 and the surface 37.
const char* const AIRBAG_DECK =
    "GammaLawStateEquation;1\n"
    "EulerMat;25;1.E+3;1\n"
    "Node;1\n"
    "Node;2;;1.\n"
    "Node;3;;1.;1.\n"
    "Node;4;;0.;1.\n"
    "ShellQuad4;1;1;1;2;3;4\n"
    "ShellProp;1;1;0.1\n"
    "IsoMat;1;2.1E+11;;0.3\n"
    "ContactElemSurface;37;1\n"
    "ContactElemSurface;120;1\n"
    "ContactElemSurface;121;1\n"
    "ContactElemSurface;122;1\n"
    "ContactElemSurface;123;1\n"
    "TableDynamic1;10;;;0.;1.;1.;1.;EndTable\n"
    "TableDynamic1;11;;;0.;1.;1.;1.;EndTable\n"
    "TableDynamic1;12;;;0.;1.;1.;1.;EndTable\n"
    "GasBag;101;37;;;;12;;14;;1.E+3;297.;101325.;;;;293.\n"
    "InflatorInlet;201;12;120;StandardInflator;5;Table;10\n"
    "InflatorInlet;202;12;121;HybridInflator;21;;0.5\n"
    "InflatorInlet;203;12;122;TankTestInflator;111\n"
    "InflatorInlet;204;12;123;ColdGasInflator;11\n"
    "StandardInflator;5;10;11;;12;;;;;25\n"
    "HybridInflator;21;10;;650.;31;12;13\n"
    "TankTestInflator;111;AverageTemperatureMethod;10;0.15;0.01;0.;;;;1.4;;281.\n"
    "ColdGasInflator;11;0.82;1.25E+5;293.;1.37;12\n"
    "InflatorGas;12;ResponseSpectrum;208.;Table;12\n"
    "InflatorGas;13;;0.029;Constant;910.\n"
    "HybridInflatorFractions;31;;TimeStep;0.;2;0.4;0.6\n"
    "InitialGasComposition;14;12;0.4;13;0.6\n"
    "BeltProp;6402;10;11;3.E-5;0.1;0.1;12";

// The fields of a GBAG after its first nine: the required values of the third part.
const char* const GAS_BAG_REST = ";1.E+3;297.;101325.;;;;293.";

} // namespace

TEST_CASE("a GBAG has its fields in the five lines of the format reference and resolves PINIT and TENV",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "GasBag;101;37;;;11;12;13;14;Constant;1.E+3;297.;101325.;Off;Off;;293.\n"
        "GasBag;102;38;Time;0.5;;;;;;1100.;300.;1.E+5;;;9.5E+4;290.;285.\n"
        "GasBag;103;39;;;;;;;;1.E+3;297.;101325.;;;;293.");
    const GasBagCard full = cardAt<GasBagTable>(model, 0);
    REQUIRE((full.id == 101 && full.surfaceId == 37 && full.porosityId == 11 && full.inflatorId == 12 &&
             full.heatTransferId == 13 && full.initialGasId == 14));
    REQUIRE((full.specificHeat.source == ValueSource::Constant && full.specificHeat.constant == 1000.0));
    REQUIRE((full.gasConstant == 297.0 && full.environmentPressure == 101325.0 && full.initialTemperature == 293.0));
    REQUIRE((full.reverse == Switch::Off && full.check == Switch::Off));
    REQUIRE((full.initialPressure == 101325.0 && full.environmentTemperature == 293.0)); // blank: PENV and TINIT

    const GasBagCard offset = cardAt<GasBagTable>(model, 1);
    REQUIRE((offset.trigger == GasBagTrigger::Time && offset.triggerOffset == 0.5));
    REQUIRE((offset.initialPressure == 95000.0 && offset.environmentTemperature == 285.0 &&
             offset.environmentPressure == 100000.0)); // given: kept
    const GasBagCard bare = cardAt<GasBagTable>(model, 2);
    REQUIRE((bare.triggerOffset == 0.0 && bare.porosityId == 0 && bare.inflatorId == 0 && bare.heatTransferId == 0 &&
             bare.initialGasId == 0));
    REQUIRE((bare.reverse == Switch::On && bare.check == Switch::On));
    REQUIRE(
        (bare.initialPressure == bare.environmentPressure && bare.environmentTemperature == bare.initialTemperature));

    RecordingVisitor visitor;
    model.table<GasBagTable>().visitReferences(0, visitor); // the HTRID names a set of HEATLOS cards
    REQUIRE(visitor.fields ==
            std::vector<std::string>{"surfaceId", "porosityId", "inflatorId", "heatTransferId", "initialGasId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::ContactSurface, ReferenceTarget::Leakage,
                                                            ReferenceTarget::InflatorSet, ReferenceTarget::HeatLossSet,
                                                            ReferenceTarget::InitialGas});
    visitor = RecordingVisitor{};
    model.table<GasBagTable>().visitReferences(2, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"surfaceId"});
    requireRoundTrip(model);

    REQUIRE_THROWS_AS(
        readBulk(std::string("GasBag;1;37;;;;;;;") + GAS_BAG_REST + "\nGasBag;1;38;;;;;;;" + GAS_BAG_REST),
        base::DeckError);

    expectInvalid(std::string("GasBag;0;37;;;;;;;") + GAS_BAG_REST, "id");
    expectInvalid(std::string("GasBag;1;0;;;;;;;") + GAS_BAG_REST, "surfaceId");
    expectInvalid(std::string("GasBag;1;37;SPACE;;;;;;") + GAS_BAG_REST, "trigger");
    expectInvalid(std::string("GasBag;1;37;;;-5;;;;") + GAS_BAG_REST, "porosityId");
    expectInvalid("GasBag;1;37;;;;;;;Table;5;297.;101325.;;;;293.", "specificHeatSource");
    expectInvalid("GasBag;1;37;;;;;;;;;297.;101325.;;;;293.", "specificHeatPressure");
    expectInvalid("GasBag;1;37;;;;;;;;1.E+3;;101325.;;;;293.", "gasConstant");
    expectInvalid("GasBag;1;37;;;;;;;;1.E+3;-297.;101325.;;;;293.", "gasConstant");
    expectInvalid("GasBag;1;37;;;;;;;;1.E+3;297.;101325.;MAYBE;;;293.", "reverseNormals");
    expectInvalid("GasBag;1;37;;;;;;;;1.E+3;297.;101325.;;;;0.", "initialTemperature");
    expectInvalid("GasBag;1;37;;;;;;;;1.E+3;297.;101325.;;;;293.;-1.", "environmentTemperature");
}

TEST_CASE("an ABINFL is filed under the set ID of its INFID, and its coefficient is a number or a table",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "InflatorInlet;201;1;120;HybridInflator;11;;0.015\n"
        "InflatorInlet;202;1;121;StandardInflator;5\n"
        "InflatorInlet;203;2;122;ColdGasInflator;12;Table;40\n"
        "InflatorInlet;204;2;123;TankTestInflator;111;Constant;0.\n"
        "ContactElemSurface;120;1\n"
        "ContactElemSurface;121;1\n"
        "ContactElemSurface;122;1\n"
        "ContactElemSurface;123;1\n"
        "TableDynamic1;40;;;0.;1.;1.;1.;EndTable\n"
        "Node;1\n"
        "Node;2;;1.\n"
        "Node;3;;1.;1.\n"
        "Node;4;;0.;1.\n"
        "ShellQuad4;1;1;1;2;3;4\n"
        "ShellProp;1;1;0.1\n"
        "IsoMat;1;2.1E+11;;0.3");
    const InflatorInletCard hybrid = cardAt<InflatorInletTable>(model, 0);
    REQUIRE((hybrid.id == 201 && hybrid.inflatorId == 1 && hybrid.subsurfaceId == 120 &&
             hybrid.inflatorKind == InflatorInletInflator::Hybrid));
    REQUIRE((hybrid.inflatorKindId == 11 && hybrid.coeff.source == ValueSource::Constant &&
             hybrid.coeff.constant == 0.015));
    const InflatorInletCard open = cardAt<InflatorInletTable>(model, 1);
    REQUIRE((open.inflatorKind == InflatorInletInflator::Standard &&
             open.coeff.constant == 1.0)); // a blank value: the whole area
    const InflatorInletCard varying = cardAt<InflatorInletTable>(model, 2);
    REQUIRE((varying.inflatorKind == InflatorInletInflator::ColdGas && varying.coeff.source == ValueSource::Table));
    REQUIRE(varying.coeff.table == 40);
    const InflatorInletCard closed = cardAt<InflatorInletTable>(model, 3);
    REQUIRE(
        (closed.inflatorKind == InflatorInletInflator::TankTest && closed.coeff.constant == 0.0)); // a closed inflator

    // The cards of a set share the INFID, which is the ID of the table; the CID is a number of the card only.
    REQUIRE(rowCount<InflatorInletTable>(model) == 4);
    REQUIRE(model.table<InflatorInletTable>().idAtRow(0) == model.table<InflatorInletTable>().idAtRow(1));
    REQUIRE(model.table<InflatorInletTable>().idAtRow(2) == 2);
    REQUIRE(unresolvedIn(model) == 4); // the four inflator cards are not in the deck

    RecordingVisitor visitor;
    model.table<InflatorInletTable>().visitReferences(2, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"subsurfaceId", "inflatorKindId", "areaCoefficient"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::ContactSurface, ReferenceTarget::Inflator,
                                                            ReferenceTarget::DynamicTable});
    visitor = RecordingVisitor{};
    model.table<InflatorInletTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"subsurfaceId", "inflatorKindId"}); // a constant visits no table

    Renumbering::apply(model, RenumberSpace::InflatorSet, Renumbering::offset(100));
    REQUIRE((cardAt<InflatorInletTable>(model, 0).inflatorId == 101 &&
             cardAt<InflatorInletTable>(model, 2).inflatorId == 102));
    Renumbering::apply(model, RenumberSpace::ContactSurface, Renumbering::offset(1000));
    REQUIRE(cardAt<InflatorInletTable>(model, 1).subsurfaceId == 1121);
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(500));
    REQUIRE(cardAt<InflatorInletTable>(model, 2).coeff.table == 540);
    requireRoundTrip(model);

    expectInvalid("InflatorInlet;0;1;120;HybridInflator;11", "id");
    expectInvalid("InflatorInlet;201;0;120;HybridInflator;11", "inflatorId");
    expectInvalid("InflatorInlet;201;1;0;HybridInflator;11", "subsurfaceId");
    expectInvalid("InflatorInlet;201;1;120;INFLXX;11", "inflatorKind");
    expectInvalid("InflatorInlet;201;1;120;;11", "inflatorKind"); // the kind is required
    expectInvalid("InflatorInlet;201;1;120;HybridInflator;0", "inflatorKindId");
    expectInvalid("InflatorInlet;201;1;120;HybridInflator;11;;1.5",
                  "areaCoefficient"); // an area coefficient is at most 1
    expectInvalid("InflatorInlet;201;1;120;HybridInflator;11;;-0.1", "areaCoefficient");
    expectInvalid("InflatorInlet;201;1;120;HybridInflator;11;Table", "areaCoefficient");
    expectInvalid("InflatorInlet;201;1;120;HybridInflator;11;Table;0", "areaCoefficient");
    expectInvalid("InflatorInlet;201;1;120;HybridInflator;11;Element", "areaCoefficientSource");
}

TEST_CASE("the airbag cards of a deck resolve their references and follow a renumbering", "[model][cards][explicit]")
{
    Model model = readBulk(AIRBAG_DECK);
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(rowCount<InflatorInletTable>(model) == 4);

    // The gases: an INFLTR, an INFLHB, an INFLCG, an INFLGAS table and an INITGAS follow the INFLGAS numbers.
    Renumbering::apply(model, RenumberSpace::InflatorGas, Renumbering::offset(100));
    REQUIRE((cardAt<InflatorGasTable>(model, 0).id == 112 && cardAt<InflatorGasTable>(model, 1).id == 113));
    REQUIRE(cardAt<StandardInflatorTable>(model).gasName.gas == 112);
    REQUIRE(cardAt<ColdGasInflatorTable>(model).gasName.gas == 112);
    REQUIRE((cardAt<HybridInflatorTable>(model).gasId == std::vector<base::EntityId>{112, 113}));
    REQUIRE((cardAt<InitialGasCompositionTable>(model).gases[0].gasId == 112 &&
             cardAt<InitialGasCompositionTable>(model).gases[1].gasId == 113));
    REQUIRE(unresolvedIn(model) == 0);

    // The four kinds of inflator share one number space of references: an ABINFL names any of them.
    Renumbering::apply(model, RenumberSpace::Inflator, Renumbering::offset(1000));
    REQUIRE((cardAt<StandardInflatorTable>(model).id == 1005 && cardAt<HybridInflatorTable>(model).id == 1021));
    REQUIRE((cardAt<TankTestInflatorTable>(model).id == 1111 && cardAt<ColdGasInflatorTable>(model).id == 1011));
    REQUIRE((cardAt<InflatorInletTable>(model, 0).inflatorKindId == 1005 &&
             cardAt<InflatorInletTable>(model, 1).inflatorKindId == 1021));
    REQUIRE((cardAt<InflatorInletTable>(model, 2).inflatorKindId == 1111 &&
             cardAt<InflatorInletTable>(model, 3).inflatorKindId == 1011));
    REQUIRE(unresolvedIn(model) == 0);

    Renumbering::apply(model, RenumberSpace::InflatorSet, Renumbering::offset(100));
    REQUIRE((cardAt<InflatorInletTable>(model, 0).inflatorId == 112 && cardAt<GasBagTable>(model).inflatorId == 112));
    Renumbering::apply(model, RenumberSpace::InitialGas, Renumbering::offset(100));
    REQUIRE((cardAt<InitialGasCompositionTable>(model).id == 114 && cardAt<GasBagTable>(model).initialGasId == 114));
    Renumbering::apply(model, RenumberSpace::GasFractions, Renumbering::offset(100));
    REQUIRE((cardAt<HybridInflatorFractionsTable>(model).id == 131 &&
             cardAt<HybridInflatorTable>(model).fractionId == 131));
    Renumbering::apply(model, RenumberSpace::ContactSurface, Renumbering::offset(1000));
    REQUIRE(
        (cardAt<GasBagTable>(model).surfaceId == 1037 && cardAt<InflatorInletTable>(model, 0).subsurfaceId == 1120));
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(1000));
    REQUIRE((cardAt<StandardInflatorTable>(model).massFlowTableId == 1010 &&
             cardAt<HybridInflatorTable>(model).massFlowTableId == 1010));
    REQUIRE((cardAt<TankTestInflatorTable>(model).tankPressureTableId == 1010 &&
             cardAt<InflatorGasTable>(model, 0).heatTableId == 1012));
    REQUIRE((cardAt<InflatorInletTable>(model, 0).coeff.table == 1010 &&
             cardAt<BeltPropTable>(model).slackTableId == 1012));
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(1000));
    REQUIRE(cardAt<StandardInflatorTable>(model).matId == 1025);
    Renumbering::apply(model, RenumberSpace::GasBag, Renumbering::offset(100));
    REQUIRE(cardAt<GasBagTable>(model).id == 201);
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(5));
    REQUIRE(cardAt<BeltPropTable>(model).id == 6407);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
}
