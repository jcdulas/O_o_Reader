// The axisymmetric cards: the conical shell cards (RINGAX, POINTAX, AXIC, MOMAX, PRESAX, OMITAX), the
// axisymmetric fluid cards (RINGFL, FREEPT, PRESPT, AXIF) and the slot cards (GRIDF, GRIDS, GRIDA, AXSLOT, SLBDY), with
// SNORM, the surface normal of a grid point. Per card: the fields in their format reference positions, the defaults,
// the forms the format draws, the references the card lists and the numbers it owns, the refused values and the round
// trip through a model file. The generic tests of CardRoundTripTest.cpp read, write and copy every sample of the
// group.

#include "oor/model/cards/loads/axisymConePressureLoad/AxisymConePressureLoadTable.hpp"
#include "oor/model/cards/loads/axisymRingMomentLoad/AxisymRingMomentLoadTable.hpp"
#include "oor/model/cards/nodes/acousticSlotControl/AcousticSlotControlTable.hpp"
#include "oor/model/cards/nodes/acousticSlotFluidPoint/AcousticSlotFluidPointTable.hpp"
#include "oor/model/cards/nodes/acousticSlotSurfacePoint/AcousticSlotSurfacePointTable.hpp"
#include "oor/model/cards/nodes/associativeNode/AssociativeNodeTable.hpp"
#include "oor/model/cards/nodes/axisymConeControl/AxisymConeControlTable.hpp"
#include "oor/model/cards/nodes/axisymConePoint/AxisymConePointTable.hpp"
#include "oor/model/cards/nodes/axisymConeRing/AxisymConeRingTable.hpp"
#include "oor/model/cards/nodes/axisymFluidControl/AxisymFluidControlTable.hpp"
#include "oor/model/cards/nodes/axisymFluidFreePoint/AxisymFluidFreePointTable.hpp"
#include "oor/model/cards/nodes/axisymFluidPressurePoint/AxisymFluidPressurePointTable.hpp"
#include "oor/model/cards/nodes/axisymFluidRing/AxisymFluidRingTable.hpp"
#include "oor/model/cards/nodes/shellNormal/ShellNormalTable.hpp"
#include "oor/model/cards/sets/axisymOmittedDofSet/AxisymOmittedDofSetTable.hpp"
#include "oor/model/cards/thermal/acousticSlotBoundary/AcousticSlotBoundaryTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// How many times `part` is in `text`.

// An OMITAX record of `count` degrees of freedom of the ring 81001.
std::string omitax(int count)
{
    std::string text = "AxisymOmittedDofSet";
    for (int index = 0; index < count; ++index)
        text += ";81001;0;1";
    return text;
}

// The model written in free format.

} // namespace

// ---------------------------------------------------------------------------------------------------------------------
// The points of the conical shell model: RINGAX and POINTAX
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a RINGAX reads the layout of the format, keeps its constraints sorted and shares the node ID space",
          "[model][cards][axisymmetric]")
{
    Model model =
        readBulk("AxisymConeRing;81001;2.5;-12.5;251\nAxisymConeRing;81002;5.;0.\nAxisymConeRing;81003;1.5;3.5;321");
    const AxisymConeRingCard example = cardAt<AxisymConeRingTable>(model, 0);
    REQUIRE((example.id == 81001 && example.radius == 2.5 && example.zCoord == -12.5));
    REQUIRE(example.permanentConstraints == 125); // the digits of 251, in ascending order
    REQUIRE(cardAt<AxisymConeRingTable>(model, 1).permanentConstraints == 0);
    REQUIRE(cardAt<AxisymConeRingTable>(model, 2).permanentConstraints == 123);
    REQUIRE((AxisymConeRingTable::INFO.category == CardCategory::Point &&
             AxisymConeRingTable::INFO.idSpace == base::IdSpace::Node));
    REQUIRE(unresolvedIn(model) == 0); // a RINGAX names no other card
    requireRoundTrip(model);

    // The number is one of the node ID space: a GRID, a POINTAX or another RINGAX cannot have it.
    REQUIRE_THROWS_AS(readBulk("Node;5\nAxisymConeRing;5;1.;1."), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("AxisymConeRing;5;1.;1.\nAxisymConePoint;5;5;0."), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("AxisymConeRing;5;1.;1.\nAxisymConeRing;5;2.;2."), base::DeckError);

    // A renumbering of the nodes moves the rings with the grid points.
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE(cardAt<AxisymConeRingTable>(model, 0).id == 82001);
    REQUIRE(cardAt<AxisymConeRingTable>(model, 2).id == 82003);

    expectInvalid("AxisymConeRing;0;2.5;-12.5", "id");
    expectInvalid("AxisymConeRing;-3;2.5;-12.5", "id");
    expectInvalid("AxisymConeRing;5;0.;1.", "radius");
    expectInvalid("AxisymConeRing;5;-1.;1.", "radius");
    expectInvalid("AxisymConeRing;5;2.", "zCoord");
    expectInvalid("AxisymConeRing;5", "radius");
    expectInvalid("AxisymConeRing;5;2.;1.;7", "permanentConstraints");
    expectInvalid("AxisymConeRing;5;2.;1.;12X", "permanentConstraints");
}

TEST_CASE("a POINTAX names a ring, is a point of the node ID space and follows its renumbering",
          "[model][cards][axisymmetric]")
{
    Model model = readBulk(
        "AxisymConeRing;81001;2.;1.\nAxisymConeRing;81002;3.;1.\nAxisymConePoint;81401;81002;30.\nAxisymConePoint;"
        "81402;81001;-45.");
    const AxisymConePointCard card = cardAt<AxisymConePointTable>(model, 0);
    REQUIRE((card.id == 81401 && card.ringId == 81002 && card.phiAngle == 30.0));
    REQUIRE(cardAt<AxisymConePointTable>(model, 1).phiAngle == -45.0);
    REQUIRE((AxisymConePointTable::INFO.category == CardCategory::Point &&
             AxisymConePointTable::INFO.idSpace == base::IdSpace::Node));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    RecordingVisitor visitor;
    model.table<AxisymConePointTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"ringId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Node});

    // A ring that no card defines is an unresolved reference.
    Model missing = readBulk("AxisymConeRing;81001;2.;1.\nAxisymConePoint;81401;81009;30.");
    REQUIRE(unresolvedIn(missing) == 1);

    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE(
        (cardAt<AxisymConePointTable>(model, 0).id == 82401 && cardAt<AxisymConePointTable>(model, 0).ringId == 82002));
    REQUIRE(unresolvedIn(model) == 0);

    expectInvalid("AxisymConePoint;81401;81001", "phiAngle");
    expectInvalid("AxisymConePoint;81401;0;3.", "ringId");
    expectInvalid("AxisymConePoint;0;81001;3.", "id");
    expectInvalid("AxisymConePoint;81401;81001;x", "phiAngle");
}

// ---------------------------------------------------------------------------------------------------------------------
// AXIC, AXIF and AXSLOT: one of each in a deck
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an AXIC has the highest harmonic and is the only one of a deck", "[model][cards][axisymmetric]")
{
    REQUIRE(cardAt<AxisymConeControlTable>(readBulk("AxisymConeControl;15")).highestHarmonic == 15);
    REQUIRE(cardAt<AxisymConeControlTable>(readBulk("AxisymConeControl;0")).highestHarmonic == 0);
    REQUIRE(cardAt<AxisymConeControlTable>(readBulk("AxisymConeControl;997")).highestHarmonic == 997);
    REQUIRE((AxisymConeControlTable::INFO.category == CardCategory::Other &&
             AxisymConeControlTable::INFO.idSpace == base::IdSpace::None));
    requireRoundTrip(readBulk("AxisymConeControl;15"));

    const Model twice = readModelText("[model]\nAxisymConeControl;3\nAxisymConeControl;4\n", false);
    REQUIRE(twice.diagnostics().firstError()->code == "deck.duplicate-axisym-cone-control");

    expectInvalid("AxisymConeControl", "highestHarmonic");
    expectInvalid("AxisymConeControl;998", "highestHarmonic");
    expectInvalid("AxisymConeControl;-1", "highestHarmonic");
    expectInvalid("AxisymConeControl;1.5", "highestHarmonic");
}

TEST_CASE("an AXIF reads its parameters and its harmonics in every form of the format", "[model][cards][axisymmetric]")
{
    Model model = readBulk(
        "CylindricalFrame2;2;;0.;0.;0.;0.;0.;1.;1.;0.;0.\nAxisymFluidControl;2;9.75;0.0375;1.25E+5;Yes;;1;2;3;;4;;6;"
        "11");
    const AxisymFluidControlCard card = cardAt<AxisymFluidControlTable>(model);
    REQUIRE((card.frameId == 2 && card.gravity == 9.75 && card.defaultDensity == 0.0375 &&
             card.defaultBulkModulus == 1.25e5));
    REQUIRE(card.nonsymmetric == AxisymFluidControlSymmetry::Yes);
    REQUIRE(card.harmonics == std::vector<AxisymFluidControlHarmonics>{
                                  AxisymFluidControlHarmonics::single(1), AxisymFluidControlHarmonics::single(2),
                                  AxisymFluidControlHarmonics::single(3), AxisymFluidControlHarmonics::single(4),
                                  AxisymFluidControlHarmonics::single(6), AxisymFluidControlHarmonics::single(11)});
    REQUIRE((AxisymFluidControlTable::INFO.category == CardCategory::Other &&
             AxisymFluidControlTable::INFO.idSpace == base::IdSpace::None));
    requireRoundTrip(model);

    // The system is the only reference, and a renumbering of the coordinate systems follows it.
    RecordingVisitor visitor;
    model.table<AxisymFluidControlTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"frameId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Coordinate});
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    REQUIRE(cardAt<AxisymFluidControlTable>(model).frameId == 12);
    Model missing = readBulk("AxisymFluidControl;9;1.;;;No;None");
    REQUIRE(unresolvedIn(missing) == 1);
}

TEST_CASE("an AXIF keeps its ranges and steps as written and never splits one between two lines",
          "[model][cards][axisymmetric]")
{
    // A blank DRHO, a range with a step, singles, ranges and fields left blank between numbers, over six lines.
    const Model ranges = readBulk(
        "AxisymFluidControl;12;-385.5;;0.;No;;0;Through;40;Step;4;;;;43;;;;;;;;45;Through;48;;;;;;53;Through;58;;;;;;"
        "60;;63;;66;"
        "70;;;84;96");
    const AxisymFluidControlCard card = cardAt<AxisymFluidControlTable>(ranges);
    REQUIRE((card.frameId == 12 && card.gravity == -385.5 && !card.defaultDensity.has_value() &&
             card.defaultBulkModulus == 0.0));
    REQUIRE(card.nonsymmetric == AxisymFluidControlSymmetry::No);
    REQUIRE(card.harmonics == std::vector<AxisymFluidControlHarmonics>{
                                  AxisymFluidControlHarmonics::through(0, 40, 4),
                                  AxisymFluidControlHarmonics::single(43), AxisymFluidControlHarmonics::through(45, 48),
                                  AxisymFluidControlHarmonics::through(53, 58), AxisymFluidControlHarmonics::single(60),
                                  AxisymFluidControlHarmonics::single(63), AxisymFluidControlHarmonics::single(66),
                                  AxisymFluidControlHarmonics::single(70), AxisymFluidControlHarmonics::single(84),
                                  AxisymFluidControlHarmonics::single(96)});
    requireRoundTrip(ranges);

    // A range alone, without and with a step.
    REQUIRE(cardAt<AxisymFluidControlTable>(readBulk("AxisymFluidControl;2;9.75;0.0375;;No;;0;Through;12")).harmonics ==
            std::vector<AxisymFluidControlHarmonics>{AxisymFluidControlHarmonics::through(0, 12)});
    REQUIRE(cardAt<AxisymFluidControlTable>(readBulk("AxisymFluidControl;2;9.75;0.0375;;No;;0;Through;12;Step;4"))
                .harmonics == std::vector<AxisymFluidControlHarmonics>{AxisymFluidControlHarmonics::through(0, 12, 4)});
    // THRU and STEP are read in any case.
    REQUIRE(cardAt<AxisymFluidControlTable>(readBulk("AxisymFluidControl;2;9.75;0.0375;;No;;0;Through;12;Step;4"))
                .harmonics == std::vector<AxisymFluidControlHarmonics>{AxisymFluidControlHarmonics::through(0, 12, 4)});

    // Seven numbers fill the line up to field 15: a range of three fields does not fit in what is left and starts the
    // next line (the free format writes the numbers of a line without its blank end).
    const Model full = readBulk("AxisymFluidControl;2;1.;;;Yes;;8;9;10;11;12;13;14;;15;Through;20");
    requireRoundTrip(full);
}

TEST_CASE("an AXIF with F = NONE has no harmonics and the other cases refuse what the format forbids",
          "[model][cards][axisymmetric]")
{
    const Model none = readBulk("AxisymFluidControl;3;9.81;;;No;None");
    const AxisymFluidControlCard card = cardAt<AxisymFluidControlTable>(none);
    REQUIRE((card.harmonics.empty() && !card.defaultDensity && !card.defaultBulkModulus)); // F = NONE is the empty list
    requireRoundTrip(none);

    const Model twice =
        readModelText("[model]\nAxisymFluidControl;2;1.;;;No;None\nAxisymFluidControl;3;1.;;;Yes;None\n", false);
    REQUIRE(twice.diagnostics().firstError()->code == "deck.duplicate-axisym-fluid-control");

    expectInvalid("AxisymFluidControl;2;9.75;0.0375;;No;None;1;2", "noHarmonicsFlag"); // no continuation with NONE
    expectInvalid("AxisymFluidControl;2;9.75;0.0375;;No;X;1", "noHarmonicsFlag");
    expectInvalid("AxisymFluidControl;2;9.75;0.0375;;No", "harmonic"); // blank F: the harmonics are listed
    expectInvalid("AxisymFluidControl;2;9.75;0.0375;;MAYBE;;1", "nonsymmetric");
    expectInvalid("AxisymFluidControl;2;9.75;0.0375;;;;1", "nonsymmetric");
    expectInvalid("AxisymFluidControl;0;9.75;0.0375;;No;;1", "frameId");
    expectInvalid("AxisymFluidControl;2;;0.03;;No;;1", "gravity");
    expectInvalid("AxisymFluidControl;2;32.2;-0.03;;No;;1", "defaultDensity");
    expectInvalid("AxisymFluidControl;2;9.75;0.0375;;No;;3;2", "harmonic"); // an increasing sequence
    expectInvalid("AxisymFluidControl;2;9.75;0.0375;;No;;2;2", "harmonic");
    expectInvalid("AxisymFluidControl;2;9.75;0.0375;;No;;100", "harmonic"); // below 100
    expectInvalid("AxisymFluidControl;2;9.75;0.0375;;No;;-1", "harmonic");
    expectInvalid("AxisymFluidControl;2;9.75;0.0375;;No;;1;Through", "lastHarmonic");
    expectInvalid("AxisymFluidControl;2;9.75;0.0375;;No;;5;Through;3", "lastHarmonic");
    expectInvalid("AxisymFluidControl;2;9.75;0.0375;;No;;1;Through;5;Step", "harmonicStep");
    expectInvalid("AxisymFluidControl;2;9.75;0.0375;;No;;1;Through;5;Step;0", "harmonicStep");
    expectInvalid("AxisymFluidControl;2;9.75;0.0375;;No;;Step;3", "harmonic");
}

TEST_CASE("an AXSLOT keeps its blank fields blank and is the only one of a deck", "[model][cards][axisymmetric]")
{
    const AcousticSlotControlCard example =
        cardAt<AcousticSlotControlTable>(readBulk("AcousticSlotControl;0.0125;250.;4;0.375;8"));
    REQUIRE((example.defaultDensity == 0.0125 && example.defaultBulkModulus == 250.0 && example.harmonic == 4 &&
             example.defaultSlotWidth == 0.375 && example.defaultSlotCount == 8));
    const AcousticSlotControlCard incompressible =
        cardAt<AcousticSlotControlTable>(readBulk("AcousticSlotControl;1.;0.;;0.5;0"));
    REQUIRE((incompressible.defaultBulkModulus == 0.0 && !incompressible.harmonic.has_value() &&
             incompressible.defaultSlotCount == 0));
    const Model blank = readBulk("AcousticSlotControl");
    const AcousticSlotControlCard none = cardAt<AcousticSlotControlTable>(blank);
    REQUIRE((!none.defaultDensity && !none.defaultBulkModulus && !none.harmonic && !none.defaultSlotWidth &&
             !none.defaultSlotCount));
    requireRoundTrip(blank);
    requireRoundTrip(readBulk("AcousticSlotControl;;;2"));
    REQUIRE((AcousticSlotControlTable::INFO.category == CardCategory::Other &&
             AcousticSlotControlTable::INFO.idSpace == base::IdSpace::None));

    const Model twice = readModelText("[model]\nAcousticSlotControl;1.\nAcousticSlotControl;2.\n", false);
    REQUIRE(twice.diagnostics().firstError()->code == "deck.duplicate-acoustic-slot-control");

    expectInvalid("AcousticSlotControl;0.", "defaultDensity");
    expectInvalid("AcousticSlotControl;-1.", "defaultDensity");
    expectInvalid("AcousticSlotControl;;-1.", "defaultBulkModulus");
    expectInvalid("AcousticSlotControl;;;-1", "harmonic");
    expectInvalid("AcousticSlotControl;;;;-0.5", "defaultSlotWidth");
    expectInvalid("AcousticSlotControl;;;;;-1", "defaultSlotCount");
    expectInvalid("AcousticSlotControl;x", "defaultDensity");
}

// ---------------------------------------------------------------------------------------------------------------------
// The axisymmetric fluid: RINGFL, FREEPT and PRESPT
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a RINGFL gives one fluid point per object, one or two to a card", "[model][cards][axisymmetric]")
{
    Model model = readBulk(
        "AxisymFluidRing;81101;1.5;;25.\nAxisymFluidRing;81102;4.;;10.;81103;8.;;10.\nAxisymFluidRing;81104;3.;45.");
    REQUIRE(rowCount<AxisymFluidRingTable>(model) == 4);
    const AxisymFluidRingCard first = cardAt<AxisymFluidRingTable>(model, 0);
    REQUIRE((first.id == 81101 && first.position1 == 1.5 && first.position2 == 0.0 && first.position3 == 25.0));
    const AxisymFluidRingCard second = cardAt<AxisymFluidRingTable>(model, 1);
    const AxisymFluidRingCard third = cardAt<AxisymFluidRingTable>(model, 2);
    REQUIRE((second.id == 81102 && second.position1 == 4.0 && second.position3 == 10.0));
    REQUIRE((third.id == 81103 && third.position1 == 8.0 && third.position3 == 10.0));
    const AxisymFluidRingCard spherical = cardAt<AxisymFluidRingTable>(model, 3);
    REQUIRE((spherical.position1 == 3.0 && spherical.position2 == 45.0 &&
             spherical.position3 == 0.0)); // the third coordinate is blank
    REQUIRE((AxisymFluidRingTable::INFO.category == CardCategory::Point &&
             AxisymFluidRingTable::INFO.idSpace == base::IdSpace::Node));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // Each point is written as a card of its own, in the layout of the format.

    // The number is one of the node ID space and is below 500000.
    REQUIRE_THROWS_AS(readBulk("Node;5\nAxisymFluidRing;5;1.;;2."), base::DeckError);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE(cardAt<AxisymFluidRingTable>(model, 2).id == 82103);

    expectInvalid("AxisymFluidRing;0;1.;;5.", "id");
    expectInvalid("AxisymFluidRing;500000;1.;;5.", "id");
    expectInvalid("AxisymFluidRing;5;0.;;5.", "position1");
    expectInvalid("AxisymFluidRing;5;-1.;;5.", "position1");
    expectInvalid("AxisymFluidRing;5", "firstPosition1");
    expectInvalid("AxisymFluidRing;5;1.;;5.;6",
                  "secondPosition1"); // a second point is there as soon as a field of it is
    expectInvalid("AxisymFluidRing;5;1.;;5.;;1.", "secondId");
    expectInvalid("AxisymFluidRing;5;1.;;5.;6;0.;;5.", "position1");
    expectInvalid("AxisymFluidRing;5;1.;x;5.", "firstPosition2");
}

TEST_CASE(
    "a FREEPT gives one free surface point per pair, in the layout of the format and with a blank field 2 before the "
    "pairs",
    "[model][cards][axisymmetric]")
{
    Model model = readBulk(
        "AxisymFluidRing;81101;1.5;;25.\n"
        "AxisymFluidFreePoint;81101;81201;12.5;81202;105.;81203;375.\n"
        "AxisymFluidFreePoint;81101;81204;15.;81205;30.");
    REQUIRE(rowCount<AxisymFluidFreePointTable>(model) == 5);
    const AxisymFluidFreePointCard last = cardAt<AxisymFluidFreePointTable>(model, 2);
    REQUIRE((last.fluidPointId == 81101 && last.pointId == 81203 && last.phiAngle == 375.0));
    // A blank field 2 starts the pairs in field 3.
    const AxisymFluidFreePointCard shifted = cardAt<AxisymFluidFreePointTable>(model, 3);
    REQUIRE((shifted.fluidPointId == 81101 && shifted.pointId == 81204 && shifted.phiAngle == 15.0));
    REQUIRE(cardAt<AxisymFluidFreePointTable>(model, 4).pointId == 81205);
    REQUIRE((AxisymFluidFreePointTable::INFO.category == CardCategory::Point &&
             AxisymFluidFreePointTable::INFO.idSpace == base::IdSpace::Node));
    REQUIRE(unresolvedIn(model) == 0); // the fluid point is a RINGFL
    requireRoundTrip(model);

    // The writer always gives the layout of the format, one point to a card.

    RecordingVisitor visitor;
    model.table<AxisymFluidFreePointTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"fluidPointId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Node});
    Model missing = readBulk("AxisymFluidFreePoint;81101;81201;12.5");
    REQUIRE(unresolvedIn(missing) == 1);

    // The point is in the node ID space; the fluid point it is on follows a renumbering of the nodes.
    REQUIRE_THROWS_AS(readBulk("Node;81201\nAxisymFluidFreePoint;81101;81201;12.5"), base::DeckError);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE((cardAt<AxisymFluidFreePointTable>(model, 0).fluidPointId == 82101 &&
             cardAt<AxisymFluidFreePointTable>(model, 0).pointId == 82201));
    REQUIRE(unresolvedIn(model) == 0);

    expectInvalid("AxisymFluidFreePoint;81101", "pointId1");
    expectInvalid("AxisymFluidFreePoint;81101;81201", "phiAngle");
    expectInvalid("AxisymFluidFreePoint;0;81201;1.", "fluidPointId");
    expectInvalid("AxisymFluidFreePoint;81101;0;1.", "pointId");
    expectInvalid("AxisymFluidFreePoint;81101;81201;1.;81202", "phiAngle");
    expectInvalid("AxisymFluidFreePoint;81101;81201;1.;81202;2.;81203;3.;;81204;4.",
                  "phiAngle3"); // at most three points
    expectInvalid("AxisymFluidFreePoint;81101;81201;1.;81202;2.;81203;3.;81204;4.", "phiAngle3");
    expectInvalid("AxisymFluidFreePoint;81101");
}

TEST_CASE("a PRESPT gives one pressure point per pair, with a blank pair between two points",
          "[model][cards][axisymmetric]")
{
    Model model = readBulk(
        "AxisymFluidRing;81101;1.5;;25.\nAxisymFluidPressurePoint;81101;81301;15.;;;81302;105."
        "\nAxisymFluidPressurePoint;81101;81303;30.;81304;60.");
    REQUIRE(rowCount<AxisymFluidPressurePointTable>(model) == 4);
    REQUIRE((cardAt<AxisymFluidPressurePointTable>(model, 0).pointId == 81301 &&
             cardAt<AxisymFluidPressurePointTable>(model, 0).phiAngle == 15.0));
    REQUIRE((cardAt<AxisymFluidPressurePointTable>(model, 1).pointId == 81302 &&
             cardAt<AxisymFluidPressurePointTable>(model, 1).phiAngle == 105.0));
    REQUIRE((cardAt<AxisymFluidPressurePointTable>(model, 2).fluidPointId == 81101 &&
             cardAt<AxisymFluidPressurePointTable>(model, 2).pointId == 81303));
    REQUIRE((cardAt<AxisymFluidPressurePointTable>(model, 3).pointId == 81304 &&
             cardAt<AxisymFluidPressurePointTable>(model, 3).phiAngle == 60.0));
    REQUIRE((AxisymFluidPressurePointTable::INFO.category == CardCategory::Point &&
             AxisymFluidPressurePointTable::INFO.idSpace == base::IdSpace::Node));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    Model missing = readBulk("AxisymFluidPressurePoint;81101;81301;0.");
    REQUIRE(unresolvedIn(missing) == 1);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE((cardAt<AxisymFluidPressurePointTable>(model, 0).fluidPointId == 82101 &&
             cardAt<AxisymFluidPressurePointTable>(model, 0).pointId == 82301));
    REQUIRE(unresolvedIn(model) == 0);

    expectInvalid("AxisymFluidPressurePoint;81101", "pointId1");
    expectInvalid("AxisymFluidPressurePoint;81101;81301", "phiAngle");
    expectInvalid("AxisymFluidPressurePoint;0;81301;1.", "fluidPointId");
    expectInvalid("AxisymFluidPressurePoint;81101;0;1.", "pointId");
    expectInvalid("AxisymFluidPressurePoint;81101;81301;1.;81302;2.;81303;3.;;81304;4.", "phiAngle3");
}

// ---------------------------------------------------------------------------------------------------------------------
// The slot cards: GRIDF, GRIDS, GRIDA, SLBDY
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a GRIDF is a point of the node ID space below 1000000", "[model][cards][axisymmetric]")
{
    const Model model = readBulk("AcousticSlotFluidPoint;81501;3.25;-6.5\nAcousticSlotFluidPoint;999999;0.5;0.");
    const AcousticSlotFluidPointCard card = cardAt<AcousticSlotFluidPointTable>(model, 0);
    REQUIRE((card.id == 81501 && card.radius == 3.25 && card.zCoord == -6.5));
    REQUIRE((AcousticSlotFluidPointTable::INFO.category == CardCategory::Point &&
             AcousticSlotFluidPointTable::INFO.idSpace == base::IdSpace::Node));
    requireRoundTrip(model);
    REQUIRE_THROWS_AS(readBulk("Node;5\nAcousticSlotFluidPoint;5;1.;1."), base::DeckError);

    expectInvalid("AcousticSlotFluidPoint;0;1.;2.", "id");
    expectInvalid("AcousticSlotFluidPoint;1000000;1.;2.", "id");
    expectInvalid("AcousticSlotFluidPoint;81501;0.;2.", "radius");
    expectInvalid("AcousticSlotFluidPoint;81501;-1.;2.", "radius");
    expectInvalid("AcousticSlotFluidPoint;81501;1.", "zCoord");
}

TEST_CASE("a GRIDS keeps a blank width blank and names the GRIDF of an interface", "[model][cards][axisymmetric]")
{
    Model model = readBulk(
        "AcousticSlotFluidPoint;81501;3.25;-6.5\n"
        "AcousticSlotSurfacePoint;81601;3.25;-6.5;0.625\n"
        "AcousticSlotSurfacePoint;81602;1.;0.\n"
        "AcousticSlotSurfacePoint;81603;3.;2.;0.25;81501\n"
        "AcousticSlotSurfacePoint;81604;-2.;1.\n"
        "AcousticSlotSurfacePoint;81605;1.;1.;0.");
    const AcousticSlotSurfacePointCard example = cardAt<AcousticSlotSurfacePointTable>(model, 0);
    REQUIRE((example.id == 81601 && example.radius == 3.25 && example.zCoord == -6.5 && example.slotWidth == 0.625 &&
             example.fluidPointId == 0));
    REQUIRE_FALSE(cardAt<AcousticSlotSurfacePointTable>(model, 1).slotWidth.has_value()); // the AXSLOT default applies
    const AcousticSlotSurfacePointCard onInterface = cardAt<AcousticSlotSurfacePointTable>(model, 2);
    REQUIRE((onInterface.slotWidth == 0.25 && onInterface.fluidPointId == 81501));
    REQUIRE(cardAt<AcousticSlotSurfacePointTable>(model, 3).radius ==
            -2.0); // R may be any real but zero when IDF is blank
    REQUIRE(cardAt<AcousticSlotSurfacePointTable>(model, 4).slotWidth == 0.0); // a zero width is not a blank one
    REQUIRE((AcousticSlotSurfacePointTable::INFO.category == CardCategory::Point &&
             AcousticSlotSurfacePointTable::INFO.idSpace == base::IdSpace::Node));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    RecordingVisitor visitor;
    model.table<AcousticSlotSurfacePointTable>().visitReferences(2, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"fluidPointId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Node});
    RecordingVisitor none;
    model.table<AcousticSlotSurfacePointTable>().visitReferences(0, none);
    REQUIRE(none.fields.empty());
    Model missing = readBulk("AcousticSlotSurfacePoint;81603;3.;2.;0.25;81501");
    REQUIRE(unresolvedIn(missing) == 1);

    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE((cardAt<AcousticSlotSurfacePointTable>(model, 2).id == 82603 &&
             cardAt<AcousticSlotSurfacePointTable>(model, 2).fluidPointId == 82501));

    expectInvalid("AcousticSlotSurfacePoint;0;1.;2.", "id");
    expectInvalid("AcousticSlotSurfacePoint;81601;0.;2.", "radius");
    expectInvalid("AcousticSlotSurfacePoint;81601;1.", "zCoord");
    expectInvalid("AcousticSlotSurfacePoint;81601;2.;2.;-1.", "slotWidth");
    expectInvalid("AcousticSlotSurfacePoint;81601;2.;2.;;-3", "fluidPointId");
    expectInvalid("AcousticSlotSurfacePoint;81601;-2.;2.;;81501", "radius"); // IDF needs R above zero
}

TEST_CASE("an associative node preserves the point and exact long part name", "[model][cards][axisymmetric]")
{
    const Model model = readBulk("AssociativeNode;81701;1000;Left_outboard_wing_tank\nAssociativeNode;81702;25;Wing");
    const AssociativeNodeCard card = cardAt<AssociativeNodeTable>(model, 0);
    REQUIRE((card.id == 81701 && card.origNodeId == 1000 && card.partitionName == "Left_outboard_wing_tank"));
    REQUIRE(cardAt<AssociativeNodeTable>(model, 1).partitionName == "Wing");
    REQUIRE((AssociativeNodeTable::INFO.category == CardCategory::Point &&
             AssociativeNodeTable::INFO.idSpace == base::IdSpace::Node));
    requireRoundTrip(model);

    // A deck in the small format puts the name in the fields of the continuation line.
    const Model fixed =
        readModelText("[model]\nAssociativeNode;81703;26;Fuselage_Forward_Section_Left_Hand_Part\n", true);
    REQUIRE(cardAt<AssociativeNodeTable>(fixed).partitionName == "Fuselage_Forward_Section_Left_Hand_Part");

    requireRoundTrip(fixed);

    // Long part names remain whole in a model record.
    const std::string longest(264, 'P');
    const Model full = readBulk("AssociativeNode;81704;27;" + longest);
    REQUIRE(cardAt<AssociativeNodeTable>(full).partitionName == longest);
    requireRoundTrip(full);

    // The point of the Part is not a card of this model: nothing is looked up and the ID is a plain integer.
    Model other = readBulk("AssociativeNode;81701;1000;Wing");
    REQUIRE(unresolvedIn(other) == 0);
    RecordingVisitor visitor;
    other.table<AssociativeNodeTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());
    REQUIRE_THROWS_AS(readBulk("Node;81701\nAssociativeNode;81701;1000;Wing"),
                      base::DeckError); // the GID is in the node space

    expectInvalid("AssociativeNode;81701;1000", "partitionName");
    expectInvalid("AssociativeNode;0;1000;Wing", "id");
    expectInvalid("AssociativeNode;81701;0;Wing", "origNodeId");
    oor::testing::requireModelFileRoundTrip(
        readModelText("AssociativeNode;81701;1000;Name_of_65_chars_" + std::string(50, 'x'), true));
    oor::testing::requireModelFileRoundTrip(
        readModelText("AssociativeNode;81701;1000;Two words", true)); // a name has no blank
}

TEST_CASE("an SLBDY lists the slot points of a boundary in order and keeps blank defaults blank",
          "[model][cards][axisymmetric]")
{
    Model model = readBulk(
        "AcousticSlotSurfacePoint;81601;1.;1.\n"
        "AcousticSlotSurfacePoint;81602;1.;2.\n"
        "AcousticSlotBoundary;0.0125;4;81601;81602;81603;81604;81605;81606;81607\n"
        "AcousticSlotBoundary;;;81601;81602\n"
        "AcousticSlotBoundary;;3;81602;;81601");
    const AcousticSlotBoundaryCard example = cardAt<AcousticSlotBoundaryTable>(model, 0);
    REQUIRE((example.density == 0.0125 && example.slotCount == 4));
    REQUIRE(example.points == std::vector<base::EntityId>{81601, 81602, 81603, 81604, 81605, 81606, 81607});
    const AcousticSlotBoundaryCard minimal = cardAt<AcousticSlotBoundaryTable>(model, 1);
    REQUIRE((!minimal.density && !minimal.slotCount));
    REQUIRE(minimal.points == std::vector<base::EntityId>{81601, 81602});
    // A blank field between two points is padding, and the order of the points is the topology of the boundary.
    REQUIRE(cardAt<AcousticSlotBoundaryTable>(model, 2).points == std::vector<base::EntityId>{81602, 81601});
    REQUIRE(cardAt<AcousticSlotBoundaryTable>(model, 2).slotCount == 3);
    REQUIRE((AcousticSlotBoundaryTable::INFO.category == CardCategory::Other &&
             AcousticSlotBoundaryTable::INFO.idSpace == base::IdSpace::None));
    requireRoundTrip(model);

    // The points are GRIDS points of the node ID space: five of the seven here are not defined.
    REQUIRE(unresolvedIn(model) == 5);
    RecordingVisitor visitor;
    model.table<AcousticSlotBoundaryTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"slotPoint", "slotPoint"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Node, ReferenceTarget::Node});
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE(cardAt<AcousticSlotBoundaryTable>(model, 1).points == std::vector<base::EntityId>{82601, 82602});

    // Seventeen points go over two continuation lines: six on the first line, eight on each other one.
    const Model many = readBulk(
        "AcousticSlotBoundary;0.5;0;81601;81602;81603;81604;81605;81606;81607;81608;81609;81610;81611;81612;81613;"
        "81614;81615;81616;81617");
    REQUIRE(cardAt<AcousticSlotBoundaryTable>(many).points.size() == 17);
    requireRoundTrip(many);

    expectInvalid("AcousticSlotBoundary;0.5;2;81601", "slotPoint2");
    expectInvalid("AcousticSlotBoundary", "slotPoint2");
    expectInvalid("AcousticSlotBoundary;-0.5;2;81601;81602", "density");
    expectInvalid("AcousticSlotBoundary;0.;2;81601;81602", "density");
    expectInvalid("AcousticSlotBoundary;;-1;81601;81602", "slotCount");
    expectInvalid("AcousticSlotBoundary;;;0;81602", "slotPoint");
    expectInvalid("AcousticSlotBoundary;;;81601;x", "slotPoint");
}

// ---------------------------------------------------------------------------------------------------------------------
// SNORM
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an SNORM gives the normal of a grid point in a system and has one card per grid point",
          "[model][cards][axisymmetric]")
{
    Model model = readBulk(
        "Node;3\nNode;4\nRectFrame2;2\nShellNormal;3;2;0.6;-0.8;0.\nShellNormal;4;;1.;0.;0.\nShellNormal;"
        "5\nShellNormal;6;2;0.5;;2.");
    const ShellNormalCard example = cardAt<ShellNormalTable>(model, 0);
    REQUIRE((example.nodeId == 3 && example.frameId == 2));
    REQUIRE((example.normal.x == 0.6 && example.normal.y == -0.8 && example.normal.z == 0.0));
    const ShellNormalCard basic = cardAt<ShellNormalTable>(model, 1);
    REQUIRE((basic.frameId == 0 && basic.normal.x == 1.0)); // CID 0 is the basic system
    const ShellNormalCard bare = cardAt<ShellNormalTable>(model, 2);
    REQUIRE(
        (bare.nodeId == 5 && bare.frameId == 0 && bare.normal == base::Vec3{})); // the components are 0.0 when blank
    REQUIRE((cardAt<ShellNormalTable>(model, 3).normal == base::Vec3{0.5, 0.0, 2.0}));
    REQUIRE((ShellNormalTable::INFO.category == CardCategory::Other &&
             ShellNormalTable::INFO.idSpace == base::IdSpace::PerCardType));
    requireRoundTrip(model);

    // The grid point and the system are references; the grids 5 and 6 are not defined.
    RecordingVisitor visitor;
    model.table<ShellNormalTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"nodeId", "frameId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Node, ReferenceTarget::Coordinate});
    RecordingVisitor basicVisitor;
    model.table<ShellNormalTable>().visitReferences(1, basicVisitor); // the basic system is not looked up
    REQUIRE(basicVisitor.fields == std::vector<std::string>{"nodeId"});
    REQUIRE(unresolvedIn(model) == 2);

    // One normal for a grid point, whatever other cards have the number; a renumbering of the nodes moves it.
    REQUIRE_THROWS_AS(readBulk("ShellNormal;3\nShellNormal;3;2"), base::DeckError);
    REQUIRE_NOTHROW(readBulk("Node;3\nShellNormal;3"));
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE((cardAt<ShellNormalTable>(model, 0).nodeId == 103 && cardAt<ShellNormalTable>(model, 1).nodeId == 104));
    REQUIRE(unresolvedIn(model) == 2);

    expectInvalid("ShellNormal", "nodeId");
    expectInvalid("ShellNormal;0;2;0.;1.;0.", "nodeId");
    expectInvalid("ShellNormal;3;-1;0.;1.;0.", "frameId");
    expectInvalid("ShellNormal;3;2;0.;x;0.", "directionY");
}

// ---------------------------------------------------------------------------------------------------------------------
// The loads and the omitted degrees of freedom: MOMAX, PRESAX, OMITAX
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a MOMAX has a harmonic or a sequence of them, a load set ID and a ring", "[model][cards][axisymmetric]")
{
    Model model = readBulk(
        "AxisymConeRing;81001;2.;1.\n"
        "AxisymRingMomentLoad;81901;81001;3;1.5;0.25;0.5;0.75\n"
        "AxisymRingMomentLoad;81902;81001;S0T10;2.;0.5\n"
        "AxisymRingMomentLoad;81903;81001;0;1.;;;5.");
    const AxisymRingMomentLoadCard example = cardAt<AxisymRingMomentLoadTable>(model, 0);
    REQUIRE((example.id == 81901 && example.ringId == 81001 && example.harmonicId == 3 && !example.isSequence()));
    REQUIRE((example.scaleFactor == 1.5 && example.radialMoment == 0.25 && example.azimuthalMoment == 0.5 &&
             example.axialMoment == 0.75));
    const AxisymRingMomentLoadCard sequence = cardAt<AxisymRingMomentLoadTable>(model, 1);
    REQUIRE((sequence.harmonicId == 0 && sequence.harmonicEnd == 10 && sequence.isSequence()));
    REQUIRE((sequence.scaleFactor == 2.0 && sequence.radialMoment == 0.5 && sequence.azimuthalMoment == 0.0 &&
             sequence.axialMoment == 0.0));
    const AxisymRingMomentLoadCard zero = cardAt<AxisymRingMomentLoadTable>(model, 2);
    REQUIRE((zero.harmonicId == 0 && !zero.isSequence() && zero.radialMoment == 0.0 &&
             zero.axialMoment == 5.0)); // harmonic 0 is allowed
    REQUIRE((AxisymRingMomentLoadTable::INFO.category == CardCategory::Load &&
             AxisymRingMomentLoadTable::INFO.idSpace == base::IdSpace::Group));
    requireRoundTrip(model);

    RecordingVisitor visitor;
    model.table<AxisymRingMomentLoadTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"ringId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Node});
    REQUIRE(unresolvedIn(model) == 0);
    Model missing = readBulk("AxisymRingMomentLoad;81901;81009;3;1.");
    REQUIRE(unresolvedIn(missing) == 1);

    // The ring follows the nodes, the set ID the load sets.
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE(cardAt<AxisymRingMomentLoadTable>(model, 0).ringId == 82001);
    Renumbering::apply(model, RenumberSpace::LoadCombinationSet, Renumbering::offset(10));
    REQUIRE((cardAt<AxisymRingMomentLoadTable>(model, 0).id == 81911 &&
             cardAt<AxisymRingMomentLoadTable>(model, 1).id == 81912));

    expectInvalid("AxisymRingMomentLoad;0;81001;3;1.", "id");
    expectInvalid("AxisymRingMomentLoad;81901;0;3;1.", "ringId");
    expectInvalid("AxisymRingMomentLoad;81901;81001;-1;1.", "harmonicId");
    expectInvalid("AxisymRingMomentLoad;81901;81001;S3;1.", "harmonicId");
    expectInvalid("AxisymRingMomentLoad;81901;81001;S5T3;1.", "harmonicId");
    expectInvalid("AxisymRingMomentLoad;81901;81001;SxT3;1.", "harmonicId");
    expectInvalid("AxisymRingMomentLoad;81901;81001;3", "scaleFactor");
}

TEST_CASE("a PRESAX gives a pressure between two rings over an angle, 0 to 360 degrees when the angles are blank",
          "[model][cards][axisymmetric]")
{
    Model model = readBulk(
        "AxisymConeRing;81001;2.;1.\n"
        "AxisymConeRing;81002;3.;1.\n"
        "AxisymConePressureLoad;81911;6.25;81002;81001;22.5;37.5\n"
        "AxisymConePressureLoad;81912;0.1;81001;81002\n"
        "AxisymConePressureLoad;81913;0.1;81001;81002;0.;360.");
    const AxisymConePressureLoadCard example = cardAt<AxisymConePressureLoadTable>(model, 0);
    REQUIRE((example.id == 81911 && example.pressure == 6.25 && example.ringId1 == 81002 && example.ringId2 == 81001));
    REQUIRE((example.phiAngle1 == 22.5 && example.phiAngle2 == 37.5));
    // A blank angle is 0 and 360 degrees (the format has no default); the values are stored.
    REQUIRE((cardAt<AxisymConePressureLoadTable>(model, 1).phiAngle1 == 0.0 &&
             cardAt<AxisymConePressureLoadTable>(model, 1).phiAngle2 == 360.0));
    REQUIRE((cardAt<AxisymConePressureLoadTable>(model, 2).phiAngle1 == 0.0 &&
             cardAt<AxisymConePressureLoadTable>(model, 2).phiAngle2 == 360.0));
    REQUIRE((AxisymConePressureLoadTable::INFO.category == CardCategory::Load &&
             AxisymConePressureLoadTable::INFO.idSpace == base::IdSpace::Group));
    requireRoundTrip(model);
    // The angles are always written, so that the card the writer gives is the one the format describes.

    RecordingVisitor visitor;
    model.table<AxisymConePressureLoadTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"ringId1", "ringId2"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Node, ReferenceTarget::Node});
    REQUIRE(unresolvedIn(model) == 0);
    Model missing = readBulk("AxisymConeRing;81001;2.;1.\nAxisymConePressureLoad;81911;1.;81001;81002");
    REQUIRE(unresolvedIn(missing) == 1);

    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE((cardAt<AxisymConePressureLoadTable>(model, 0).ringId1 == 82002 &&
             cardAt<AxisymConePressureLoadTable>(model, 0).ringId2 == 82001));
    Renumbering::apply(model, RenumberSpace::LoadCombinationSet, Renumbering::offset(10));
    REQUIRE(cardAt<AxisymConePressureLoadTable>(model, 2).id == 81923);

    expectInvalid("AxisymConePressureLoad;0;1.;81001;81002", "id");
    expectInvalid("AxisymConePressureLoad;81911;;81001;81002", "pressure");
    expectInvalid("AxisymConePressureLoad;81911;1.;0;81002", "ringId1");
    expectInvalid("AxisymConePressureLoad;81911;1.;81001;0", "ringId2");
    expectInvalid("AxisymConePressureLoad;81911;1.;81001;81002;30.;20.", "phiAngle2");
    expectInvalid("AxisymConePressureLoad;81911;1.;81001;81002;30.;30.", "phiAngle2");
    expectInvalid("AxisymConePressureLoad;81911;1.;81001;81002;;-10.",
                  "phiAngle2"); // the default PHI1 of 0.0 is above it
}

TEST_CASE("an OMITAX lists up to twelve degrees of freedom of the rings, two to a line", "[model][cards][axisymmetric]")
{
    Model model = readBulk(
        "AxisymConeRing;81001;2.;1.\n"
        "AxisymConeRing;81002;3.;1.\n"
        "AxisymConeRing;81003;4.;1.\n"
        "AxisymOmittedDofSet;81001;6;3;81002;7;1\n"
        "AxisymOmittedDofSet;81001;0;321\n"
        "AxisymOmittedDofSet;81001;0;1;81001;1;2;81002;0;3;81002;1;4;81003;2;5");
    REQUIRE(
        cardAt<AxisymOmittedDofSetTable>(model, 0).dofs ==
        std::vector<AxisymOmittedDofSetDof>{AxisymOmittedDofSetDof{81001, 6, 3}, AxisymOmittedDofSetDof{81002, 7, 1}});
    REQUIRE(cardAt<AxisymOmittedDofSetTable>(model, 1).dofs ==
            std::vector<AxisymOmittedDofSetDof>{AxisymOmittedDofSetDof{81001, 0, 123}}); // sorted
    const AxisymOmittedDofSetCard five = cardAt<AxisymOmittedDofSetTable>(model, 2);
    REQUIRE(five.dofs == std::vector<AxisymOmittedDofSetDof>{
                             AxisymOmittedDofSetDof{81001, 0, 1}, AxisymOmittedDofSetDof{81001, 1, 2},
                             AxisymOmittedDofSetDof{81002, 0, 3}, AxisymOmittedDofSetDof{81002, 1, 4},
                             AxisymOmittedDofSetDof{81003, 2, 5}});
    REQUIRE((AxisymOmittedDofSetTable::INFO.category == CardCategory::Set &&
             AxisymOmittedDofSetTable::INFO.idSpace == base::IdSpace::None));
    requireRoundTrip(model);

    // Two triples to a line: the third starts the first continuation line.

    RecordingVisitor visitor;
    model.table<AxisymOmittedDofSetTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"ringId", "ringId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Node, ReferenceTarget::Node});
    REQUIRE(unresolvedIn(model) == 0);
    Model missing = readBulk("AxisymOmittedDofSet;81001;6;3");
    REQUIRE(unresolvedIn(missing) == 1);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE(cardAt<AxisymOmittedDofSetTable>(model, 0).dofs.front().ring == 82001);

    REQUIRE(cardAt<AxisymOmittedDofSetTable>(readBulk(omitax(12))).dofs.size() == 12);
    requireRoundTrip(readBulk(omitax(12)));
    expectInvalid(omitax(13), "ringId");
    expectInvalid("AxisymOmittedDofSet", "ringId1");
    expectInvalid("AxisymOmittedDofSet;0;0;3", "ringId");
    expectInvalid("AxisymOmittedDofSet;81001;-1;3", "harmonicId");
    expectInvalid("AxisymOmittedDofSet;81001;0;7", "components");
    expectInvalid("AxisymOmittedDofSet;81001;0;0", "components");
    expectInvalid("AxisymOmittedDofSet;81001;0", "components");
    expectInvalid("AxisymOmittedDofSet;81001;0;3;81002", "harmonicId");
}
