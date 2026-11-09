// Checks of the free-body and load cards: FBODYLD and FBODYSB (an equilibrated free-body load case and its
// subsystem, both named), HYDROC and HYDROS (hydrostatic pressure loads, whose CID/GRD field is a coordinate system or
// minus a grid point), IMPGEOM (a geometric imperfection that an IMPCASE names), TMPSET and TTEMP (a group of grid
// points and the time-dependent temperature of the group) and TPADEFN (the set-up of a transfer path analysis). Per
// card: the fields in their format reference positions, the defaults, the references the card lists and what each one
// points to, the renumbering of the spaces the card owns and of the ones it names, the refused values and the round
// trip through the three formats. The generic tests of CardRoundTripTest.cpp read, write and copy every sample; this
// file covers what each card means.

#include "oor/model/cards/loads/HydrostaticSupport.hpp"
#include "oor/model/cards/loads/freeBodyLoad/FreeBodyLoadTable.hpp"
#include "oor/model/cards/loads/freeBodyLoadSubcase/FreeBodyLoadSubcaseTable.hpp"
#include "oor/model/cards/loads/hydrostaticCavityLoad/HydrostaticCavityLoadTable.hpp"
#include "oor/model/cards/loads/hydrostaticLoad/HydrostaticLoadTable.hpp"
#include "oor/model/cards/loads/imperfectionCase/ImperfectionCaseTable.hpp"
#include "oor/model/cards/loads/imperfectionGeometry/ImperfectionGeometryTable.hpp"
#include "oor/model/cards/loads/nodeTemperatureLoad/NodeTemperatureLoadTable.hpp"
#include "oor/model/cards/loads/temperatureTimeGroup/TemperatureTimeGroupTable.hpp"
#include "oor/model/cards/loads/timeTemperatureLoad/TimeTemperatureLoadTable.hpp"
#include "oor/model/cards/solution_control/transferPathSetup/TransferPathSetupTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <limits>
#include <string>
#include <string_view>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// One reference of a card as a visitor sees it: the field, what it points to and the ID.
struct Use
{
    std::string field;
    ReferenceTarget target;
    base::EntityId id;

    [[nodiscard]] bool operator==(const Use&) const noexcept = default;
};

using Uses = std::vector<Use>;

struct UseVisitor final : ReferenceVisitor
{
    Uses seen;
    base::EntityId visit(std::string_view field, ReferenceTarget target, base::EntityId id) override
    {
        seen.push_back(Use{std::string(field), target, id});
        return id;
    }
};

template <typename TableT>
Uses usesOf(Model& model, std::uint32_t row)
{
    UseVisitor visitor;
    model.table<TableT>().visitReferences(row, visitor);
    return visitor.seen;
}

// The problem of a check: the field it names.
template <typename ProblemT>
std::string fieldOf(const ProblemT& problem)
{
    return problem ? std::string(problem->field) : std::string("(no problem)");
}

// The references that name nothing in a model that was just read (the support function takes a model that stays).
std::size_t unresolvedIn(Model&& model)
{
    return testing::unresolvedIn(model);
}

// The grid points and the sets that the decks below name, so that a card that refers to them resolves.
const std::string POINTS_TEXT = "Node;1;;0.;0.;0.\nNode;2;;1.;0.;0.\nNode;10;;2.;0.;0.\nNode;11;;3.;0.;0.\n";

} // namespace

// ---------------------------------------------------------------------------------------------------------------------
// FBODYLD
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an FBODYLD is named and not numbered, keeps its names as written and has its label on a second line",
          "[model][cards][fbodyld]")
{
    Model model = readBulk(
        "FreeBodyLoad;FLAPLD;FLAPSB;LOADS OF THE INNER FLAP\nFreeBodyLoad;TailLd;TailSb\n"
        "FreeBodyLoad;L3;S3;Lab");
    REQUIRE((FreeBodyLoadTable::INFO.category == CardCategory::Other &&
             FreeBodyLoadTable::INFO.idSpace == base::IdSpace::None));
    const FreeBodyLoadCard sampleCard = cardAt<FreeBodyLoadTable>(model, 0);
    REQUIRE((sampleCard.name == "FLAPLD" && sampleCard.subsystemName == "FLAPSB" &&
             sampleCard.label == "LOADS OF THE INNER FLAP"));
    const FreeBodyLoadCard plain = cardAt<FreeBodyLoadTable>(model, 1);
    REQUIRE((plain.name == "TailLd" && plain.subsystemName == "TailSb")); // the case is kept
    REQUIRE(plain.label.empty());
    REQUIRE(cardAt<FreeBodyLoadTable>(model, 2).label == "Lab");
    REQUIRE(usesOf<FreeBodyLoadTable>(model, 0).empty()); // a subsystem is named, not numbered: nothing resolves it
    requireRoundTrip(model);

    // In the small format the card comes back as it was written, with the label cut into the fields of its line; in the
    // free format the label is the first field of the line.
}

TEST_CASE("free body loads preserve long labels, names and quoted text", "[model][cards][fbodyld]")
{
    const std::string longest = "ONE TWO THREE FOUR FIVE SIX SEVEN EIGHT NINE TEN ELEVEN TWELVE X";
    REQUIRE(longest.size() == 64);
    const Model model = readBulk("FreeBodyLoad;A;B;" + longest);
    REQUIRE(cardAt<FreeBodyLoadTable>(model).label == longest);
    requireRoundTrip(model);

    // The same card and a name, an edit that the Python layer would make.
    FreeBodyLoadCard card = cardAt<FreeBodyLoadTable>(model);
    REQUIRE_FALSE(FreeBodyLoadFormat::check(card).has_value());
    card.label += "Y";
    REQUIRE_FALSE(FreeBodyLoadFormat::check(card).has_value());
    card.label = "a,b";
    REQUIRE_FALSE(FreeBodyLoadFormat::check(card).has_value());
    card.label = " leading blank";
    REQUIRE_FALSE(FreeBodyLoadFormat::check(card).has_value());
    card.label = "trailing blank ";
    REQUIRE_FALSE(FreeBodyLoadFormat::check(card).has_value());
    card.label = "line\nbreak";
    REQUIRE(fieldOf(FreeBodyLoadFormat::check(card)) == "label");
    card.label.clear();
    REQUIRE_FALSE(FreeBodyLoadFormat::check(card).has_value()); // the label is optional
    card.subsystemName = "ABCDEFGHIJKLMNOPQ";                   // seventeen characters
    REQUIRE_FALSE(FreeBodyLoadFormat::check(card).has_value());
    card.subsystemName = "B";
    card.name = "has blank";
    REQUIRE_FALSE(FreeBodyLoadFormat::check(card).has_value());

    expectInvalid("FreeBodyLoad;;SB", "name");
    expectInvalid("FreeBodyLoad;NAME", "subsystemName"); // required
    oor::testing::requireModelFileRoundTrip(readModelText("FreeBodyLoad;ABCDEFGHIJKLMNOPQ;SB", true));
    oor::testing::requireModelFileRoundTrip(readModelText("FreeBodyLoad;NAME;ABCDEFGHIJKLMNOPQ", true));
    oor::testing::requireModelFileRoundTrip(readModelText("FreeBodyLoad;A;B;" + longest + "Y", true));
}

TEST_CASE("an FBODYLD is edited in place, deleted, and a repeated name is reported", "[model][cards][fbodyld]")
{
    Model model = readBulk("FreeBodyLoad;FLAPLD;FLAPSB\nFreeBodyLoad;TAILLD;TAILSB");
    FreeBodyLoadCard edited = cardAt<FreeBodyLoadTable>(model, 1);
    edited.subsystemName = "NEWSB";
    edited.label = "A new label";
    model.table<FreeBodyLoadTable>().store(1, edited);
    REQUIRE(cardAt<FreeBodyLoadTable>(model, 1) == edited);
    REQUIRE(cardAt<FreeBodyLoadTable>(model, 0).subsystemName == "FLAPSB"); // the other card is as it was
    requireRoundTrip(model);

    model.remove(CardLocation{typeIndexOf<FreeBodyLoadTable>(), 0});
    model.compact();
    REQUIRE(rowCount<FreeBodyLoadTable>(model) == 1);
    REQUIRE(cardAt<FreeBodyLoadTable>(model, 0).name == "TAILLD");

    // The exact name is unique; names that differ by case remain distinct.
    const Model twice = readModelText("[model]\nFreeBodyLoad;A;B\nFreeBodyLoad;A;C\n");
    REQUIRE(twice.diagnostics().hasErrors());
    REQUIRE(twice.diagnostics().firstError()->code == "deck.duplicate-free-body-load");
    const Model different = readModelText("[model]\nFreeBodyLoad;A;B\nFreeBodyLoad;a;B\n");
    REQUIRE_FALSE(different.diagnostics().hasErrors());
}

// ---------------------------------------------------------------------------------------------------------------------
// FBODYSB
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an FBODYSB names a set of grid points and a set of elements and leaves out types of force",
          "[model][cards][fbodysb]")
{
    Model model = readBulk(POINTS_TEXT +
                           "IdListSet1;1;10;11\n"
                           "IdListSet1;2;1;2\n"
                           "FreeBodyLoadSubcase;FLAP;2;1;SLP;INNER FLAP\n"
                           "FreeBodyLoadSubcase;Tail;2\n"
                           "FreeBodyLoadSubcase;Low;1;2;d");
    REQUIRE((FreeBodyLoadSubcaseTable::INFO.category == CardCategory::Other &&
             FreeBodyLoadSubcaseTable::INFO.idSpace == base::IdSpace::None));
    const FreeBodyLoadSubcaseCard sampleCard = cardAt<FreeBodyLoadSubcaseTable>(model, 0);
    REQUIRE((sampleCard.names == "FLAP" && sampleCard.nodeSetId == 2 && sampleCard.elementSetId == 1));
    REQUIRE((sampleCard.excludedForces == "SLP" && sampleCard.label == "INNER FLAP"));
    const FreeBodyLoadSubcaseCard bare = cardAt<FreeBodyLoadSubcaseTable>(model, 1);
    REQUIRE((bare.names == "Tail" && bare.nodeSetId == 2 &&
             bare.elementSetId == 0)); // a blank ELEMSET: no element contributes
    REQUIRE((bare.excludedForces.empty() && bare.label.empty()));
    REQUIRE(cardAt<FreeBodyLoadSubcaseTable>(model, 2).excludedForces ==
            "D"); // any case is read, the letters are kept in upper case
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // The two sets are Set references; a blank one is not visited.
    REQUIRE(usesOf<FreeBodyLoadSubcaseTable>(model, 0) ==
            Uses{{"nodeSetId", ReferenceTarget::Set, 2}, {"elementSetId", ReferenceTarget::Set, 1}});
    REQUIRE(usesOf<FreeBodyLoadSubcaseTable>(model, 1) == Uses{{"nodeSetId", ReferenceTarget::Set, 2}});
    REQUIRE(usesOf<FreeBodyLoadSubcaseTable>(model, 2) ==
            Uses{{"nodeSetId", ReferenceTarget::Set, 1}, {"elementSetId", ReferenceTarget::Set, 2}});

    // A renumbering of the sets follows into both fields.
    REQUIRE(Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(1000)) == 2);
    REQUIRE((cardAt<FreeBodyLoadSubcaseTable>(model, 0).nodeSetId == 1002 &&
             cardAt<FreeBodyLoadSubcaseTable>(model, 0).elementSetId == 1001));
    REQUIRE((cardAt<FreeBodyLoadSubcaseTable>(model, 1).nodeSetId == 1002 &&
             cardAt<FreeBodyLoadSubcaseTable>(model, 1).elementSetId == 0));
    REQUIRE((cardAt<FreeBodyLoadSubcaseTable>(model, 2).nodeSetId == 1001 &&
             cardAt<FreeBodyLoadSubcaseTable>(model, 2).elementSetId == 1002));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // A set that no card defines is an unresolved reference.
    REQUIRE(unresolvedIn(readBulk("FreeBodyLoadSubcase;A;7;8")) == 2);
    REQUIRE(unresolvedIn(readBulk("FreeBodyLoadSubcase;A;7")) == 1);
}

TEST_CASE("an FBODYSB writes its layout and refuses what the format does not allow", "[model][cards][fbodysb]")
{
    Model model = readBulk("FreeBodyLoadSubcase;FLAP;2;1;SLP;INNER FLAP\nFreeBodyLoadSubcase;Tail;2");

    // the blank ELEMSET and XFLAG are blank fields

    // The flag is a combination of the six letters.
    for (const std::string flag : {"S", "M", "A", "L", "P", "D", "SM", "ALP", "SMALPD", "DDDD"})
    {
        INFO(flag);
        const Model each = readBulk(std::string("FreeBodyLoadSubcase;X;1;;") + flag);
        REQUIRE(cardAt<FreeBodyLoadSubcaseTable>(each).excludedForces == flag);
        requireRoundTrip(each);
    }

    FreeBodyLoadSubcaseCard edited = cardAt<FreeBodyLoadSubcaseTable>(model, 0);
    edited.excludedForces = "SMALPDSMA"; // repeated allowed flags have no field-width limit
    REQUIRE_FALSE(FreeBodyLoadSubcaseFormat::check(edited).has_value());
    edited.excludedForces = "C"; // C is a letter of the MONPNT3 and not of this card
    REQUIRE(fieldOf(FreeBodyLoadSubcaseFormat::check(edited)) == "excludedForces");
    edited.excludedForces = "SM";
    edited.nodeSetId = 0;
    REQUIRE(fieldOf(FreeBodyLoadSubcaseFormat::check(edited)) == "nodeSetId");
    edited.nodeSetId = 1;
    edited.elementSetId = -3;
    REQUIRE(fieldOf(FreeBodyLoadSubcaseFormat::check(edited)) == "elementSetId");
    edited.elementSetId = 2;
    REQUIRE_FALSE(FreeBodyLoadSubcaseFormat::check(edited).has_value());
    model.table<FreeBodyLoadSubcaseTable>().store(0, edited);
    REQUIRE(cardAt<FreeBodyLoadSubcaseTable>(model, 0) == edited);

    model.remove(CardLocation{typeIndexOf<FreeBodyLoadSubcaseTable>(), 1});
    model.compact();
    REQUIRE(rowCount<FreeBodyLoadSubcaseTable>(model) == 1);

    expectInvalid("FreeBodyLoadSubcase;;1", "names");
    expectInvalid("FreeBodyLoadSubcase;WING", "nodeSetId"); // required
    expectInvalid("FreeBodyLoadSubcase;WING;0", "nodeSetId");
    expectInvalid("FreeBodyLoadSubcase;WING;-1", "nodeSetId");
    expectInvalid("FreeBodyLoadSubcase;WING;x", "nodeSetId");
    expectInvalid("FreeBodyLoadSubcase;WING;1;-1", "elementSetId");
    expectInvalid("FreeBodyLoadSubcase;WING;1;x", "elementSetId");
    expectInvalid("FreeBodyLoadSubcase;WING;1;;Q", "excludedForces");
    requireRoundTrip(readBulk("FreeBodyLoadSubcase;WING;1;;SMALPDSMA"));
    oor::testing::requireModelFileRoundTrip(readModelText("FreeBodyLoadSubcase;ABCDEFGHIJKLMNOPQ;1", true));

    // The format reference does not say that NAMES is unique, but an FBODYLD names a subsystem by it: a repeat is a
    // warning.
    const Model twice = readModelText("[model]\nFreeBodyLoadSubcase;A;1\nFreeBodyLoadSubcase;A;2\n");
    REQUIRE_FALSE(twice.diagnostics().hasErrors());
    REQUIRE(twice.diagnostics().diagnostics().size() == 1);
    REQUIRE(twice.diagnostics().diagnostics().front().code == "deck.duplicate-free-body-load-subcase");
    const Model caseDistinct = readBulk("FreeBodyLoadSubcase;A;1\nFreeBodyLoadSubcase;a;2");
    REQUIRE(caseDistinct.diagnostics().diagnostics().empty());
    requireRoundTrip(caseDistinct);
    oor::testing::requireModelFileRoundTrip(model);
}

// ---------------------------------------------------------------------------------------------------------------------
// HYDROC
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a HYDROC has the layout of the format, resolves its defaults and is a load of its set",
          "[model][cards][hydroc]")
{
    Model model = readBulk(
        "HydrostaticCavityLoad;2101;2201;3.5;12.5;;1;;2301;0.;7.5\n"
        "HydrostaticCavityLoad;100001;100011\n"
        "HydrostaticCavityLoad;100002;100012;101325.;9.81;3;1;1;-100031;0.5;2.5\n"
        "HydrostaticCavityLoad;2102;2201;0.;1.;0;1;;-2401;0.;25.");
    REQUIRE((HydrostaticCavityLoadTable::INFO.category == CardCategory::Load &&
             HydrostaticCavityLoadTable::INFO.idSpace == base::IdSpace::Group));
    const HydrostaticCavityLoadCard sampleCard = cardAt<HydrostaticCavityLoadTable>(model, 0);
    REQUIRE((sampleCard.id == 2101 && sampleCard.propId == 2201 && sampleCard.nominalPressure == 3.5 &&
             sampleCard.gravity == 12.5));
    REQUIRE((sampleCard.punchOption == 0 && sampleCard.punchPressureLoads == 1 && sampleCard.overrideOption == 0));
    REQUIRE((sampleCard.frameOrNode == 2301 && sampleCard.baseHeight == 0.0 && sampleCard.heightAbove == 7.5));
    const HydrostaticCavityLoadCard bare = cardAt<HydrostaticCavityLoadTable>(model, 1);
    REQUIRE((bare.id == 100001 && bare.propId == 100011 && bare.nominalPressure == 0.0 &&
             bare.gravity == 1.0)); // PNOM 0.0, G 1.0
    REQUIRE((bare.punchOption == 0 && bare.punchPressureLoads == 0 && bare.overrideOption == 0));
    REQUIRE((bare.frameOrNode == 0 && bare.baseHeight == 0.0 && bare.heightAbove == 0.0));
    const HydrostaticCavityLoadCard full = cardAt<HydrostaticCavityLoadTable>(model, 2);
    REQUIRE((full.nominalPressure == 101325.0 && full.gravity == 9.81 && full.punchOption == 3 &&
             full.punchPressureLoads == 1 && full.overrideOption == 1));
    REQUIRE((full.frameOrNode == -100031 && full.baseHeight == 0.5 && full.heightAbove == 2.5));
    // A card on one line: the fields of the second line follow the eight fields of the first.
    const HydrostaticCavityLoadCard oneLine = cardAt<HydrostaticCavityLoadTable>(model, 3);
    REQUIRE((oneLine.id == 2102 && oneLine.propId == 2201 && oneLine.punchPressureLoads == 1 &&
             oneLine.frameOrNode == -2401));
    REQUIRE((oneLine.baseHeight == 0.0 && oneLine.heightAbove == 25.0));
    requireRoundTrip(model);

    // The static defaults are blank fields.

    // A zero is not a blank: G may be zero (PNOM alone is applied) and stays zero.
    const Model zero = readBulk("HydrostaticCavityLoad;1;2;;0.");
    REQUIRE((cardAt<HydrostaticCavityLoadTable>(zero).gravity == 0.0 &&
             cardAt<HydrostaticCavityLoadTable>(zero).nominalPressure == 0.0));

    requireRoundTrip(zero);
}

TEST_CASE("a HYDROC names a property and a coordinate system, or a grid point when CID/GRD is negative",
          "[model][cards][hydroc]")
{
    Model model = readBulk(
        "IsoMat;1;2.1E+11;;0.3\nSolidProp;2201;1\nNode;2401;;0.;0.;0.\nRectFrame2;2301\n"
        "HydrostaticCavityLoad;2101;2201;;;;1;;2301\n"
        "HydrostaticCavityLoad;2102;2201;;;;;;-2401\n"
        "HydrostaticCavityLoad;2103;2201");
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(usesOf<HydrostaticCavityLoadTable>(model, 0) ==
            Uses{{"propId", ReferenceTarget::Property, 2201}, {"frameId/nodeAlt", ReferenceTarget::Coordinate, 2301}});
    // The sign is the switch: the grid point of a negative number is its absolute value.
    REQUIRE(usesOf<HydrostaticCavityLoadTable>(model, 1) ==
            Uses{{"propId", ReferenceTarget::Property, 2201}, {"frameId/nodeAlt", ReferenceTarget::Node, 2401}});
    REQUIRE(usesOf<HydrostaticCavityLoadTable>(model, 2) ==
            Uses{{"propId", ReferenceTarget::Property, 2201}}); // the basic system

    REQUIRE((hydrostaticSystemOf(2301) == 2301 && hydrostaticGridOf(2301) == 0));
    REQUIRE((hydrostaticSystemOf(-2401) == 0 && hydrostaticGridOf(-2401) == 2401));
    REQUIRE((hydrostaticSystemOf(0) == 0 && hydrostaticGridOf(0) == 0));

    // Each renumbering moves what it owns and keeps the sign.
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE(cardAt<HydrostaticCavityLoadTable>(model, 1).frameOrNode == -3401);
    REQUIRE(cardAt<HydrostaticCavityLoadTable>(model, 0).frameOrNode ==
            2301); // a coordinate system is not a grid point
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    REQUIRE(cardAt<HydrostaticCavityLoadTable>(model, 0).frameOrNode == 2311);
    REQUIRE(cardAt<HydrostaticCavityLoadTable>(model, 1).frameOrNode ==
            -3401); // a grid point is not a coordinate system
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(100));
    REQUIRE((cardAt<HydrostaticCavityLoadTable>(model, 0).propId == 2301 &&
             cardAt<HydrostaticCavityLoadTable>(model, 2).propId == 2301));
    REQUIRE(Renumbering::apply(model, RenumberSpace::LoadCombinationSet, Renumbering::offset(500)) == 3);
    REQUIRE((cardAt<HydrostaticCavityLoadTable>(model, 0).id == 2601 &&
             cardAt<HydrostaticCavityLoadTable>(model, 2).id == 2603));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // What no card defines is unresolved: the property and the grid point, or the coordinate system.
    REQUIRE(unresolvedIn(readBulk("HydrostaticCavityLoad;2101;2201;;;;;;-2401")) == 2);
    REQUIRE(unresolvedIn(
                readBulk("IsoMat;1;2.1E+11;;0.3\nSolidProp;2201;1\nHydrostaticCavityLoad;2101;2201;;;;;;2301")) == 1);
}

TEST_CASE("a HYDROC is edited in place and refuses what the format does not allow", "[model][cards][hydroc]")
{
    Model model = readBulk("HydrostaticCavityLoad;1;2\nHydrostaticCavityLoad;3;4");
    HydrostaticCavityLoadCard edited = cardAt<HydrostaticCavityLoadTable>(model, 1);
    edited.nominalPressure = -5000.0; // an external pressure: the format says to change the signs
    edited.gravity = -9.81;
    edited.frameOrNode = -7;
    edited.punchOption = 2;
    REQUIRE_FALSE(HydrostaticCavityLoadFormat::check(edited).has_value());
    model.table<HydrostaticCavityLoadTable>().store(1, edited);
    REQUIRE(cardAt<HydrostaticCavityLoadTable>(model, 1) == edited);
    REQUIRE(cardAt<HydrostaticCavityLoadTable>(model, 0).nominalPressure == 0.0);
    requireRoundTrip(model);
    edited.punchOption = 4;
    REQUIRE(fieldOf(HydrostaticCavityLoadFormat::check(edited)) == "punchOption");
    edited.punchOption = 0;
    edited.frameOrNode = std::numeric_limits<base::EntityId>::min();
    REQUIRE(fieldOf(HydrostaticCavityLoadFormat::check(edited)) == "frameId/nodeAlt");

    model.remove(CardLocation{typeIndexOf<HydrostaticCavityLoadTable>(), 0});
    model.compact();
    REQUIRE(rowCount<HydrostaticCavityLoadTable>(model) == 1);
    REQUIRE(cardAt<HydrostaticCavityLoadTable>(model, 0).id == 3);

    expectInvalid("HydrostaticCavityLoad;0;2201", "id");
    expectInvalid("HydrostaticCavityLoad;2101", "propId"); // required
    expectInvalid("HydrostaticCavityLoad;2101;0", "propId");
    expectInvalid("HydrostaticCavityLoad;2101;2201;abc", "nominalPressure");
    expectInvalid("HydrostaticCavityLoad;2101;2201;;abc", "gravity");
    expectInvalid("HydrostaticCavityLoad;2101;2201;;;4", "punchOption");
    expectInvalid("HydrostaticCavityLoad;2101;2201;;;-1", "punchOption");
    expectInvalid("HydrostaticCavityLoad;2101;2201;;;;2", "punchPressureLoads");
    expectInvalid("HydrostaticCavityLoad;2101;2201;;;;;2", "overrideOption");
    expectInvalid("HydrostaticCavityLoad;2101;2201;;;;;;x", "frameId/nodeAlt");
    expectInvalid("HydrostaticCavityLoad;2101;2201;;;;;;1.5", "frameId/nodeAlt");
    expectInvalid("HydrostaticCavityLoad;2101;2201;;;;;;;x", "baseHeight");
    expectInvalid("HydrostaticCavityLoad;2101;2201;;;;;;;;x", "heightAbove");
}

// ---------------------------------------------------------------------------------------------------------------------
// HYDROS
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a HYDROS has the layout of the format, requires its density and resolves its defaults",
          "[model][cards][hydros]")
{
    Model model = readBulk(
        "HydrostaticLoad;3101;4.5;0.375;12.5;3201;;1;;3301;;62.5;3401\n"
        "HydrostaticLoad;100001;;1.E+3\n"
        "HydrostaticLoad;100002;101325.;1.E+3;9.81;100041;100042;1;1;-100031;0.5;2.5;100043\n"
        "HydrostaticLoad;3101;0.;0.375;2.5;;3202;1;;3302;0.;47.5");
    REQUIRE((HydrostaticLoadTable::INFO.category == CardCategory::Load &&
             HydrostaticLoadTable::INFO.idSpace == base::IdSpace::Group));
    const HydrostaticLoadCard sampleCard = cardAt<HydrostaticLoadTable>(model, 0);
    REQUIRE((sampleCard.id == 3101 && sampleCard.nominalPressure == 4.5 && sampleCard.density == 0.375 &&
             sampleCard.gravity == 12.5));
    REQUIRE((sampleCard.elementSetId == 3201 && sampleCard.nodeSetId == 0 && sampleCard.punchPressureLoads == 1 &&
             sampleCard.overrideOption == 0));
    REQUIRE((sampleCard.frameOrNode == 3301 && sampleCard.baseHeight == 0.0 && sampleCard.heightAbove == 62.5 &&
             sampleCard.controlNodeSetId == 3401));
    const HydrostaticLoadCard bare = cardAt<HydrostaticLoadTable>(model, 1);
    REQUIRE((bare.id == 100001 && bare.nominalPressure == 0.0 && bare.density == 1000.0 && bare.gravity == 1.0));
    REQUIRE(
        (bare.elementSetId == 0 && bare.nodeSetId == 0 && bare.punchPressureLoads == 0 && bare.overrideOption == 0));
    REQUIRE((bare.frameOrNode == 0 && bare.baseHeight == 0.0 && bare.heightAbove == 0.0 && bare.controlNodeSetId == 0));
    const HydrostaticLoadCard full = cardAt<HydrostaticLoadTable>(model, 2);
    REQUIRE((full.nominalPressure == 101325.0 && full.density == 1000.0 && full.gravity == 9.81));
    REQUIRE((full.elementSetId == 100041 && full.nodeSetId == 100042 && full.punchPressureLoads == 1 &&
             full.overrideOption == 1));
    REQUIRE((full.frameOrNode == -100031 && full.baseHeight == 0.5 && full.heightAbove == 2.5 &&
             full.controlNodeSetId == 100043));
    const HydrostaticLoadCard second = cardAt<HydrostaticLoadTable>(model, 3); // a second card of the same set
    REQUIRE((second.id == 3101 && second.nominalPressure == 0.0 && second.density == 0.375 && second.gravity == 2.5));
    REQUIRE((second.elementSetId == 0 && second.nodeSetId == 3202 && second.punchPressureLoads == 1 &&
             second.frameOrNode == 3302));
    REQUIRE((second.heightAbove == 47.5 && second.controlNodeSetId == 0));
    requireRoundTrip(model);

    // PNOM and G are blank at their defaults, RHO is always written

    // RHO is required and above zero.
    expectInvalid("HydrostaticLoad;3101;4.5", "density");
    expectInvalid("HydrostaticLoad;3101;;0.", "density");
    expectInvalid("HydrostaticLoad;3101;;-1.", "density");
    expectInvalid("HydrostaticLoad;3101;;abc", "density");
}

TEST_CASE("a HYDROS names the sets of its elements, grid points and control grids, and a system or a grid point",
          "[model][cards][hydros]")
{
    Model model =
        readBulk(POINTS_TEXT +
                 "RectFrame2;3301\nIdListSet1;3201;1;2\nIdListSet1;3202;10;11\nIdListSet1;3401;1\nIdListSet1;3402;2\n"
                 "HydrostaticLoad;3101;4.5;0.375;12.5;3201;;1;;3301;;62.5;3401\n"
                 "HydrostaticLoad;3102;;0.375;;3201;3202;;;-10;;;3402\n"
                 "HydrostaticLoad;3103;;0.375");
    REQUIRE(unresolvedIn(model) == 0);
    // The fields in the order of the card; a blank one is not visited.
    REQUIRE(usesOf<HydrostaticLoadTable>(model, 0) == Uses{{"elementSetId", ReferenceTarget::Set, 3201},
                                                           {"frameId/nodeAlt", ReferenceTarget::Coordinate, 3301},
                                                           {"controlNodeSetId", ReferenceTarget::Set, 3401}});
    REQUIRE(usesOf<HydrostaticLoadTable>(model, 1) == Uses{{"elementSetId", ReferenceTarget::Set, 3201},
                                                           {"nodeSetId", ReferenceTarget::Set, 3202},
                                                           {"frameId/nodeAlt", ReferenceTarget::Node, 10},
                                                           {"controlNodeSetId", ReferenceTarget::Set, 3402}});
    REQUIRE(usesOf<HydrostaticLoadTable>(model, 2).empty());

    // The sets and the systems and grid points follow their renumbering, the sign of CID/GRD stays.
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(1000));
    REQUIRE((cardAt<HydrostaticLoadTable>(model, 0).elementSetId == 4201 &&
             cardAt<HydrostaticLoadTable>(model, 0).controlNodeSetId == 4401));
    REQUIRE((cardAt<HydrostaticLoadTable>(model, 1).nodeSetId == 4202 &&
             cardAt<HydrostaticLoadTable>(model, 1).controlNodeSetId == 4402));
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(500));
    REQUIRE((cardAt<HydrostaticLoadTable>(model, 1).frameOrNode == -510 &&
             cardAt<HydrostaticLoadTable>(model, 0).frameOrNode == 3301));
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(5));
    REQUIRE((cardAt<HydrostaticLoadTable>(model, 0).frameOrNode == 3306 &&
             cardAt<HydrostaticLoadTable>(model, 1).frameOrNode == -510));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // Sets that no card defines are unresolved.
    REQUIRE(unresolvedIn(readBulk("HydrostaticLoad;3101;;0.375;;3201;3202")) == 2);
}

TEST_CASE("a HYDROS is edited in place and refuses what the format does not allow", "[model][cards][hydros]")
{
    Model model = readBulk("HydrostaticLoad;1;;0.375\nHydrostaticLoad;2;;0.5");
    HydrostaticLoadCard edited = cardAt<HydrostaticLoadTable>(model, 1);
    edited.density = 1000.0;
    edited.elementSetId = 5;
    edited.nodeSetId = 6;
    edited.controlNodeSetId = 7;
    edited.frameOrNode = -9;
    edited.gravity = -9.81;
    REQUIRE_FALSE(HydrostaticLoadFormat::check(edited).has_value());
    model.table<HydrostaticLoadTable>().store(1, edited);
    REQUIRE(cardAt<HydrostaticLoadTable>(model, 1) == edited);
    requireRoundTrip(model);
    edited.elementSetId = -1;
    REQUIRE(fieldOf(HydrostaticLoadFormat::check(edited)) == "elementSetId");
    edited.elementSetId = 5;
    edited.nodeSetId = -1;
    REQUIRE(fieldOf(HydrostaticLoadFormat::check(edited)) == "nodeSetId");
    edited.nodeSetId = 6;
    edited.controlNodeSetId = -1;
    REQUIRE(fieldOf(HydrostaticLoadFormat::check(edited)) == "controlNodeSetId");
    edited.controlNodeSetId = 7;
    edited.overrideOption = 2;
    REQUIRE(fieldOf(HydrostaticLoadFormat::check(edited)) == "overrideOption");

    model.remove(CardLocation{typeIndexOf<HydrostaticLoadTable>(), 0});
    model.compact();
    REQUIRE(rowCount<HydrostaticLoadTable>(model) == 1);
    REQUIRE(cardAt<HydrostaticLoadTable>(model, 0).id == 2);

    expectInvalid("HydrostaticLoad;0;;0.375", "id");
    expectInvalid("HydrostaticLoad;3101;x;0.375", "nominalPressure");
    expectInvalid("HydrostaticLoad;3101;;0.375;x", "gravity");
    expectInvalid("HydrostaticLoad;3101;;0.375;;-1", "elementSetId");
    expectInvalid("HydrostaticLoad;3101;;0.375;;;-1", "nodeSetId");
    expectInvalid("HydrostaticLoad;3101;;0.375;;;;2", "punchPressureLoads");
    expectInvalid("HydrostaticLoad;3101;;0.375;;;;;2", "overrideOption");
    expectInvalid("HydrostaticLoad;3101;;0.375;;;;;;x", "frameId/nodeAlt");
    expectInvalid("HydrostaticLoad;3101;;0.375;;;;;;;x", "baseHeight");
    expectInvalid("HydrostaticLoad;3101;;0.375;;;;;;;;x", "heightAbove");
    expectInvalid("HydrostaticLoad;3101;;0.375;;;;;;;;;-1", "controlNodeSetId");
}

// ---------------------------------------------------------------------------------------------------------------------
// IMPGEOM
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an IMPGEOM has a line for each group and resolves the defaults that a group leaves blank",
          "[model][cards][impgeom]")
{
    Model model = readBulk(
        "ImperfectionGeometry;4001;;;;;2;5;1;;0.0125;61;;2;5;3;;0.0375;61\n"
        "ImperfectionGeometry;100001;100051;0.5;7;2;1;1;1;;;;;2;1;3;100052;;8;-1\n"
        "ImperfectionGeometry;100002;;;;;;;;;1.;9\n"
        "ImperfectionGeometry;100003;;;;;1;2;3;;;5;0");
    REQUIRE((ImperfectionGeometryTable::INFO.category == CardCategory::ImperfectionGeometry &&
             ImperfectionGeometryTable::INFO.idSpace == base::IdSpace::PerCardType));

    // A card with no default of its own: the blank SETID and MODID of its groups are 0 and -1.
    const ImperfectionGeometryCard sampleCard = cardAt<ImperfectionGeometryTable>(model, 0);
    REQUIRE((sampleCard.id == 4001 && sampleCard.nodeSetId == 0 && sampleCard.scaleFactor == 0.0 &&
             sampleCard.fileUnit == 0 && sampleCard.moduleId == -1));
    REQUIRE(sampleCard.groups.size() == 2);
    REQUIRE((sampleCard.groups[0] == ImperfectionGeometryGroup{2, 5, 1, 0, 0.0125, 61, -1}));
    REQUIRE((sampleCard.groups[1] == ImperfectionGeometryGroup{2, 5, 3, 0, 0.0375, 61, -1}));

    // The defaults of the card are given to the blank fields of each group, which keeps the resolved values.
    const ImperfectionGeometryCard defaults = cardAt<ImperfectionGeometryTable>(model, 1);
    REQUIRE((defaults.nodeSetId == 100051 && defaults.scaleFactor == 0.5 && defaults.fileUnit == 7 &&
             defaults.moduleId == 2));
    REQUIRE(defaults.groups.size() == 2);
    REQUIRE((defaults.groups[0] == ImperfectionGeometryGroup{1, 1, 1, 100051, 0.5, 7, 2}));
    REQUIRE((defaults.groups[1] == ImperfectionGeometryGroup{2, 1, 3, 100052, 0.5, 8, -1}));

    // A group of one field; the blank fields are 0 for the subcase, the step and the mode, and the defaults for the
    // rest.
    const ImperfectionGeometryCard one = cardAt<ImperfectionGeometryTable>(model, 2);
    REQUIRE((one.groups.size() == 1 && one.groups[0] == ImperfectionGeometryGroup{0, 0, 0, 0, 1.0, 9, -1}));
    // A zero in a group is a value, not a blank: module 0 is the main Bulk Data and -1 the whole model.
    const ImperfectionGeometryCard module = cardAt<ImperfectionGeometryTable>(model, 3);
    REQUIRE((module.groups.size() == 1 && module.groups[0] == ImperfectionGeometryGroup{1, 2, 3, 0, 0.0, 5, 0}));
    requireRoundTrip(model);

    // The fields that default to the card are resolved, so they are always written; the others are blank at 0.
}

TEST_CASE("an IMPGEOM keeps a group's explicit zero over the default of the card, and a blank line is a group",
          "[model][cards][impgeom]")
{
    const Model zero = readBulk("ImperfectionGeometry;1;;0.5;;;1;1;1;;0.;;;2;2;2");
    const ImperfectionGeometryCard card = cardAt<ImperfectionGeometryTable>(zero);
    REQUIRE(card.scaleFactor == 0.5);
    REQUIRE((card.groups.size() == 2 && card.groups[0].scaleFactor == 0.0 && card.groups[1].scaleFactor == 0.5));
    requireRoundTrip(zero);

    // A line with nothing on it between two groups is a group of defaults (a last one cannot be told from no line).
    const Model blank = readBulk("ImperfectionGeometry;1;;;;;;;;;;;;1;1;1");
    const ImperfectionGeometryCard blanks = cardAt<ImperfectionGeometryTable>(blank);
    REQUIRE(blanks.groups.size() == 2);
    REQUIRE((blanks.groups[0] == ImperfectionGeometryGroup{0, 0, 0, 0, 0.0, 0, -1}));
    REQUIRE((blanks.groups[1] == ImperfectionGeometryGroup{1, 1, 1, 0, 0.0, 0, -1}));
    requireRoundTrip(blank);

    // Three groups are three lines of eight fields.
    const Model three = readBulk("ImperfectionGeometry;1;;;;;1;1;1;;;3;;2;1;1;;;3;;3;1;1;;;3");
    REQUIRE(cardAt<ImperfectionGeometryTable>(three).groups.size() == 3);
    REQUIRE(cardAt<ImperfectionGeometryTable>(three).groups[2] == ImperfectionGeometryGroup{3, 1, 1, 0, 0.0, 3, -1});
    requireRoundTrip(three);
}

TEST_CASE("an IMPGEOM names the sets of its groups, is named by an IMPCASE and renumbers", "[model][cards][impgeom]")
{
    Model model = readBulk(POINTS_TEXT +
                           "IdListSet1;100051;1;2\n"
                           "IdListSet1;100052;10;11\n"
                           "ImperfectionGeometry;100001;100051;0.5;7;2;1;1;1;;;;;2;1;3;100052;;8;-1\n"
                           "ImperfectionGeometry;100002;;;;;1;1;1;;;3\n"
                           "ImperfectionCase;5;100001;Through;100002\n"
                           "ImperfectionCase;6;100002");
    REQUIRE(unresolvedIn(model) == 0);
    // The default of the card and the set of each group, in the order of the card; a zero names nothing.
    REQUIRE(usesOf<ImperfectionGeometryTable>(model, 0) == Uses{{"nodeSetId", ReferenceTarget::Set, 100051},
                                                                {"nodeSetId", ReferenceTarget::Set, 100051},
                                                                {"nodeSetId", ReferenceTarget::Set, 100052}});
    REQUIRE(usesOf<ImperfectionGeometryTable>(model, 1).empty());

    // The IMPFIDi of an IMPCASE are IMPGEOM IDs now: they resolve, and follow a renumbering of the IMPGEOM.
    REQUIRE(usesOf<ImperfectionCaseTable>(model, 0) ==
            Uses{{"imperfectionIds", ReferenceTarget::ImperfectionGeometry, 100001},
                 {"imperfectionIds", ReferenceTarget::ImperfectionGeometry, 100002}});
    REQUIRE(Renumbering::apply(model, RenumberSpace::ImperfectionGeometry, Renumbering::offset(10)) == 2);
    REQUIRE((cardAt<ImperfectionGeometryTable>(model, 0).id == 100011 &&
             cardAt<ImperfectionGeometryTable>(model, 1).id == 100012));
    REQUIRE(cardAt<ImperfectionCaseTable>(model, 0).imperfections == std::vector<IdRun>{{100011, 100012}});
    REQUIRE(cardAt<ImperfectionCaseTable>(model, 1).imperfections == std::vector<IdRun>{{100012, 100012}});
    REQUIRE(unresolvedIn(model) == 0);

    // The sets follow a renumbering of the sets, the default of the card with the groups that took it.
    REQUIRE(Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(1000)) == 2);
    const ImperfectionGeometryCard moved = cardAt<ImperfectionGeometryTable>(model, 0);
    REQUIRE(moved.nodeSetId == 101051);
    REQUIRE((moved.groups[0].nodeSetId == 101051 && moved.groups[1].nodeSetId == 101052));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // An imperfection that no IMPGEOM defines is unresolved, as is a set that no card defines.
    REQUIRE(unresolvedIn(readBulk("ImperfectionCase;5;100001;100002")) == 2);
    REQUIRE(unresolvedIn(readBulk("ImperfectionGeometry;1;9;;;;1;1;1")) == 2); // the default and the group that took it

    // One number per imperfection.
    REQUIRE_THROWS_AS(readBulk("ImperfectionGeometry;1;;;;;1\nImperfectionGeometry;1;;;;;2"), base::DeckError);
}

TEST_CASE("an IMPGEOM is edited in place and refuses what the format does not allow", "[model][cards][impgeom]")
{
    Model model = readBulk("ImperfectionGeometry;1;;;;;1;1;1\nImperfectionGeometry;2;;;;;1;1;1");
    ImperfectionGeometryCard edited = cardAt<ImperfectionGeometryTable>(model, 1);
    edited.groups.push_back(ImperfectionGeometryGroup{2, 2, 2, 5, 0.25, 9, 3});
    edited.nodeSetId = 5;
    model.table<ImperfectionGeometryTable>().store(1, edited);
    REQUIRE(cardAt<ImperfectionGeometryTable>(model, 1) == edited);
    REQUIRE(cardAt<ImperfectionGeometryTable>(model, 1).groups[0].nodeSetId == 0); // the group kept its resolved value
    requireRoundTrip(model);

    edited.groups.clear();
    REQUIRE(fieldOf(ImperfectionGeometryFormat::check(edited)) == "subcase"); // at least one group
    edited.groups.push_back(ImperfectionGeometryGroup{});
    edited.groups[0].moduleId = -2;
    REQUIRE(fieldOf(ImperfectionGeometryFormat::check(edited)) == "moduleId");
    edited.groups[0].moduleId = -1;
    edited.groups[0].fileUnit = -1;
    REQUIRE(fieldOf(ImperfectionGeometryFormat::check(edited)) == "fileUnit");
    edited.groups[0].fileUnit = 0;
    edited.moduleId = -5;
    REQUIRE(fieldOf(ImperfectionGeometryFormat::check(edited)) == "moduleId");
    edited.moduleId = -1;
    edited.id = 0;
    REQUIRE(fieldOf(ImperfectionGeometryFormat::check(edited)) == "id");

    model.remove(*model.locateInType(typeIndexOf<ImperfectionGeometryTable>(), 1));
    REQUIRE_FALSE(model.locateInType(typeIndexOf<ImperfectionGeometryTable>(), 1).has_value());
    model.compact();
    REQUIRE(rowCount<ImperfectionGeometryTable>(model) == 1);
    REQUIRE(cardAt<ImperfectionGeometryTable>(model, 0).id == 2);

    expectInvalid("ImperfectionGeometry;0;;;;;1", "id");
    expectInvalid("ImperfectionGeometry;1", "subcase"); // no group
    expectInvalid("ImperfectionGeometry;1;-1;;;;1", "nodeSetId");
    expectInvalid("ImperfectionGeometry;1;;x;;;1", "scaleFactor");
    expectInvalid("ImperfectionGeometry;1;;;-1;;1", "fileUnit");
    expectInvalid("ImperfectionGeometry;1;;;;-2;1", "moduleId");
    // The groups after the five fixed fields: SUBCASE, STEP, MODEINC, SETID, SCALE, UNIT and MODID.
    const std::string head = "ImperfectionGeometry;1;;;;;";
    expectInvalid(head + "-1", "subcase");
    expectInvalid(head + "1;-1", "step");
    expectInvalid(head + "1;1;-1", "modeIncrement");
    expectInvalid(head + "1;1;1;-1", "nodeSetId");
    expectInvalid(head + "1;1;1;;x", "scaleFactor");
    expectInvalid(head + "1;1;1;;;-1", "fileUnit");
    expectInvalid(head + "1;1;1;;;;-2", "moduleId");
    // An eighth value is the first field of the next group: here a word where a number is expected.
    expectInvalid(head + "1;1;1;;;;;x", "subcase");
}

// ---------------------------------------------------------------------------------------------------------------------
// TMPSET
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a TMPSET lists grid points with THRU and BY over any number of lines and keeps the runs as written",
          "[model][cards][tmpset]")
{
    Model model = readBulk(
        "TemperatureTimeGroup;5001;120;Through;156;By;6;;;170;174;181;186;;;;;190;Through;199;;;;;;210;212;215;221;228;"
        "240\n"
        "TemperatureTimeGroup;100001;10\n"
        "TemperatureTimeGroup;100002;1;2;3;4;5;6;7;8;9;10\n"
        "TemperatureTimeGroup;100003;1;Through;100;By;5;;;200;Through;300;;;;;;400;500");
    REQUIRE((TemperatureTimeGroupTable::INFO.category == CardCategory::TemperatureGroup &&
             TemperatureTimeGroupTable::INFO.idSpace == base::IdSpace::PerCardType));
    const TemperatureTimeGroupCard sampleCard = cardAt<TemperatureTimeGroupTable>(model, 0);
    REQUIRE(sampleCard.id == 5001);
    REQUIRE((sampleCard.grids == std::vector<ContactRun>{{120, 156, 6},
                                                         {170, 170, 1},
                                                         {174, 174, 1},
                                                         {181, 181, 1},
                                                         {186, 186, 1},
                                                         {190, 199, 1},
                                                         {210, 210, 1},
                                                         {212, 212, 1},
                                                         {215, 215, 1},
                                                         {221, 221, 1},
                                                         {228, 228, 1},
                                                         {240, 240, 1}}));
    REQUIRE((cardAt<TemperatureTimeGroupTable>(model, 1).grids == std::vector<ContactRun>{{10, 10, 1}}));
    REQUIRE(cardAt<TemperatureTimeGroupTable>(model, 2).grids.size() == 10);
    REQUIRE(cardAt<TemperatureTimeGroupTable>(model, 3).grids ==
            std::vector<ContactRun>{{1, 100, 5}, {200, 300, 1}, {400, 400, 1}, {500, 500, 1}});
    requireRoundTrip(model);

    // A THRU over a million grid points is one run.
    const Model large = readBulk("TemperatureTimeGroup;1;1;Through;1000000");
    REQUIRE(cardAt<TemperatureTimeGroupTable>(large).grids == std::vector<ContactRun>{{1, 1000000, 1}});
    requireRoundTrip(large);

    // The runs are written as they were given: a range stays on one line and its increment stays.
}

TEST_CASE("a TMPSET names its grid points, and its ID is the group that a TTEMP names", "[model][cards][tmpset]")
{
    Model model = readBulk(POINTS_TEXT + "TemperatureTimeGroup;5001;1;Through;5;10\nTemperatureTimeGroup;5002;11");
    // A single grid point is a Node, the two ends of a range are NodeInRange (the points between them need not exist).
    REQUIRE(usesOf<TemperatureTimeGroupTable>(model, 0) == Uses{{"node", ReferenceTarget::NodeInRange, 1},
                                                                {"node", ReferenceTarget::NodeInRange, 5},
                                                                {"node", ReferenceTarget::Node, 10}});
    REQUIRE(unresolvedIn(model) == 0); // grid 5 does not exist and is inside a range
    REQUIRE(unresolvedIn(readBulk("TemperatureTimeGroup;5001;1;Through;5;10")) == 1); // no grid 10

    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(
        (cardAt<TemperatureTimeGroupTable>(model, 0).grids == std::vector<ContactRun>{{101, 105, 1}, {110, 110, 1}}));
    REQUIRE((cardAt<TemperatureTimeGroupTable>(model, 1).grids == std::vector<ContactRun>{{111, 111, 1}}));
    REQUIRE(Renumbering::apply(model, RenumberSpace::TemperatureGroup, Renumbering::offset(1000)) == 2);
    REQUIRE((cardAt<TemperatureTimeGroupTable>(model, 0).id == 6001 &&
             cardAt<TemperatureTimeGroupTable>(model, 1).id == 6002));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // One number per group.
    REQUIRE_THROWS_AS(readBulk("TemperatureTimeGroup;5001;1\nTemperatureTimeGroup;5001;2"), base::DeckError);
}

TEST_CASE("a TMPSET is edited in place and refuses what the format does not allow", "[model][cards][tmpset]")
{
    Model model = readBulk("TemperatureTimeGroup;1;10\nTemperatureTimeGroup;2;20");
    TemperatureTimeGroupCard edited = cardAt<TemperatureTimeGroupTable>(model, 1);
    edited.grids = {{1, 3, 1}, {10, 10, 1}, {20, 40, 5}};
    REQUIRE_FALSE(TemperatureTimeGroupFormat::check(edited).has_value());
    model.table<TemperatureTimeGroupTable>().store(1, edited);
    REQUIRE(cardAt<TemperatureTimeGroupTable>(model, 1) == edited);
    requireRoundTrip(model);
    edited.grids = {{5, 3, -1}}; // a range goes up
    REQUIRE(fieldOf(TemperatureTimeGroupFormat::check(edited)) == "node");
    edited.grids = {{1, 5, -1}}; // and so does its increment
    REQUIRE(fieldOf(TemperatureTimeGroupFormat::check(edited)) == "By");
    edited.grids = {{1, 5, 0}};
    REQUIRE(fieldOf(TemperatureTimeGroupFormat::check(edited)) == "By");
    edited.grids = {{4, 4, 2}}; // an increment needs a range
    REQUIRE(fieldOf(TemperatureTimeGroupFormat::check(edited)) == "By");
    edited.grids.clear();
    REQUIRE(fieldOf(TemperatureTimeGroupFormat::check(edited)) == "node");
    edited.grids = {{0, 0, 1}};
    REQUIRE(fieldOf(TemperatureTimeGroupFormat::check(edited)) == "node");

    model.remove(*model.locateInType(typeIndexOf<TemperatureTimeGroupTable>(), 1));
    model.compact();
    REQUIRE(rowCount<TemperatureTimeGroupTable>(model) == 1);
    REQUIRE(cardAt<TemperatureTimeGroupTable>(model, 0).id == 2);

    expectInvalid("TemperatureTimeGroup;0;1", "id");
    expectInvalid("TemperatureTimeGroup;5001", "node"); // at least one grid point
    expectInvalid("TemperatureTimeGroup;5001;0", "node");
    expectInvalid("TemperatureTimeGroup;5001;-3", "node");
    expectInvalid("TemperatureTimeGroup;5001;x", "node");
    expectInvalid("TemperatureTimeGroup;5001;5;Through;3", "node"); // a range goes up
    expectInvalid("TemperatureTimeGroup;5001;Through;5", "Through");
    expectInvalid("TemperatureTimeGroup;5001;1;Through", "Through");
    expectInvalid("TemperatureTimeGroup;5001;1;By;2", "By");
    expectInvalid("TemperatureTimeGroup;5001;1;Through;5;By", "By");
    expectInvalid("TemperatureTimeGroup;5001;1;Through;5;By;0", "By");
    expectInvalid("TemperatureTimeGroup;5001;1;Through;5;By;-1", "By");
}

// ---------------------------------------------------------------------------------------------------------------------
// TTEMP
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a TTEMP is a temperature set that names a TMPSET and a TABLED, or every grid point with -1",
          "[model][cards][ttemp]")
{
    Model model = readBulk(POINTS_TEXT +
                           "TemperatureTimeGroup;5001;1;2\n"
                           "TableDynamic1;7001;;;0.;1.;1.;2.;EndTable\n"
                           "TableDynamic1;7002;;;0.;0.;1.;1.;EndTable\n"
                           "TimeTemperatureLoad;6001;5001;7001\n"
                           "TimeTemperatureLoad;6002;-1;7002\n"
                           "NodeTemperatureLoad;6001;1;20.");
    REQUIRE((TimeTemperatureLoadTable::INFO.category == CardCategory::Load &&
             TimeTemperatureLoadTable::INFO.idSpace == base::IdSpace::Group));
    // It is filed with the TEMP cards, so that everything that selects a temperature set finds it.
    REQUIRE((NodeTemperatureLoadTable::INFO.category == TimeTemperatureLoadTable::INFO.category &&
             NodeTemperatureLoadTable::INFO.idSpace == TimeTemperatureLoadTable::INFO.idSpace));
    const TimeTemperatureLoadCard sampleCard = cardAt<TimeTemperatureLoadTable>(model, 0);
    REQUIRE((sampleCard.id == 6001 && sampleCard.groupId == 5001 && sampleCard.tableId == 7001));
    const TimeTemperatureLoadCard all = cardAt<TimeTemperatureLoadTable>(model, 1);
    REQUIRE((all.id == 6002 && all.groupId == TimeTemperatureLoadCard::ALL_GRID_POINTS && all.tableId == 7002));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    REQUIRE(usesOf<TimeTemperatureLoadTable>(model, 0) == Uses{{"groupId", ReferenceTarget::TemperatureGroup, 5001},
                                                               {"tableId", ReferenceTarget::DynamicTable, 7001}});
    REQUIRE(usesOf<TimeTemperatureLoadTable>(model, 1) ==
            Uses{{"tableId", ReferenceTarget::DynamicTable, 7002}}); // -1 names no TMPSET

    // The set number is a load set, like the SID of the TEMP of the same set: one renumbering moves both.
    REQUIRE(Renumbering::apply(model, RenumberSpace::LoadCombinationSet, Renumbering::offset(100)) == 3);
    REQUIRE((cardAt<TimeTemperatureLoadTable>(model, 0).id == 6101 &&
             cardAt<TimeTemperatureLoadTable>(model, 1).id == 6102));
    REQUIRE(cardAt<NodeTemperatureLoadTable>(model, 0).id == 6101);
    // The group and the table follow their own spaces; -1 stays.
    REQUIRE(Renumbering::apply(model, RenumberSpace::TemperatureGroup, Renumbering::offset(1000)) == 1);
    REQUIRE((cardAt<TimeTemperatureLoadTable>(model, 0).groupId == 6001 &&
             cardAt<TimeTemperatureLoadTable>(model, 1).groupId == -1));
    REQUIRE(Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(5)) == 2);
    REQUIRE((cardAt<TimeTemperatureLoadTable>(model, 0).tableId == 7006 &&
             cardAt<TimeTemperatureLoadTable>(model, 1).tableId == 7007));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // A group or a table that no card defines is unresolved.
    REQUIRE(unresolvedIn(readBulk("TimeTemperatureLoad;6001;5001;7001")) == 2);
    REQUIRE(unresolvedIn(readBulk("TimeTemperatureLoad;6001;-1;7001")) == 1);
}

TEST_CASE("a TTEMP is edited in place and refuses what the format does not allow", "[model][cards][ttemp]")
{
    Model model = readBulk("TimeTemperatureLoad;1;-1;5\nTimeTemperatureLoad;2;7;6");
    TimeTemperatureLoadCard edited = cardAt<TimeTemperatureLoadTable>(model, 1);
    edited.groupId = -1;
    edited.tableId = 9;
    REQUIRE_FALSE(TimeTemperatureLoadFormat::check(edited).has_value());
    model.table<TimeTemperatureLoadTable>().store(1, edited);
    REQUIRE(cardAt<TimeTemperatureLoadTable>(model, 1) == edited);
    requireRoundTrip(model);
    edited.groupId = -2;
    REQUIRE(fieldOf(TimeTemperatureLoadFormat::check(edited)) == "groupId");
    edited.groupId = 0;
    REQUIRE(fieldOf(TimeTemperatureLoadFormat::check(edited)) == "groupId");
    edited.groupId = 7;
    edited.tableId = 0;
    REQUIRE(fieldOf(TimeTemperatureLoadFormat::check(edited)) == "tableId");
    edited.tableId = 6;
    edited.id = 0;
    REQUIRE(fieldOf(TimeTemperatureLoadFormat::check(edited)) == "id");

    model.remove(CardLocation{typeIndexOf<TimeTemperatureLoadTable>(), 0});
    model.compact();
    REQUIRE(rowCount<TimeTemperatureLoadTable>(model) == 1);
    REQUIRE(cardAt<TimeTemperatureLoadTable>(model, 0).id == 2);

    expectInvalid("TimeTemperatureLoad;0;5001;7001", "id");
    expectInvalid("TimeTemperatureLoad;6001", "groupId");
    expectInvalid("TimeTemperatureLoad;6001;0;7001", "groupId");
    expectInvalid("TimeTemperatureLoad;6001;-2;7001", "groupId");
    expectInvalid("TimeTemperatureLoad;6001;x;7001", "groupId");
    expectInvalid("TimeTemperatureLoad;6001;5001", "tableId"); // required
    expectInvalid("TimeTemperatureLoad;6001;5001;0", "tableId");
    expectInvalid("TimeTemperatureLoad;6001;5001;-1", "tableId");
}

// ---------------------------------------------------------------------------------------------------------------------
// TPADEFN
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a TPADEFN has the pairs of a response type and a set of grid points over any number of lines",
          "[model][cards][tpadefn]")
{
    Model model = readBulk(
        "TransferPathSetup;8001;8101;;DA;8201;P;8301\n"
        "TransferPathSetup;100001;100011;;;100012\n"
        "TransferPathSetup;100002;100013;30;D;100014;V;100015;A;100016;P;100017;DVAP;100018");
    REQUIRE((TransferPathSetupTable::INFO.category == CardCategory::TransferPathSet &&
             TransferPathSetupTable::INFO.idSpace == base::IdSpace::PerCardType));
    const TransferPathSetupCard sampleCard = cardAt<TransferPathSetupTable>(model, 0);
    REQUIRE((sampleCard.id == 8001 && sampleCard.nodeSetId == 8101 && sampleCard.unit == 24)); // a blank UNIT is 24
    REQUIRE((sampleCard.responses == std::vector<TransferPathSetupResponse>{{"DA", 8201}, {"P", 8301}}));
    const TransferPathSetupCard bare = cardAt<TransferPathSetupTable>(model, 1);
    REQUIRE((bare.id == 100001 && bare.nodeSetId == 100011 && bare.unit == 24));
    REQUIRE((bare.responses == std::vector<TransferPathSetupResponse>{{"D", 100012}})); // a blank type is D
    const TransferPathSetupCard full = cardAt<TransferPathSetupTable>(model, 2);
    REQUIRE((full.unit == 30 && full.responses.size() == 5));
    REQUIRE((full.responses[4] == TransferPathSetupResponse{"DVAP", 100018})); // any case is read, kept in upper case
    requireRoundTrip(model);

    // A static default is a blank: the type D and the unit 24 are not written.

    // A pair with both fields blank is padding.
    const Model padded = readBulk("TransferPathSetup;1;2;;DA;10;;;V;11");
    REQUIRE((cardAt<TransferPathSetupTable>(padded).responses ==
             std::vector<TransferPathSetupResponse>{{"DA", 10}, {"V", 11}}));
}

TEST_CASE("a TPADEFN names the SET3 of its interface grid points and of each response, and the Case Control names it",
          "[model][cards][tpadefn]")
{
    Model model = readBulk(
        POINTS_TEXT +
        "IdListSet1;8101;1;2\nIdListSet1;8201;10\nIdListSet1;8301;11\nTransferPathSetup;8001;8101;;DA;8201;P;8301");
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(usesOf<TransferPathSetupTable>(model, 0) == Uses{{"nodeSetId", ReferenceTarget::Set, 8101},
                                                             {"outputSet", ReferenceTarget::Set, 8201},
                                                             {"outputSet", ReferenceTarget::Set, 8301}});
    REQUIRE(Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(1000)) == 3);
    const TransferPathSetupCard moved = cardAt<TransferPathSetupTable>(model, 0);
    REQUIRE((moved.nodeSetId == 9101 && moved.responses[0].outputSet == 9201 && moved.responses[1].outputSet == 9301));
    REQUIRE(unresolvedIn(model) == 0);
    // The ID is the card's own: its space renumbers it, and nothing else holds it but the Case Control.
    REQUIRE(Renumbering::apply(model, RenumberSpace::TransferPathSet, Renumbering::offset(10)) == 1);
    REQUIRE(cardAt<TransferPathSetupTable>(model, 0).id == 8011);
    requireRoundTrip(model);

    // Sets that no card defines are unresolved.
    REQUIRE(unresolvedIn(readBulk("TransferPathSetup;8001;8101;;DA;8201;P;8301")) == 3);
    REQUIRE_THROWS_AS(readBulk("TransferPathSetup;8001;8101;;D;2\nTransferPathSetup;8001;8102;;D;3"), base::DeckError);
}

TEST_CASE("a TPADEFN is edited in place and refuses what the format does not allow", "[model][cards][tpadefn]")
{
    Model model = readBulk("TransferPathSetup;1;2;;;3\nTransferPathSetup;4;5;;;6");
    TransferPathSetupCard edited = cardAt<TransferPathSetupTable>(model, 1);
    edited.unit = 99;
    edited.responses = {{"V", 7}, {"AP", 8}};
    REQUIRE_FALSE(TransferPathSetupFormat::check(edited).has_value());
    model.table<TransferPathSetupTable>().store(1, edited);
    REQUIRE(cardAt<TransferPathSetupTable>(model, 1) == edited);
    requireRoundTrip(model);
    edited.unit = 0;
    REQUIRE(fieldOf(TransferPathSetupFormat::check(edited)) == "unit");
    edited.unit = 24;
    edited.responses[0].type = "X";
    REQUIRE(fieldOf(TransferPathSetupFormat::check(edited)) == "responseKind");
    edited.responses[0].type = "";
    REQUIRE(fieldOf(TransferPathSetupFormat::check(edited)) == "responseKind");
    edited.responses[0].type = "DDDDDDDDD"; // repeated response letters have no field-width limit
    REQUIRE_FALSE(TransferPathSetupFormat::check(edited).has_value());
    edited.responses[0].type = "D";
    edited.responses[0].outputSet = 0;
    REQUIRE(fieldOf(TransferPathSetupFormat::check(edited)) == "outputSet");
    edited.responses.clear();
    REQUIRE(fieldOf(TransferPathSetupFormat::check(edited)) == "outputSet"); // at least one response
    edited.responses = {{"D", 1}};
    edited.nodeSetId = 0;
    REQUIRE(fieldOf(TransferPathSetupFormat::check(edited)) == "nodeSetId");

    model.remove(*model.locateInType(typeIndexOf<TransferPathSetupTable>(), 1));
    model.compact();
    REQUIRE(rowCount<TransferPathSetupTable>(model) == 1);
    REQUIRE(cardAt<TransferPathSetupTable>(model, 0).id == 4);

    expectInvalid("TransferPathSetup;0;8101", "id");
    expectInvalid("TransferPathSetup;8001", "nodeSetId");
    expectInvalid("TransferPathSetup;8001;0", "nodeSetId");
    expectInvalid("TransferPathSetup;8001;8101;x", "unit");
    expectInvalid("TransferPathSetup;8001;8101;0;D;5", "unit");
    expectInvalid("TransferPathSetup;8001;8101", "outputSet"); // at least one response
    expectInvalid("TransferPathSetup;8001;8101;;Q;8201", "responseKind");
    requireRoundTrip(readBulk("TransferPathSetup;8001;8101;;DDDDDDDDD;8201"));
    expectInvalid("TransferPathSetup;8001;8101;;DA", "outputSet"); // a type and no set
    expectInvalid("TransferPathSetup;8001;8101;;DA;0", "outputSet");
    expectInvalid("TransferPathSetup;8001;8101;;DA;x", "outputSet");
}
