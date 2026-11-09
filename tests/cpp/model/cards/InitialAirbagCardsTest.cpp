// Checks of the initial conditions and the air bag heat cards of the explicit solver: TIC3,
// TICEL and TICEUDS (the initial conditions that the IC command selects), TICEUL1, TICREG and TICVAL (the initial
// values of the Eulerian regions), GBAGCOU (the switch of an air bag to uniform pressure), HEATLOS, HTRCONV and HTRRAD
// (the heat transfer of an air bag surface), and the PEULER1, COUPLE and GBAG fields that now resolve to them. Per
// card: the fields in their format reference positions, the defaults, the references it lists and what each one points
// to, the renumbering of the kinds it owns, the refused values and the round trip through the three formats. The
// generic tests of CardRoundTripTest.cpp read, write and copy every sample; this file covers what each card means.

#include "oor/model/cards/dynamics/initialElemValues/InitialElemValuesTable.hpp"
#include "oor/model/cards/dynamics/initialValuesUserRoutine/InitialValuesUserRoutineTable.hpp"
#include "oor/model/cards/dynamics/initialVelocityField/InitialVelocityFieldTable.hpp"
#include "oor/model/cards/explicit/cylinderShape/CylinderShapeTable.hpp"
#include "oor/model/cards/explicit/eulerInitialRegions/EulerInitialRegionsTable.hpp"
#include "oor/model/cards/explicit/eulerLagrangeCoupling/EulerLagrangeCouplingTable.hpp"
#include "oor/model/cards/explicit/eulerRegionProp/EulerRegionPropTable.hpp"
#include "oor/model/cards/explicit/gasBag/GasBagTable.hpp"
#include "oor/model/cards/explicit/gasBagConvection/GasBagConvectionTable.hpp"
#include "oor/model/cards/explicit/gasBagHeatTransfer/GasBagHeatTransferTable.hpp"
#include "oor/model/cards/explicit/gasBagRadiation/GasBagRadiationTable.hpp"
#include "oor/model/cards/explicit/gasBagSwitch/GasBagSwitchTable.hpp"
#include "oor/model/cards/explicit/initialConditionRegion/InitialConditionRegionTable.hpp"
#include "oor/model/cards/explicit/initialConditionValues/InitialConditionValuesTable.hpp"
#include "oor/model/cards/explicit/initialSurfaceVolume/InitialSurfaceVolumeTable.hpp"
#include "oor/model/cards/explicit/sphereShape/SphereShapeTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <limits>
#include <optional>
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

// The points of a table of time, the TableDynamic1 that the coefficients and the radial profiles name.
const std::string TABLE_TEXT = ";;;0.;1.;1.;1.;EndTable";

// The fields of a GasBag after its HTRID when it has nothing but its required fields (pressures and temperatures of
// the gas).
const std::string GAS_BAG_REST = ";;;1.E+3;297.;101325.;;;;293.";
// One reference as a visitor sees it: the field, what it points to and the ID it holds.
struct Seen
{
    std::string field;
    ReferenceTarget target;
    base::EntityId id;

    [[nodiscard]] bool operator==(const Seen&) const = default;
};

struct SeenVisitor final : ReferenceVisitor
{
    std::vector<Seen> seen;
    base::EntityId visit(std::string_view field, ReferenceTarget target, base::EntityId id) override
    {
        seen.push_back(Seen{std::string(field), target, id});
        return id;
    }
};

// The references of the card at `row`, in the order the card lists them.
template <typename TableT>
std::vector<Seen> seenBy(Model& model, std::uint32_t row = 0)
{
    SeenVisitor visitor;
    model.table<TableT>().visitReferences(row, visitor);
    return visitor.seen;
}

// The field texts a card writes in the free format: index 0 is field 1, a blank field is an empty text.
template <typename FormatT, typename CardT>
std::vector<std::string> writtenFields(const CardT& card)
{
    FieldWriter writer;
    FormatT::write(card, writer);
    return writer.fields();
}

// Whether the check of a card finds a problem, and in which field.
template <typename FormatT, typename CardT>
std::string problemField(const CardT& card)
{
    const std::optional<CardProblem> problem = FormatT::check(card);
    return problem ? std::string(problem->field) : std::string("none");
}

} // namespace

// ---------------------------------------------------------------------------------------------------------------------
// TIC3
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a TIC3 names a grid point at the centre and a list of grid points, and renumbers them",
          "[model][cards][tic3]")
{
    Model model = readBulk(
        "Node;1\n"
        "Node;2\n"
        "Node;3\n"
        "Node;4\n"
        "Node;5\n"
        "Node;6\n"
        "InitialVelocityField;7;5;12.;85.;;;;;;1;2;Through;4\n"
        "InitialVelocityField;8;6;;;;;;;;3\n");
    REQUIRE(unresolvedIn(model) == 0);
    // The centre and the single IDs are grid points, the two ends of a range are the ends of a range (which may not
    // exist), and the points between them are not listed.
    REQUIRE(seenBy<InitialVelocityFieldTable>(model, 0) ==
            std::vector<Seen>{{"node", ReferenceTarget::Node, 5},
                              {"node", ReferenceTarget::Node, 1},
                              {"node", ReferenceTarget::NodeInRange, 2},
                              {"node", ReferenceTarget::NodeInRange, 4}});
    REQUIRE(seenBy<InitialVelocityFieldTable>(model, 1) ==
            std::vector<Seen>{{"node", ReferenceTarget::Node, 6}, {"node", ReferenceTarget::Node, 3}});

    // A grid point that no card has is an unresolved reference; the range may start or end where the model has no
    // point.
    Model missing = readBulk("Node;5\nNode;1\nInitialVelocityField;7;5;;;;;;;;1;7");
    REQUIRE(unresolvedIn(missing) == 1); // the single point 7
    Model open = readBulk("Node;5\nNode;1\nInitialVelocityField;7;5;;;;;;;;1;7;Through;40");
    REQUIRE(unresolvedIn(open) == 0);

    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    const InitialVelocityFieldCard first = cardAt<InitialVelocityFieldTable>(model, 0);
    REQUIRE(first.node == 105);
    REQUIRE(first.grids == std::vector<ContactRun>{{101, 101, 1}, {102, 104, 1}});
    REQUIRE((cardAt<InitialVelocityFieldTable>(model, 1).node == 106 &&
             cardAt<InitialVelocityFieldTable>(model, 1).grids[0].first == 103));
    REQUIRE(unresolvedIn(model) == 0);
    // The set IDs are the space of the initial conditions, which the IC command names.
    Renumbering::apply(model, RenumberSpace::InitialCondition, Renumbering::offset(1000));
    REQUIRE((cardAt<InitialVelocityFieldTable>(model, 0).id == 1007 &&
             cardAt<InitialVelocityFieldTable>(model, 1).id == 1008));
    requireRoundTrip(model);

    // Deleting a grid point leaves the card naming it.
    model.remove(*model.locate(base::IdSpace::Node, 105));
    REQUIRE(unresolvedIn(model) == 1);
}

TEST_CASE("a TIC3 refuses a value that is not in its layout", "[model][cards][tic3]")
{
    expectInvalid("InitialVelocityField;0;5;;;;;;;;1", "id");
    expectInvalid("InitialVelocityField;-7;5;;;;;;;;1", "id");
    expectInvalid("InitialVelocityField;7;0;;;;;;;;1", "node");
    expectInvalid("InitialVelocityField;7", "node");    // required
    expectInvalid("InitialVelocityField;7;5", "node1"); // the list is required
    expectInvalid("InitialVelocityField;7;5;abc;;;;;;;1", "scaleFactor");
    expectInvalid("InitialVelocityField;7;5;;x;;;;;;1", "xVelocity");
    expectInvalid("InitialVelocityField;7;5;;;;;;;abc;1", "zRotation");
    // THRU and BY: a range goes up, BY needs a range and a step with the way of the range.
    expectInvalid("InitialVelocityField;7;5;;;;;;;;1;2;THRY;1000", "node1");
    expectInvalid("InitialVelocityField;7;5;;;;;;;;20;Through;10", "node1");
    expectInvalid("InitialVelocityField;7;5;;;;;;;;Through;10", "Through");
    expectInvalid("InitialVelocityField;7;5;;;;;;;;10;Through", "Through");
    expectInvalid("InitialVelocityField;7;5;;;;;;;;10;By;2", "By");
    expectInvalid("InitialVelocityField;7;5;;;;;;;;10;Through;20;By;0", "By");
    expectInvalid("InitialVelocityField;7;5;;;;;;;;10;Through;20;By;-2", "By");
    expectInvalid("InitialVelocityField;7;5;;;;;;;;1;0;3", "node1"); // a zero is not a grid point
    expectInvalid("InitialVelocityField;7;5;;;;;;;;1;-3", "node1");
    expectInvalid("InitialVelocityField;7;5;;;;;;;;1;abc", "node1");

    // The check of the values is what editing applies: an empty list and a velocity that is not a number.
    InitialVelocityFieldCard card;
    card.id = 7;
    card.node = 5;
    REQUIRE(problemField<InitialVelocityFieldFormat>(card) == "node1");
    card.grids = {ContactRun{1, 1, 1}};
    REQUIRE(problemField<InitialVelocityFieldFormat>(card) == "none");
    card.scaleFactor = std::numeric_limits<double>::infinity();
    REQUIRE(problemField<InitialVelocityFieldFormat>(card) == "scaleFactor");
    card.scaleFactor = 1.0;
    card.zRotation = std::numeric_limits<double>::quiet_NaN();
    REQUIRE(problemField<InitialVelocityFieldFormat>(card) == "zRotation");
}

// ---------------------------------------------------------------------------------------------------------------------
// TICEL
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a TICEL gives pairs of a variable and a value from field 3 on any number of lines", "[model][cards][ticel]")
{
    Model model = readBulk(
        "IdListSet1;40;1;2\n"
        "IdListSet1;41;1;2\n"
        "IdListSet1;42;1;2\n"
        "Node;1\n"
        "Node;2\n"
        "InitialElemValues;3;40;Density;920.;SpecificInternalEnergy;2.8E+5\n"
        "InitialElemValues;4;41;VelocityX;1.;VelocityY;2.;VelocityZ;3.;Density;1.E+3;SpecificInternalEnergy;2.5E+5;"
        "ArtificialViscosity;0.5;Divergence;0.;VoidFraction;0.1;MaterialFraction;0.9;MomentumX;1."
        "5;MomentumY;2.5;MomentumZ;3.5\n"
        "InitialElemValues;3;42;CenterX;0.;CenterY;-1.5;CenterZ;2.;RadialVelocity;50.;Decay;0.2\n"
        "InitialElemValues;5;40;SXX;1.E+6");
    REQUIRE(rowCount<InitialElemValuesTable>(model) == 4);
    const InitialElemValuesCard example = cardAt<InitialElemValuesTable>(model, 0);
    REQUIRE((example.id == 3 && example.elementSetId == 40));
    REQUIRE(example.variables == std::vector<InitialVariable>{{"Density", 920.0}, {"SpecificInternalEnergy", 2.8E5}});

    // The twelve variables of the Eulerian regions over three lines: the pairs go on after the third one on the next
    // line.
    const InitialElemValuesCard twelve = cardAt<InitialElemValuesTable>(model, 1);
    REQUIRE((twelve.id == 4 && twelve.elementSetId == 41 && twelve.variables.size() == 12));
    REQUIRE((twelve.variables.front() == InitialVariable{"VelocityX", 1.0} && twelve.variables[3].name == "Density"));
    REQUIRE((twelve.variables[9] == InitialVariable{"MomentumX", 1.5} &&
             twelve.variables.back() == InitialVariable{"MomentumZ", 3.5}));

    // The radial velocity field, the names in any case (kept in capitals), numbers all of them.
    const InitialElemValuesCard radial = cardAt<InitialElemValuesTable>(model, 2);
    REQUIRE((radial.id == 3 && radial.elementSetId == 42 && radial.variables.size() == 5));
    REQUIRE((radial.variables[0] == InitialVariable{"CenterX", 0.0} &&
             radial.variables[3] == InitialVariable{"RadialVelocity", 50.0}));
    REQUIRE(radial.variables[4] == InitialVariable{"Decay", 0.2});
    // A variable the format does not list (a Lagrangian one) is kept as written.
    REQUIRE(cardAt<InitialElemValuesTable>(model, 3).variables == std::vector<InitialVariable>{{"SXX", 1.0E6}});

    // The cards of a set share the SID, the ID of the table (IdSpace::Group, category InitialCondition).
    const InitialElemValuesTable& table = *model.findTable<InitialElemValuesTable>();
    REQUIRE((table.idAtRow(0) == 3 && table.idAtRow(2) == 3 && table.idAtRow(1) == 4));
    REQUIRE((InitialElemValuesTable::INFO.category == CardCategory::InitialCondition &&
             InitialElemValuesTable::INFO.idSpace == base::IdSpace::Group));
    requireRoundTrip(model);

    // Name and value are in the fields 3 and 4, 5 and 6, ...: three pairs on the first line, then four to a line.
    const std::vector<std::string> fields = writtenFields<InitialElemValuesFormat>(twelve);
    REQUIRE(fields.size() == 26);
    REQUIRE((fields[0] == "4" && fields[1] == "41" && fields[2] == "VelocityX" && fields[4] == "VelocityY" &&
             fields[6] == "VelocityZ"));
    REQUIRE((fields[8] == "Density" && fields[10] == "SpecificInternalEnergy" && fields[12] == "ArtificialViscosity" &&
             fields[14] == "Divergence"));
    REQUIRE((fields[16] == "VoidFraction" && fields[20] == "MomentumX" && fields[24] == "MomentumZ" &&
             fields[25] == "3.5"));

    REQUIRE(seenBy<InitialElemValuesTable>(model, 0) == std::vector<Seen>{{"elementSetId", ReferenceTarget::Set, 40}});
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(100));
    REQUIRE((cardAt<InitialElemValuesTable>(model, 0).elementSetId == 140 &&
             cardAt<InitialElemValuesTable>(model, 1).elementSetId == 141));
    Renumbering::apply(model, RenumberSpace::InitialCondition, Renumbering::offset(1000));
    REQUIRE(
        (cardAt<InitialElemValuesTable>(model, 0).id == 1003 && cardAt<InitialElemValuesTable>(model, 2).id == 1003));
    REQUIRE(
        (cardAt<InitialElemValuesTable>(model, 1).id == 1004 && cardAt<InitialElemValuesTable>(model, 3).id == 1005));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    Model dangling = readBulk("InitialElemValues;3;40;Density;100.");
    REQUIRE(unresolvedIn(dangling) == 1);
}

TEST_CASE("a TICEL refuses a missing name or value and a pair after a blank one", "[model][cards][ticel]")
{
    expectInvalid("InitialElemValues;0;40;Density;100.", "id");
    expectInvalid("InitialElemValues;3;0;Density;100.", "elementSetId");
    expectInvalid("InitialElemValues;3", "elementSetId");             // required
    expectInvalid("InitialElemValues;3;40", "variableName");          // at least one variable
    expectInvalid("InitialElemValues;3;40;Density", "variableValue"); // a name with no value
    expectInvalid("InitialElemValues;3;40;;100.", "variableName");    // a value with no name
    expectInvalid("InitialElemValues;3;40;Density;abc", "variableValue");
    expectInvalid("InitialElemValues;3;40;Density;100.;;;SpecificInternalEnergy;1.",
                  "variableName"); // a blank field ends the list
    expectInvalid("InitialElemValues;3;40;Density;100.;SpecificInternalEnergy;1.E+5;ArtificialViscosity;1.;VelocityX",
                  "variableValue");
    requireRoundTrip(readBulk("InitialElemValues;3;40;MOMENTUMX;1.")); // nine characters

    // The padding of the continuation lines is not a pair: a blank pair at the end is fine.
    REQUIRE_NOTHROW(readBulk("InitialElemValues;3;40;Density;100."));
    InitialElemValuesCard card;
    card.id = 3;
    card.elementSetId = 40;
    REQUIRE(problemField<InitialElemValuesFormat>(card) == "variableName");
    card.variables = {InitialVariable{"Density", 100.0}};
    REQUIRE(problemField<InitialElemValuesFormat>(card) == "none");
    card.variables.push_back(InitialVariable{"SpecificInternalEnergy", std::numeric_limits<double>::infinity()});
    REQUIRE(problemField<InitialElemValuesFormat>(card) == "variableValue");
}

// ---------------------------------------------------------------------------------------------------------------------
// TICEUDS
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a TICEUDS has a group, the routine EXINIT, a set and the elements or grids it applies to",
          "[model][cards][ticeuds]")
{
    Model model = readBulk(
        "InitialValuesUserRoutine;12;seedgrp;ExternalInitialValues;100;Node\n"
        "InitialValuesUserRoutine;13;mygroup;;100\n"
        "InitialValuesUserRoutine;12;GROUP1;ExternalInitialValues;101;Element\n"
        "IdListSet1;100;1;2\n"
        "IdListSet1;101;1;2\n"
        "Node;1\n"
        "Node;2");
    const InitialValuesUserRoutineCard example = cardAt<InitialValuesUserRoutineTable>(model, 0);
    REQUIRE((example.id == 12 && example.group == "seedgrp" &&
             example.routineName == InitialValuesUserRoutineRoutine::Exinit));
    REQUIRE((example.elementSetId == 100 && example.target == InitialValuesUserRoutineTarget::Grid));
    // Group names keep their exact spelling; routineName and target have defaults.
    const InitialValuesUserRoutineCard defaults = cardAt<InitialValuesUserRoutineTable>(model, 1);
    REQUIRE((defaults.id == 13 && defaults.group == "mygroup" &&
             defaults.routineName == InitialValuesUserRoutineRoutine::Exinit));
    REQUIRE((defaults.elementSetId == 100 && defaults.target == InitialValuesUserRoutineTarget::Element));
    // Reserved words use their exact neutral spelling.
    const InitialValuesUserRoutineCard lower = cardAt<InitialValuesUserRoutineTable>(model, 2);
    REQUIRE((lower.group == "GROUP1" && lower.routineName == InitialValuesUserRoutineRoutine::Exinit &&
             lower.target == InitialValuesUserRoutineTarget::Element));
    const InitialValuesUserRoutineTable& table = *model.findTable<InitialValuesUserRoutineTable>();
    REQUIRE((table.idAtRow(0) == 12 && table.idAtRow(2) == 12 && table.idAtRow(1) == 13));
    REQUIRE((InitialValuesUserRoutineTable::INFO.category == CardCategory::InitialCondition &&
             InitialValuesUserRoutineTable::INFO.idSpace == base::IdSpace::Group));
    requireRoundTrip(model);

    // The defaults are written as blanks (field 3 and field 5).
    const std::vector<std::string> fields = writtenFields<InitialValuesUserRoutineFormat>(defaults);
    REQUIRE(fields.size() == 5); // COPT is the fifth field, blank at its default
    REQUIRE((fields[0] == "13" && fields[1] == "mygroup" && fields[2].empty() && fields[3] == "100"));
    const std::vector<std::string> full = writtenFields<InitialValuesUserRoutineFormat>(example);
    REQUIRE((full.size() == 5 && full[4] == "Node"));

    REQUIRE(seenBy<InitialValuesUserRoutineTable>(model, 0) ==
            std::vector<Seen>{{"elementSetId", ReferenceTarget::Set, 100}});
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(10));
    REQUIRE((cardAt<InitialValuesUserRoutineTable>(model, 0).elementSetId == 110 &&
             cardAt<InitialValuesUserRoutineTable>(model, 2).elementSetId == 111));
    Renumbering::apply(model, RenumberSpace::InitialCondition, Renumbering::offset(1000));
    REQUIRE((cardAt<InitialValuesUserRoutineTable>(model, 0).id == 1012 &&
             cardAt<InitialValuesUserRoutineTable>(model, 1).id == 1013));
    REQUIRE(unresolvedIn(model) == 0);
    Model dangling = readBulk("InitialValuesUserRoutine;12;initex;ExternalInitialValues;100;Node");
    REQUIRE(unresolvedIn(dangling) == 1);
}

TEST_CASE("a TICEUDS refuses a group that is no name, another routine and a sixth field", "[model][cards][ticeuds]")
{
    expectInvalid("InitialValuesUserRoutine;0;g;;100", "id");
    expectInvalid("InitialValuesUserRoutine;12;;;100", "group");                // no default
    requireRoundTrip(readBulk("InitialValuesUserRoutine;12;TOOLONGNAME;;100")); // more than a field
    expectInvalid("InitialValuesUserRoutine;12;g;MYINIT;100", "routineName");   // EXINIT only
    expectInvalid("InitialValuesUserRoutine;12;g;;0", "elementSetId");
    expectInvalid("InitialValuesUserRoutine;12;g", "elementSetId"); // required
    expectInvalid("InitialValuesUserRoutine;12;g;;100;POINT", "target");
    InitialValuesUserRoutineCard card;
    card.id = 12;
    card.elementSetId = 100;
    REQUIRE(problemField<InitialValuesUserRoutineFormat>(card) == "group");
    card.group = "g";
    REQUIRE(problemField<InitialValuesUserRoutineFormat>(card) == "none");
}

// ---------------------------------------------------------------------------------------------------------------------
// TICEUL1, TICREG and TICVAL
// ---------------------------------------------------------------------------------------------------------------------

namespace
{

// A PEULER1 with its TICEUL1, the five regions of a group (one of each type, the SPHERE with a material and a TICVAL)
// and everything the regions name.
const std::string REGIONS_TEXT =
    "EulerRegionProp;100;;300\n"
    "EulerInitialRegions;300;200\n"
    "InitialConditionRegion;1;200;Sphere;400;25;3;4.\n"
    "InitialConditionRegion;2;200;Surface;401;25;3\n"
    "InitialConditionRegion;3;200;Box;402\n"
    "InitialConditionRegion;4;200;Elements;403\n"
    "InitialConditionRegion;5;200;Cylinder;404;;3;-1.5\n"
    "SphereShape;400;0.;0.;0.;1.\n"
    "InitialSurfaceVolume;401;60\n"
    "CylinderShape;404;0.;0.;0.;0.;0.;1.;0.5\n"
    "ContactElemSurface;402;1\n"
    "ContactElemSurface;60;1\n"
    "IdListSet1;403;1;2\n"
    "EulerMat;25;1.E+3;1\n"
    "GammaLawStateEquation;1;1.4\n"
    "InitialConditionValues;3;;Density;1.E+3";

} // namespace

TEST_CASE("a TICEUL1 names the group of TICREG cards that a PEULER1 reaches through it", "[model][cards][ticeul1]")
{
    Model model = readBulk(REGIONS_TEXT + SURFACE_ELEMENT);
    const EulerInitialRegionsCard regions = cardAt<EulerInitialRegionsTable>(model);
    REQUIRE((regions.id == 300 && regions.regionGroupId == 200));
    REQUIRE((EulerInitialRegionsTable::INFO.category == CardCategory::EulerInitialization &&
             EulerInitialRegionsTable::INFO.idSpace == base::IdSpace::PerCardType));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // The group is a set of cards: every TICREG that has the TSID. The PEULER1 names the TICEUL1 (it is a property: the
    // SID is no longer a plain number).
    REQUIRE(seenBy<EulerInitialRegionsTable>(model) ==
            std::vector<Seen>{{"regionGroupId", ReferenceTarget::EulerInitialRegions, 200}});
    REQUIRE(seenBy<EulerRegionPropTable>(model) ==
            std::vector<Seen>{{"initialRegionsId", ReferenceTarget::EulerInitialization, 300}});
    Model sid2 = readBulk("EulerRegionProp;100;;300;7\nEulerBodyForceSet;7;1\nEulerInitialRegions;300;200");
    REQUIRE(seenBy<EulerRegionPropTable>(sid2) ==
            std::vector<Seen>{{"initialRegionsId", ReferenceTarget::EulerInitialization, 300},
                              {"bodyForceSetId", ReferenceTarget::EulerBodyForce, 7}});

    Model noTable = readBulk("EulerRegionProp;100;;300");
    REQUIRE(unresolvedIn(noTable) == 1); // the TICEUL1 does not exist
    Model noGroup = readBulk("EulerRegionProp;100;;300\nEulerInitialRegions;300;200");
    REQUIRE(unresolvedIn(noGroup) == 1); // no TICREG has the TSID

    // Renumbering the TICEUL1 follows into the PEULER1, and the group into the TICREG cards that make it.
    Renumbering::apply(model, RenumberSpace::EulerInitialization, Renumbering::offset(1000));
    REQUIRE((cardAt<EulerInitialRegionsTable>(model).id == 1300 &&
             cardAt<EulerRegionPropTable>(model).initialRegionsId == 1300));
    Renumbering::apply(model, RenumberSpace::EulerInitialRegions, Renumbering::offset(50));
    REQUIRE(cardAt<EulerInitialRegionsTable>(model).regionGroupId == 250);
    for (std::uint32_t row = 0; row < 5; ++row)
        REQUIRE(cardAt<InitialConditionRegionTable>(model, row).groupId == 250);
    REQUIRE(unresolvedIn(model) == 0);

    REQUIRE_THROWS_AS(readBulk("EulerInitialRegions;300;200\nEulerInitialRegions;300;201"),
                      base::DeckError);                                                    // unique among the cards
    REQUIRE_NOTHROW(readBulk("EulerInitialRegions;300;200\nEulerInitialRegions;301;200")); // a group may serve two
    expectInvalid("EulerInitialRegions;0;200", "id");
    expectInvalid("EulerInitialRegions;300;0", "regionGroupId");
    expectInvalid("EulerInitialRegions;300", "regionGroupId"); // required
    expectInvalid("EulerInitialRegions;-300;200", "id");
}

TEST_CASE("a TICREG is filed under its first TSID and names a shape by its type, a material and a TICVAL",
          "[model][cards][ticreg]")
{
    Model model = readBulk(REGIONS_TEXT + SURFACE_ELEMENT);
    const InitialConditionRegionCard sphere = cardAt<InitialConditionRegionTable>(model, 0);
    REQUIRE(
        (sphere.id == 1 && sphere.groupId == 200 && sphere.type == EulerRegionType::Sphere && sphere.shapeId == 400));
    REQUIRE((sphere.matId == 25 && sphere.valuesId == 3 && sphere.level == 4.0));
    REQUIRE((cardAt<InitialConditionRegionTable>(model, 1).type == EulerRegionType::Surface &&
             cardAt<InitialConditionRegionTable>(model, 1).shapeId == 401));
    const InitialConditionRegionCard box = cardAt<InitialConditionRegionTable>(model, 2);
    REQUIRE((box.type == EulerRegionType::Box && box.shapeId == 402));
    REQUIRE((box.matId == 0 && box.valuesId == 0 && box.level == 0.0)); // the defaults
    REQUIRE(cardAt<InitialConditionRegionTable>(model, 3).type == EulerRegionType::Element);
    const InitialConditionRegionCard cylinder = cardAt<InitialConditionRegionTable>(model, 4);
    REQUIRE((cylinder.type == EulerRegionType::Cylinder && cylinder.matId == 0 && cylinder.valuesId == 3));
    REQUIRE(cylinder.level == -1.5);
    // The cards of a group share the TSID, which is the ID of the table (IdSpace::Group); the TRID is a number of the
    // card.
    const InitialConditionRegionTable& table = *model.findTable<InitialConditionRegionTable>();
    REQUIRE((table.idAtRow(0) == 200 && table.idAtRow(4) == 200));
    REQUIRE((InitialConditionRegionTable::INFO.category == CardCategory::EulerInitialRegions &&
             InitialConditionRegionTable::INFO.idSpace == base::IdSpace::Group));
    REQUIRE_NOTHROW(readBulk(
        "InitialConditionRegion;1;200;Sphere;400\nInitialConditionRegion;1;200;Surface;401")); // even the same TRID
    requireRoundTrip(model);

    // What the VID names depends on the TYPE, as for an EULFREG; a blank MID and a blank TSID are not visited.
    REQUIRE(seenBy<InitialConditionRegionTable>(model, 0) ==
            std::vector<Seen>{{"shapeId", ReferenceTarget::EulerRegionShape, 400},
                              {"matId", ReferenceTarget::Material, 25},
                              {"valuesId", ReferenceTarget::InitialValueSet, 3}});
    REQUIRE(seenBy<InitialConditionRegionTable>(model, 1).front() ==
            Seen{"shapeId", ReferenceTarget::EulerRegionShape, 401});
    REQUIRE(seenBy<InitialConditionRegionTable>(model, 2) ==
            std::vector<Seen>{{"shapeId", ReferenceTarget::ContactSurface, 402}});
    REQUIRE(seenBy<InitialConditionRegionTable>(model, 3) == std::vector<Seen>{{"shapeId", ReferenceTarget::Set, 403}});
    REQUIRE(seenBy<InitialConditionRegionTable>(model, 4) ==
            std::vector<Seen>{{"shapeId", ReferenceTarget::EulerRegionShape, 404},
                              {"valuesId", ReferenceTarget::InitialValueSet, 3}});
    REQUIRE(unresolvedIn(model) == 0);

    // The shapes, the box, the elements, the material and the TICVAL are renumbered with the regions that name them.
    Renumbering::apply(model, RenumberSpace::EulerRegionShape, Renumbering::offset(100));
    REQUIRE((cardAt<SphereShapeTable>(model).id == 500 && cardAt<InitialSurfaceVolumeTable>(model).id == 501));
    REQUIRE((cardAt<InitialConditionRegionTable>(model, 0).shapeId == 500 &&
             cardAt<InitialConditionRegionTable>(model, 1).shapeId == 501));
    REQUIRE(
        (cardAt<InitialConditionRegionTable>(model, 4).shapeId == 504 && cardAt<CylinderShapeTable>(model).id == 504));
    REQUIRE((cardAt<InitialConditionRegionTable>(model, 2).shapeId == 402 &&
             cardAt<InitialConditionRegionTable>(model, 3).shapeId == 403));
    Renumbering::apply(model, RenumberSpace::ContactSurface, Renumbering::offset(1000));
    REQUIRE((cardAt<InitialConditionRegionTable>(model, 2).shapeId == 1402 &&
             cardAt<InitialSurfaceVolumeTable>(model).surfaceId == 1060));
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(10));
    REQUIRE(cardAt<InitialConditionRegionTable>(model, 3).shapeId == 413);
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(5));
    REQUIRE((cardAt<InitialConditionRegionTable>(model, 0).matId == 30 &&
             cardAt<InitialConditionRegionTable>(model, 1).matId == 30));
    Renumbering::apply(model, RenumberSpace::InitialValueSet, Renumbering::offset(7));
    REQUIRE((cardAt<InitialConditionValuesTable>(model).id == 10 &&
             cardAt<InitialConditionRegionTable>(model, 0).valuesId == 10));
    REQUIRE((cardAt<InitialConditionRegionTable>(model, 4).valuesId == 10 &&
             cardAt<InitialConditionRegionTable>(model, 2).valuesId == 0));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // Deleting the TICVAL leaves the three regions that named it with an unresolved reference.
    model.remove(cardsOfSet(model, CardCategory::InitialValueSet, 10).front());
    REQUIRE(unresolvedIn(model) == 3);

    expectInvalid("InitialConditionRegion;0;200;Sphere;400", "id");
    expectInvalid("InitialConditionRegion;1;0;Sphere;400", "groupId");
    expectInvalid("InitialConditionRegion;1;200;;400", "shapeType"); // required
    expectInvalid("InitialConditionRegion;1;200;TORUS;400", "shapeType");
    expectInvalid("InitialConditionRegion;1;200;CYLINDR;400", "shapeType"); // the word is CYLINDER
    expectInvalid("InitialConditionRegion;1;200;Sphere", "shapeId");
    expectInvalid("InitialConditionRegion;1;200;Sphere;0", "shapeId");
    expectInvalid("InitialConditionRegion;1;200;Sphere;400;-1", "matId");
    expectInvalid("InitialConditionRegion;1;200;Sphere;400;;-1", "valuesId");
    expectInvalid("InitialConditionRegion;1;200;Sphere;400;;;abc", "level");
}

TEST_CASE("a TICVAL gives numbers, or the TABLED1 of the radial profiles, and the method decides which",
          "[model][cards][ticval]")
{
    Model model = readBulk(
        "InitialConditionValues;3;;Density;1180.;VelocityY;12.;SpecificInternalEnergy;3.25;VelocityX;6.5\n"
        "InitialConditionValues;4;Normal;SpecificInternalEnergy;2.5E+5\n"
        "InitialConditionValues;5;Radial;SpecificInternalEnergy;31;Density;32;RadialVelocity;33;CenterX;0.5;CenterY;-0."
        "5;CenterZ;0.;Decay;2.\n"
        "TableDynamic1;31" +
        TABLE_TEXT + "\nTableDynamic1;32" + TABLE_TEXT + "\nTableDynamic1;33" + TABLE_TEXT);
    const InitialConditionValuesCard example = cardAt<InitialConditionValuesTable>(model, 0);
    REQUIRE((example.id == 3 && example.method == InitialConditionValuesMethod::Normal)); // METHOD left blank
    REQUIRE(example.variables ==
            std::vector<InitialVariable>{
                {"Density", 1180.0}, {"VelocityY", 12.0}, {"SpecificInternalEnergy", 3.25}, {"VelocityX", 6.5}});
    const InitialConditionValuesCard normal = cardAt<InitialConditionValuesTable>(model, 1);
    REQUIRE(normal.method == InitialConditionValuesMethod::Normal);
    REQUIRE(normal.variables == std::vector<InitialVariable>{{"SpecificInternalEnergy", 2.5E5}});
    const InitialConditionValuesCard radial = cardAt<InitialConditionValuesTable>(model, 2);
    REQUIRE((radial.id == 5 && radial.method == InitialConditionValuesMethod::Radial && radial.variables.size() == 7));
    // The profiles of SIE, DENSITY and R-VEL are TABLED1 numbers, the centre and the decay are numbers.
    REQUIRE((radial.variables[0] == InitialVariable{"SpecificInternalEnergy", 31.0} &&
             radial.variables[2] == InitialVariable{"RadialVelocity", 33.0}));
    REQUIRE((radial.variables[3] == InitialVariable{"CenterX", 0.5} &&
             radial.variables[6] == InitialVariable{"Decay", 2.0}));
    REQUIRE((holdsTable(InitialValueMode::RadialProfiles, "SpecificInternalEnergy") &&
             !holdsTable(InitialValueMode::RadialProfiles, "CenterX")));
    REQUIRE(!holdsTable(InitialValueMode::Numbers, "SpecificInternalEnergy"));
    REQUIRE((InitialConditionValuesTable::INFO.category == CardCategory::InitialValueSet &&
             InitialConditionValuesTable::INFO.idSpace == base::IdSpace::PerCardType));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // A radial table value is written as an integer in field 4, 6, 8 and the centre as a real; METHOD is blank for
    // NORMAL.
    const std::vector<std::string> fields = writtenFields<InitialConditionValuesFormat>(radial);
    REQUIRE((fields[0] == "5" && fields[1] == "Radial" && fields[2] == "SpecificInternalEnergy" && fields[3] == "31"));
    REQUIRE((fields[6] == "RadialVelocity" && fields[7] == "33" && fields[8] == "CenterX" && fields[9] == "0.5"));
    REQUIRE(writtenFields<InitialConditionValuesFormat>(example)[1].empty());

    // Only the radial profiles name a table.
    REQUIRE(seenBy<InitialConditionValuesTable>(model, 0).empty());
    REQUIRE(seenBy<InitialConditionValuesTable>(model, 2) ==
            std::vector<Seen>{{"variableValue", ReferenceTarget::DynamicTable, 31},
                              {"variableValue", ReferenceTarget::DynamicTable, 32},
                              {"variableValue", ReferenceTarget::DynamicTable, 33}});
    Model missing = readBulk("InitialConditionValues;5;Radial;SpecificInternalEnergy;31;Density;32;RadialVelocity;33");
    REQUIRE(unresolvedIn(missing) == 3);
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(100));
    REQUIRE((cardAt<InitialConditionValuesTable>(model, 2).variables[0].value == 131.0 &&
             cardAt<InitialConditionValuesTable>(model, 2).variables[2].value == 133.0));
    REQUIRE(cardAt<InitialConditionValuesTable>(model, 2).variables[3].value == 0.5); // the centre is no table
    REQUIRE(cardAt<InitialConditionValuesTable>(model, 0).variables[0].value ==
            1180.0); // nor is a value of a normal list
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::InitialValueSet, Renumbering::offset(10));
    REQUIRE((cardAt<InitialConditionValuesTable>(model, 0).id == 13 &&
             cardAt<InitialConditionValuesTable>(model, 2).id == 15));

    // The method is part of the values: a list of numbers that are not whole is no radial list (what editing checks).
    InitialConditionValuesCard edited = cardAt<InitialConditionValuesTable>(model, 0);
    edited.method = InitialConditionValuesMethod::Radial;
    REQUIRE(problemField<InitialConditionValuesFormat>(edited) == "variableValue"); // 3.25 is no table
    edited.variables = {InitialVariable{"SpecificInternalEnergy", 31.0}};
    REQUIRE(problemField<InitialConditionValuesFormat>(edited) == "none");
    edited.method = InitialConditionValuesMethod::Normal;
    REQUIRE(problemField<InitialConditionValuesFormat>(edited) == "none");

    expectInvalid("InitialConditionValues;0;;Density;1.", "id");
    expectInvalid("InitialConditionValues;3;SPHERICAL;Density;1.", "method");
    expectInvalid("InitialConditionValues;3", "variableName"); // at least one variable
    expectInvalid("InitialConditionValues;3;;Density", "variableValue");
    expectInvalid("InitialConditionValues;3;;Density;abc", "variableValue");
    expectInvalid("InitialConditionValues;3;;;1.", "variableName");
    expectInvalid("InitialConditionValues;3;;Density;100.;;;SpecificInternalEnergy;1.",
                  "variableName"); // a blank field ends the list
    expectInvalid("InitialConditionValues;3;Radial;SpecificInternalEnergy;1.5",
                  "variableValue"); // a table is a whole number
    expectInvalid("InitialConditionValues;3;Radial;SpecificInternalEnergy;0", "variableValue");
    expectInvalid("InitialConditionValues;3;Radial;CenterX;abc", "variableValue");
    oor::testing::requireModelFileRoundTrip(readBulk("InitialConditionValues;3;;SYMBOLIC1;1."));
    REQUIRE_THROWS_AS(
        readBulk("InitialConditionValues;3;;Density;1.\nInitialConditionValues;3;;SpecificInternalEnergy;1."),
        base::DeckError); // unique among the cards
}

// ---------------------------------------------------------------------------------------------------------------------
// GBAGCOU
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a GBAGCOU switches a COUPLE to a GBAG after a time, with a percentage", "[model][cards][gbagcou]")
{
    Model model = readBulk(
        "GasBagSwitch;1;100;101;0.;8\nGasBagSwitch;2;100\nGasBagSwitch;3;100;101;0.02;12.5\n"
        "EulerLagrangeCoupling;100;10\nGasBag;101;10;;;;;" +
        GAS_BAG_REST + "\nContactElemSurface;10;1" + SURFACE_ELEMENT);
    const GasBagSwitchCard example = cardAt<GasBagSwitchTable>(model, 0);
    REQUIRE((example.id == 1 && example.couplingId == 100 && example.gasBagId == 101 && example.startTime == 0.0 &&
             example.validityPercent == 8.0));
    const GasBagSwitchCard bare = cardAt<GasBagSwitchTable>(model, 1);
    REQUIRE((bare.id == 2 && bare.couplingId == 100 && bare.gasBagId == 0 && bare.startTime == 0.0 &&
             bare.validityPercent == 0.0)); // the defaults
    const GasBagSwitchCard full = cardAt<GasBagSwitchTable>(model, 2);
    REQUIRE((full.startTime == 0.02 && full.validityPercent == 12.5));
    REQUIRE((GasBagSwitchTable::INFO.category == CardCategory::Other &&
             GasBagSwitchTable::INFO.idSpace == base::IdSpace::PerCardType));
    REQUIRE_THROWS_AS(readBulk("GasBagSwitch;1;100\nGasBagSwitch;1;101"), base::DeckError); // unique among the cards
    requireRoundTrip(model);

    // The defaults are blanks; the COUPLE and the GBAG are fields 2 and 3.
    const std::vector<std::string> fields = writtenFields<GasBagSwitchFormat>(bare);
    REQUIRE(fields.size() == 5); // GID, TSTART and PERCENT are blank at their defaults
    REQUIRE((fields[0] == "2" && fields[1] == "100"));
    REQUIRE(writtenFields<GasBagSwitchFormat>(full).size() == 5);

    REQUIRE(seenBy<GasBagSwitchTable>(model, 0) == std::vector<Seen>{{"couplingId", ReferenceTarget::Coupling, 100},
                                                                     {"gasBagId", ReferenceTarget::GasBag, 101}});
    REQUIRE(seenBy<GasBagSwitchTable>(model, 1) == std::vector<Seen>{{"couplingId", ReferenceTarget::Coupling, 100}});
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Coupling, Renumbering::offset(1000));
    Renumbering::apply(model, RenumberSpace::GasBag, Renumbering::offset(2000));
    REQUIRE((cardAt<GasBagSwitchTable>(model, 0).couplingId == 1100 &&
             cardAt<EulerLagrangeCouplingTable>(model).id == 1100));
    REQUIRE((cardAt<GasBagSwitchTable>(model, 0).gasBagId == 2101 && cardAt<GasBagTable>(model).id == 2101));
    REQUIRE(cardAt<GasBagSwitchTable>(model, 1).gasBagId == 0); // a blank stays blank
    REQUIRE(unresolvedIn(model) == 0);

    Model dangling = readBulk("GasBagSwitch;1;100;101");
    REQUIRE(unresolvedIn(dangling) == 2);

    expectInvalid("GasBagSwitch;0;100", "id");
    expectInvalid("GasBagSwitch;1;0", "couplingId");
    expectInvalid("GasBagSwitch;1", "couplingId"); // required
    expectInvalid("GasBagSwitch;1;100;-1", "gasBagId");
    expectInvalid("GasBagSwitch;1;100;101;-1.", "startTime");
    expectInvalid("GasBagSwitch;1;100;101;0.;-5.", "validityPercent");
    expectInvalid("GasBagSwitch;1;100;101;abc", "startTime");
}

// ---------------------------------------------------------------------------------------------------------------------
// HEATLOS, HTRCONV, HTRRAD, and the COUPLE and GBAG that name a set of HEATLOS cards
// ---------------------------------------------------------------------------------------------------------------------

namespace
{

// A coupling and a gas bag that name the set 76 of heat transfer cards (the HTRID is field 10 of a COUPLE and field 7
// of a GBAG), the cards of the set and the heat transfer cards they name, with the surfaces and the table.
const std::string HEAT_TEXT =
    "EulerLagrangeCoupling;1;10;;;;;;;;76\nGasBag;101;10;;;;;76" + GAS_BAG_REST +
    "\nGasBagHeatTransfer;101;76;;GasBagConvection;2;Table;14\nGasBagHeatTransfer;102;76;60;GasBagRadiation;3\n"
    "GasBagHeatTransfer;103;79;;GasBagConvection;2;Constant;0.5\n"
    "GasBagConvection;2;;14;293.\nGasBagRadiation;3;0.22;;293.;5.6704E-8\nTableDynamic1;14" +
    TABLE_TEXT + "\nContactElemSurface;10;1\nContactElemSurface;60;1";

} // namespace

TEST_CASE("a HEATLOS is filed under its HTRID and names a surface, an HTRCONV or an HTRRAD and an area coefficient",
          "[model][cards][heatlos]")
{
    Model model = readBulk(HEAT_TEXT + SURFACE_ELEMENT);
    const GasBagHeatTransferCard convection = cardAt<GasBagHeatTransferTable>(model, 0);
    REQUIRE((convection.id == 101 && convection.setId == 76 && convection.subsurfaceId == 0));
    REQUIRE((convection.modelKind == HeatTransferType::Convection && convection.modelId == 2));
    REQUIRE((convection.coefficient.source == ValueSource::Table && convection.coefficient.table == 14));
    const GasBagHeatTransferCard radiation = cardAt<GasBagHeatTransferTable>(model, 1);
    REQUIRE(
        (radiation.subsurfaceId == 60 && radiation.modelKind == HeatTransferType::Radiation && radiation.modelId == 3));
    // A blank COEFF is CONSTANT and a blank COEFFV is 1.0, the whole area.
    REQUIRE((radiation.coefficient.source == ValueSource::Constant && radiation.coefficient.constant == 1.0));
    const GasBagHeatTransferCard constant = cardAt<GasBagHeatTransferTable>(model, 2);
    REQUIRE((constant.setId == 79 && constant.coefficient.source == ValueSource::Constant &&
             constant.coefficient.constant == 0.5));
    // The cards of a set share the HTRID, the ID of the table (IdSpace::Group); the HID is a number of the card.
    const GasBagHeatTransferTable& heatlos = *model.findTable<GasBagHeatTransferTable>();
    REQUIRE((heatlos.idAtRow(0) == 76 && heatlos.idAtRow(1) == 76 && heatlos.idAtRow(2) == 79));
    REQUIRE((GasBagHeatTransferTable::INFO.category == CardCategory::HeatLossSet &&
             GasBagHeatTransferTable::INFO.idSpace == base::IdSpace::Group));
    REQUIRE_NOTHROW(readBulk(
        "GasBagHeatTransfer;101;76;;GasBagConvection;2\nGasBagHeatTransfer;101;76;;GasBagConvection;2")); // the HID is
                                                                                                          // not indexed
    requireRoundTrip(model);

    // The defaults are blanks: the whole surface, CONSTANT and 1.0.
    const std::vector<std::string> fields = writtenFields<GasBagHeatTransferFormat>(radiation);
    REQUIRE(fields.size() == 7); // COEFF and COEFFV are blank at their defaults
    REQUIRE((fields[0] == "102" && fields[1] == "76" && fields[2] == "60" && fields[3] == "GasBagRadiation" &&
             fields[4] == "3"));
    const std::vector<std::string> tabled = writtenFields<GasBagHeatTransferFormat>(convection);
    REQUIRE((tabled.size() == 7 && tabled[2].empty() && tabled[5] == "Table" && tabled[6] == "14"));

    // The references: the surface, the heat transfer card (whichever kind has the number) and the table.
    REQUIRE(seenBy<GasBagHeatTransferTable>(model, 0) ==
            std::vector<Seen>{{"modelId", ReferenceTarget::HeatTransferModel, 2},
                              {"coefficient", ReferenceTarget::DynamicTable, 14}});
    REQUIRE(seenBy<GasBagHeatTransferTable>(model, 1) ==
            std::vector<Seen>{{"subsurfaceId", ReferenceTarget::ContactSurface, 60},
                              {"modelId", ReferenceTarget::HeatTransferModel, 3}});
    REQUIRE(seenBy<GasBagHeatTransferTable>(model, 2) ==
            std::vector<Seen>{{"modelId", ReferenceTarget::HeatTransferModel, 2}});
    // The COUPLE and the GBAG name the set.
    REQUIRE(seenBy<EulerLagrangeCouplingTable>(model) ==
            std::vector<Seen>{{"surfaceId", ReferenceTarget::ContactSurface, 10},
                              {"heatTransferId", ReferenceTarget::HeatLossSet, 76}});
    REQUIRE(seenBy<GasBagTable>(model) == std::vector<Seen>{{"surfaceId", ReferenceTarget::ContactSurface, 10},
                                                            {"heatTransferId", ReferenceTarget::HeatLossSet, 76}});
    REQUIRE(unresolvedIn(model) == 0);
    // A set that no HEATLOS has, and the heat transfer card of another kind: the number is what is looked up.
    Model noSet = readBulk("EulerLagrangeCoupling;1;10;;;;;;;;76\nGasBag;101;10;;;;;76" + GAS_BAG_REST +
                           "\nContactElemSurface;10;1" + SURFACE_ELEMENT);
    REQUIRE(unresolvedIn(noSet) == 2);
    Model otherKind = readBulk("GasBagHeatTransfer;1;5;;GasBagRadiation;2\nGasBagConvection;2;5.;;300.");
    REQUIRE(unresolvedIn(otherKind) == 0); // the check explicit.gas-bag-heat-transfer-type tells the mistake

    Renumbering::apply(model, RenumberSpace::HeatLossSet, Renumbering::offset(1000));
    REQUIRE((cardAt<GasBagHeatTransferTable>(model, 0).setId == 1076 &&
             cardAt<GasBagHeatTransferTable>(model, 1).setId == 1076));
    REQUIRE((cardAt<GasBagHeatTransferTable>(model, 2).setId == 1079 &&
             cardAt<GasBagHeatTransferTable>(model, 0).id == 101));
    REQUIRE((cardAt<EulerLagrangeCouplingTable>(model).heatTransferId == 1076 &&
             cardAt<GasBagTable>(model).heatTransferId == 1076));
    Renumbering::apply(model, RenumberSpace::HeatTransferModel, Renumbering::offset(10));
    REQUIRE((cardAt<GasBagConvectionTable>(model).id == 12 && cardAt<GasBagRadiationTable>(model).id == 13));
    REQUIRE((cardAt<GasBagHeatTransferTable>(model, 0).modelId == 12 &&
             cardAt<GasBagHeatTransferTable>(model, 1).modelId == 13));
    REQUIRE(cardAt<GasBagHeatTransferTable>(model, 2).modelId == 12);
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(100));
    REQUIRE((cardAt<GasBagHeatTransferTable>(model, 0).coefficient.table == 114 &&
             cardAt<GasBagConvectionTable>(model).coefficientTableId == 114));
    Renumbering::apply(model, RenumberSpace::ContactSurface, Renumbering::offset(500));
    REQUIRE((cardAt<GasBagHeatTransferTable>(model, 1).subsurfaceId == 560 &&
             cardAt<EulerLagrangeCouplingTable>(model).surfaceId == 510));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // Deleting a HEATLOS that is not the last of its set keeps the set; deleting the last one leaves the COUPLE and the
    // GBAG naming nothing.
    model.remove(cardsOfSet(model, CardCategory::HeatLossSet, 1076).front());
    REQUIRE(unresolvedIn(model) == 0);
    model.remove(cardsOfSet(model, CardCategory::HeatLossSet, 1076).front());
    REQUIRE(unresolvedIn(model) == 2);
}

TEST_CASE("a HEATLOS refuses a value that is not in its layout", "[model][cards][heatlos]")
{
    expectInvalid("GasBagHeatTransfer;0;76;;GasBagConvection;2", "id");
    expectInvalid("GasBagHeatTransfer;1;0;;GasBagConvection;2", "setId");
    expectInvalid("GasBagHeatTransfer;1;76;-1;GasBagConvection;2", "subsurfaceId");
    expectInvalid("GasBagHeatTransfer;1;76;;;2", "modelKind"); // required
    expectInvalid("GasBagHeatTransfer;1;76;;HTRFOO;2", "modelKind");
    expectInvalid("GasBagHeatTransfer;1;76;;GasBagConvection", "modelId"); // required
    expectInvalid("GasBagHeatTransfer;1;76;;GasBagConvection;0", "modelId");
    expectInvalid("GasBagHeatTransfer;1;76;;GasBagConvection;2;LINEAR;1.", "coefficientSource");
    expectInvalid("GasBagHeatTransfer;1;76;;GasBagConvection;2;Element",
                  "coefficientSource"); // the format has two choices
    expectInvalid("GasBagHeatTransfer;1;76;;GasBagConvection;2;Table", "coefficient");
    expectInvalid("GasBagHeatTransfer;1;76;;GasBagConvection;2;Table;0", "coefficient");
    expectInvalid("GasBagHeatTransfer;1;76;;GasBagConvection;2;;1.5", "coefficient"); // a fraction of the area
    expectInvalid("GasBagHeatTransfer;1;76;;GasBagConvection;2;;-0.5", "coefficient");
    // Both ends of the range are allowed: no heat transfer through the subsurface, and all of its area.
    const Model ends =
        readBulk("GasBagHeatTransfer;1;76;;GasBagConvection;2;;0.\nGasBagHeatTransfer;2;76;;GasBagConvection;2;;1.");
    REQUIRE((cardAt<GasBagHeatTransferTable>(ends, 0).coefficient.constant == 0.0 &&
             cardAt<GasBagHeatTransferTable>(ends, 1).coefficient.constant == 1.0));
    GasBagHeatTransferCard card;
    card.id = 1;
    card.setId = 76;
    card.modelId = 2;
    REQUIRE(problemField<GasBagHeatTransferFormat>(card) == "none");
    card.coefficient = SourcedValue{ValueSource::Element, 0.0, 0};
    REQUIRE(problemField<GasBagHeatTransferFormat>(card) ==
            "coefficientSource"); // ELEMENT is not one of the two sources
}

TEST_CASE("an HTRCONV gives a constant coefficient or the TABLED1 of one, and the environmental temperature",
          "[model][cards][htrconv]")
{
    Model model = readBulk(
        "GasBagConvection;8;;41;288.5\nGasBagConvection;9;12.5;;300.\nGasBagConvection;10;0.;;293.15\n"
        "TableDynamic1;41" +
        TABLE_TEXT);
    const GasBagConvectionCard table = cardAt<GasBagConvectionTable>(model, 0);
    REQUIRE((table.id == 8 && !table.coefficient.has_value() && table.coefficientTableId == 41 &&
             table.environmentTemperature == 288.5)); // a table and no constant
    const GasBagConvectionCard constant = cardAt<GasBagConvectionTable>(model, 1);
    REQUIRE((constant.coefficient.has_value() && *constant.coefficient == 12.5 && constant.coefficientTableId == 0 &&
             constant.environmentTemperature == 300.0));
    // A constant of zero is a coefficient (the format says "Real >= 0"), which is not a blank.
    REQUIRE((cardAt<GasBagConvectionTable>(model, 2).coefficient.has_value() &&
             *cardAt<GasBagConvectionTable>(model, 2).coefficient == 0.0));
    REQUIRE((GasBagConvectionTable::INFO.category == CardCategory::HeatTransferModel &&
             GasBagConvectionTable::INFO.idSpace == base::IdSpace::PerCardType));
    REQUIRE_THROWS_AS(readBulk("GasBagConvection;8;5.;;300.\nGasBagConvection;8;6.;;300."),
                      base::DeckError); // unique among the cards
    requireRoundTrip(model);

    const std::vector<std::string> tabled = writtenFields<GasBagConvectionFormat>(table);
    REQUIRE((tabled.size() == 4 && tabled[0] == "8" && tabled[1].empty() && tabled[2] == "41" && !tabled[3].empty()));
    const std::vector<std::string> constants = writtenFields<GasBagConvectionFormat>(constant);
    REQUIRE((constants[1] == "12.5" && constants[2].empty()));
    REQUIRE(!writtenFields<GasBagConvectionFormat>(cardAt<GasBagConvectionTable>(model, 2))[1]
                 .empty()); // a zero constant is written

    REQUIRE(seenBy<GasBagConvectionTable>(model, 0) ==
            std::vector<Seen>{{"coefficientTableId", ReferenceTarget::DynamicTable, 41}});
    REQUIRE(seenBy<GasBagConvectionTable>(model, 1).empty());
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(100));
    REQUIRE((cardAt<GasBagConvectionTable>(model, 0).coefficientTableId == 141 &&
             cardAt<GasBagConvectionTable>(model, 1).coefficientTableId == 0));
    Renumbering::apply(model, RenumberSpace::HeatTransferModel, Renumbering::offset(1000));
    REQUIRE((cardAt<GasBagConvectionTable>(model, 0).id == 1008 && cardAt<GasBagConvectionTable>(model, 2).id == 1010));
    // An HTRCONV and an HTRRAD each have a number space: the number 2 may be both, and they move together.
    Model both = readBulk("GasBagConvection;2;5.;;300.\nGasBagRadiation;2;0.1;;300.;5.7E-8");
    Renumbering::apply(both, RenumberSpace::HeatTransferModel, Renumbering::offset(10));
    REQUIRE((cardAt<GasBagConvectionTable>(both).id == 12 && cardAt<GasBagRadiationTable>(both).id == 12));

    expectInvalid("GasBagConvection;0;5.;;300.", "id");
    expectInvalid("GasBagConvection;8;-1.;;293.", "coefficient");
    expectInvalid("GasBagConvection;8;abc;;293.", "coefficient");
    expectInvalid("GasBagConvection;8;;-1;293.", "coefficientTableId");
    expectInvalid("GasBagConvection;8;;;293.", "coefficient");            // one of the two
    expectInvalid("GasBagConvection;8;5.;14;293.", "coefficientTableId"); // not both
    expectInvalid("GasBagConvection;8;5.", "environmentTemperature");     // required
    expectInvalid("GasBagConvection;8;5.;;0.", "environmentTemperature");
    expectInvalid("GasBagConvection;8;5.;;-3.", "environmentTemperature");
}

TEST_CASE("an HTRRAD gives a constant emissivity or the TABLED1 of one, the temperature and the Boltzmann constant",
          "[model][cards][htrrad]")
{
    Model model = readBulk(
        "GasBagRadiation;2;0.22;;293.;5.6704E-8\nGasBagRadiation;3;;14;310.;5.67E-8\nGasBagRadiation;4;0.;;293.;5.67E-"
        "8\n"
        "TableDynamic1;14" +
        TABLE_TEXT);
    const GasBagRadiationCard example = cardAt<GasBagRadiationTable>(model, 0);
    REQUIRE((example.id == 2 && example.emissivity.has_value() && *example.emissivity == 0.22 &&
             example.emissivityTableId == 0));
    REQUIRE((example.environmentTemperature == 293.0 && example.stefanBoltzmann == 5.6704E-8));
    const GasBagRadiationCard table = cardAt<GasBagRadiationTable>(model, 1);
    REQUIRE((!table.emissivity.has_value() && table.emissivityTableId == 14 && table.environmentTemperature == 310.0));
    REQUIRE((cardAt<GasBagRadiationTable>(model, 2).emissivity.has_value() &&
             *cardAt<GasBagRadiationTable>(model, 2).emissivity == 0.0));
    REQUIRE((GasBagRadiationTable::INFO.category == CardCategory::HeatTransferModel &&
             GasBagRadiationTable::INFO.idSpace == base::IdSpace::PerCardType));
    REQUIRE_THROWS_AS(readBulk("GasBagRadiation;2;0.1;;300.;5.7E-8\nGasBagRadiation;2;0.2;;300.;5.7E-8"),
                      base::DeckError);
    requireRoundTrip(model);

    const std::vector<std::string> fields = writtenFields<GasBagRadiationFormat>(table);
    REQUIRE((fields.size() == 5 && fields[0] == "3" && fields[1].empty() && fields[2] == "14" && !fields[3].empty()));
    REQUIRE(!fields[4].empty());

    REQUIRE(seenBy<GasBagRadiationTable>(model, 1) ==
            std::vector<Seen>{{"emissivityTableId", ReferenceTarget::DynamicTable, 14}});
    REQUIRE(seenBy<GasBagRadiationTable>(model, 0).empty());
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(100));
    REQUIRE((cardAt<GasBagRadiationTable>(model, 1).emissivityTableId == 114 &&
             cardAt<GasBagRadiationTable>(model, 0).emissivityTableId == 0));
    Renumbering::apply(model, RenumberSpace::HeatTransferModel, Renumbering::offset(1000));
    REQUIRE((cardAt<GasBagRadiationTable>(model, 0).id == 1002 && cardAt<GasBagRadiationTable>(model, 2).id == 1004));

    expectInvalid("GasBagRadiation;0;0.1;;300.;5.7E-8", "id");
    expectInvalid("GasBagRadiation;2;-0.1;;300.;5.7E-8", "emissivity");
    expectInvalid("GasBagRadiation;2;abc;;300.;5.7E-8", "emissivity");
    expectInvalid("GasBagRadiation;2;;-1;300.;5.7E-8", "emissivityTableId");
    expectInvalid("GasBagRadiation;2;;;300.;5.7E-8", "emissivity"); // one of the two
    expectInvalid("GasBagRadiation;2;0.1;14;300.;5.7E-8", "emissivityTableId");
    expectInvalid("GasBagRadiation;2;0.1;;;5.7E-8", "environmentTemperature"); // required
    expectInvalid("GasBagRadiation;2;0.1;;-300.;5.7E-8", "environmentTemperature");
    expectInvalid("GasBagRadiation;2;0.1;;300.", "stefanBoltzmann"); // required
    expectInvalid("GasBagRadiation;2;0.1;;300.;0.", "stefanBoltzmann");
}

TEST_CASE("a COUPLE and a GBAG name their HEATLOS set, and a PEULER1 its TICEUL1, with no ID left plain",
          "[model][cards][heatlos][ticeul1]")
{
    // The three existing cards whose plain IDs are references: the fields keep their positions, the check of the
    // values is the one it was, and the targets are the kinds the new cards define.
    const Model couple = readBulk("EulerLagrangeCoupling;1;10;;;;;;;;76");
    REQUIRE(cardAt<EulerLagrangeCouplingTable>(couple).heatTransferId == 76);
    const Model gbag = readBulk("GasBag;101;10;;;;;76" + GAS_BAG_REST);
    REQUIRE(cardAt<GasBagTable>(gbag).heatTransferId == 76);
    const Model peuler1 = readBulk("EulerRegionProp;100;;300");
    REQUIRE(cardAt<EulerRegionPropTable>(peuler1).initialRegionsId == 300);
    expectInvalid("EulerLagrangeCoupling;1;10;;;;;;;;-1", "heatTransferId");
    expectInvalid("GasBag;101;10;;;;;-1" + GAS_BAG_REST, "heatTransferId");
    expectInvalid("EulerRegionProp;100;;0", "initialRegionsId");
    // A blank HTRID names nothing.
    Model blank = readBulk("EulerLagrangeCoupling;1;10\nContactElemSurface;10;1" + SURFACE_ELEMENT);
    REQUIRE(unresolvedIn(blank) == 0);
    REQUIRE(seenBy<EulerLagrangeCouplingTable>(blank) ==
            std::vector<Seen>{{"surfaceId", ReferenceTarget::ContactSurface, 10}});
}
