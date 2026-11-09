// Checks of what the aeroelastic cards share, on the four cards that show it: AEFACT (a list of reals),
// AELIST (a list of boxes with THRU), CAERO1 (a panel: the aerodynamic element ID space, resolved references, the boxes
// it owns) and TRIM (a card kept whole, labels, the Case Control command that selects it). The new ID spaces, the
// reference kinds and the renumbering of the boxes are tested here once; the cards of the groups have their own files.

#include "oor/model/cards/aero/AeroBoxRanges.hpp"
#include "oor/model/cards/aero/aeroBoxList/AeroBoxListTable.hpp"
#include "oor/model/cards/aero/aeroFactorList/AeroFactorListTable.hpp"
#include "oor/model/cards/aero/aeroLiftingPanel1/AeroLiftingPanel1Table.hpp"
#include "oor/model/cards/aero/aeroTrim/AeroTrimTable.hpp"
#include "oor/model/cards/coordinate_systems/rectFrame2/RectFrame2Table.hpp"
#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/control/case_control/CaseControl.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The number a command of the global Case Control section holds.
std::int64_t globalCommand(const Model& model, const char* name)
{
    const CaseEntry* entry = CaseControl::ownCommand(model.control().caseControl.global(), name);
    REQUIRE(entry != nullptr);
    return entry->value.integer;
}

} // namespace

TEST_CASE("an AEFACT is a list of reals with no blank inside", "[model][cards][aero]")
{
    const Model model = readBulk(
        "AeroFactorList;93;0.2;0.6;1.\nAeroFactorList;98;0.;0.1;0.2;0.3;0.4;0.5;0.6;0.7;0.8;0.9;1.\nAeroFactorList;99;"
        "2.5");
    REQUIRE(cardAt<AeroFactorListTable>(model, 0).values == std::vector<double>{0.2, 0.6, 1.0});
    REQUIRE(cardAt<AeroFactorListTable>(model, 1).values.size() == 11);
    REQUIRE(model.findTable<AeroFactorListTable>()->valuesAt(2).size() == 1);
    requireRoundTrip(model);

    expectInvalid("AeroFactorList;0;0.3", "id");
    expectInvalid("AeroFactorList;5", "value1");          // at least one number
    expectInvalid("AeroFactorList;5;0.3;;0.7", "values"); // embedded blank fields are not allowed
    expectInvalid("AeroFactorList;5;0.3;LEFT", "values");
    // An AEFACT and an FLFACT may have the same ID: the space is the card type's, but two AEFACTs may not.
    REQUIRE_THROWS_AS(readBulk("AeroFactorList;5;0.3\nAeroFactorList;5;0.4"), base::DeckError);
}

TEST_CASE("an AELIST keeps its single boxes and its THRU ranges", "[model][cards][aero]")
{
    Model model = readBulk("AeroBoxList;72;1001;Through;1064;1121;Through;1128;1301;1302\nAeroBoxList;76;5");
    const AeroBoxListCard card = cardAt<AeroBoxListTable>(model, 0);
    REQUIRE(card.ids == std::vector<IdRun>{{1001, 1064}, {1121, 1128}, {1301, 1301}, {1302, 1302}});
    requireRoundTrip(model);

    RecordingVisitor visitor;
    model.table<AeroBoxListTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"entries"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroBox});

    expectInvalid("AeroBoxList;0;1", "id");
    expectInvalid("AeroBoxList;10", "entries"); // a list needs an item
    expectInvalid("AeroBoxList;10;-4", "entries");
    expectInvalid("AeroBoxList;10;9;Through;3", "entries"); // a range goes up
}

TEST_CASE("a CAERO1 is an aerodynamic element: its own ID space, its references and the boxes it owns",
          "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroLiftingPanel1;1000;1;3;2;1;;;1;0.;0.;0.;1.5;0.3;2.;0.;0.9\n"
        "AeroLiftingPanel1;2000;2;;;;10;20;2;1.;2.;3.;2.;1.;5.;0.;1.\n"
        "AeroFactorList;10;0.;0.25;0.5;1.\n"
        "AeroFactorList;20;0.;0.5;1.\n"
        "RectFrame2;3;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "ShellQuad4;1000;1;1;2;3;4\n");
    const AeroLiftingPanel1Card first = cardAt<AeroLiftingPanel1Table>(model, 0);
    REQUIRE((first.id == 1000 && first.propId == 1 && first.positionFrameId == 3 && first.spanBoxCount == 2 &&
             first.chordBoxCount == 1));
    REQUIRE((first.spanDivisionListId == 0 && first.chordDivisionListId == 0 && first.interferenceGroupId == 1));
    REQUIRE((first.point1 == std::array<double, 3>{0., 0., 0.} && first.rootChord == 1.5));
    REQUIRE((first.point4 == std::array<double, 3>{0.3, 2., 0.} && first.tipChord == 0.9));
    const AeroLiftingPanel1Card lists = cardAt<AeroLiftingPanel1Table>(model, 1);
    REQUIRE((lists.spanDivisionListId == 10 && lists.chordDivisionListId == 20 && lists.spanBoxCount == 0 &&
             lists.point1[2] == 3.0));
    requireRoundTrip(model);

    // The element space of the aerodynamic panels is not the structural one: a CQUAD4 1000 and a CAERO1 1000 coexist.
    REQUIRE(model.locate(base::IdSpace::AeroElement, 1000).has_value());
    REQUIRE(model.locate(base::IdSpace::Element, 1000).has_value());
    REQUIRE_THROWS_AS(readBulk("AeroLiftingPanel1;1;1;;2;1;;;1;0.;0.;0.;1.;0.;1.;0.;1.\nAeroLiftingPanel1;1;1;;2;1;;;1;"
                               "0.;0.;0.;1.;0.;1.;0.;1."),
                      base::DeckError);

    // The boxes: 2 x 1 for the first panel, 3 x 2 from the lists for the second. A panel owns at least its EID.
    const AeroBoxRanges ranges(model);
    REQUIRE(ranges.ranges().size() == 2);
    REQUIRE((ranges.ranges()[0].panelId == 1000 && ranges.ranges()[0].boxCount == 2));
    REQUIRE((ranges.ranges()[1].panelId == 2000 && ranges.ranges()[1].boxCount == 6));
    REQUIRE(ranges.owner(1001) != nullptr);
    REQUIRE(ranges.owner(1002) == nullptr);
    REQUIRE((ranges.owner(2005) != nullptr && ranges.owner(2005)->panelId == 2000));
    REQUIRE(ranges.owner(2006) == nullptr);
    REQUIRE((ranges.ranges()[0].countKnown && ranges.ranges()[1].countKnown));

    // Its references: the property, the coordinate system, the two division lists. PAERO1 is not in this deck.
    RecordingVisitor visitor;
    model.table<AeroLiftingPanel1Table>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propId", "spanDivisionListId", "chordDivisionListId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroProperty, ReferenceTarget::AeroFactor,
                                                            ReferenceTarget::AeroFactor});
    visitor = RecordingVisitor{};
    model.table<AeroLiftingPanel1Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propId", "positionFrameId"});
    // The two PAERO1 cards that nothing defines yet, and the property and four grid points of the CQUAD4.
    REQUIRE(unresolvedIn(model) == 7);

    Renumbering::apply(model, RenumberSpace::AeroFactor, Renumbering::offset(100));
    REQUIRE((cardAt<AeroLiftingPanel1Table>(model, 1).spanDivisionListId == 110 &&
             cardAt<AeroLiftingPanel1Table>(model, 1).chordDivisionListId == 120));
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    REQUIRE(cardAt<AeroLiftingPanel1Table>(model, 0).positionFrameId == 13);

    expectInvalid("AeroLiftingPanel1;0;1;;2;1;;;1;0.;0.;0.;1.;0.;1.;0.;1.", "id");
    expectInvalid("AeroLiftingPanel1;1;0;;2;1;;;1;0.;0.;0.;1.;0.;1.;0.;1.", "propId");
    expectInvalid("AeroLiftingPanel1;1;1;;;1;;;1;0.;0.;0.;1.;0.;1.;0.;1.",
                  "spanBoxCount"); // no number and no list of spans
    expectInvalid("AeroLiftingPanel1;1;1;;2;;;;1;0.;0.;0.;1.;0.;1.;0.;1.", "chordBoxCount");
    expectInvalid("AeroLiftingPanel1;1;1;;2;1;;;;0.;0.;0.;1.;0.;1.;0.;1.", "interferenceGroupId");
    expectInvalid("AeroLiftingPanel1;1;1;;2;1;;;1;0.;0.;0.;0.;0.;1.;0.;0.", "rootChord"); // both chords zero: no area
    expectInvalid("AeroLiftingPanel1;1;1;;2;1;;;1;0.;0.;0.;-1.;0.;1.;0.;1.", "rootChord");
    expectInvalid("AeroLiftingPanel1;1;1;-1;2;1;;;1;0.;0.;0.;1.;0.;1.;0.;1.", "positionFrameId");
    expectInvalid("AeroLiftingPanel1;1;1;;-2;1;;;1;0.;0.;0.;1.;0.;1.;0.;1.", "spanBoxCount");
}

TEST_CASE("a panel whose division list is missing owns its EID only and says it does not know its boxes",
          "[model][cards][aero]")
{
    const Model model = readBulk(
        "AeroLiftingPanel1;1000;1;;2;;;99;1;0.;0.;0.;1.;0.;1.;0.;1.\n"
        "AeroFactorList;10;0.;1.\n"
        "AeroLiftingPanel1;2000;1;;2;;;10;1;0.;0.;0.;1.;0.;1.;0.;1.");
    const AeroBoxRanges ranges(model);
    REQUIRE(ranges.ranges().size() == 2);
    REQUIRE((ranges.ranges()[0].panelId == 1000 && ranges.ranges()[0].boxCount == 1 && !ranges.ranges()[0].countKnown));
    REQUIRE((ranges.ranges()[1].panelId == 2000 && ranges.ranges()[1].boxCount == 2 && ranges.ranges()[1].countKnown));
}

TEST_CASE("the boxes of an AELIST and a CAERO1 move together when the aerodynamic elements are renumbered",
          "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroLiftingPanel1;1000;1;;2;3;;;1;0.;0.;0.;1.;0.;1.;0.;1.\n"
        "AeroLiftingPanel1;2000;1;;2;3;;;1;0.;2.;0.;1.;0.;3.;0.;1.\n"
        "AeroBoxList;1;1000;Through;1005\n"
        "AeroBoxList;2;2003;5001\n"
        "AeroBoxList;3;2000;Through;2005;1003");
    REQUIRE(unresolvedIn(model) == 2); // the PAERO1 only: a box that no panel has is not reported (it may be a grid)

    const std::size_t changed = Renumbering::apply(model, RenumberSpace::AeroElement, Renumbering::offset(500));
    REQUIRE(changed == 2);
    REQUIRE(
        (cardAt<AeroLiftingPanel1Table>(model, 0).id == 1500 && cardAt<AeroLiftingPanel1Table>(model, 1).id == 2500));
    REQUIRE(cardAt<AeroBoxListTable>(model, 0).ids == std::vector<IdRun>{{1500, 1505}});
    // 2003 is the fourth box of the second panel: it follows it. 5001 belongs to no panel and stays.
    REQUIRE(cardAt<AeroBoxListTable>(model, 1).ids == std::vector<IdRun>{{2503, 2503}, {5001, 5001}});
    // An item of another panel's range stays in step with its own panel (1003 is the fourth box of the first one).
    REQUIRE(cardAt<AeroBoxListTable>(model, 2).ids == std::vector<IdRun>{{2500, 2505}, {1503, 1503}});
}

TEST_CASE("a TRIM has its labels in the fields of the format reference and is selected by the Case Control TRIM",
          "[model][cards][aero]")
{
    Model model = readModelText(
        "[case]\n"
        "TrimSelection;2\n"
        "[model]\n"
        "AeroTrim;1;0.8;150.;RigidAccelZ;1.;AngleOfAttack;4.;0.;ELEV;0.3\n"
        "AeroTrim;2;1.2;740.;PitchRate;0.;RigidAccelZ;-4.;;RigidAngularAccelY;0.;AILERON;0.;SideSlip;0.;YawRate;0.;"
        "RigidAngularAccelX;0.\n"
        "AeroTrim;3;0.5;10.\n",
        true);
    const AeroTrimCard first = cardAt<AeroTrimTable>(model, 0);
    REQUIRE((first.id == 1 && first.machNumber == 0.8 && first.dynamicPressure == 150.0 &&
             first.equivalentAspectRatio == 0.0));
    REQUIRE(first.variables ==
            std::vector<AeroTrimVariable>{{"RigidAccelZ", 1.0}, {"AngleOfAttack", 4.0}, {"ELEV", 0.3}});
    const AeroTrimCard second = cardAt<AeroTrimTable>(model, 1);
    REQUIRE((second.equivalentAspectRatio == 1.0 && second.variables.size() == 7 &&
             second.variables[6].label == "RigidAngularAccelX"));
    REQUIRE(second.variables[4].label == "SideSlip"); // the label is kept as written
    REQUIRE(cardAt<AeroTrimTable>(model, 2).variables.empty());
    requireRoundTrip(model);

    // The field of each pair: AEQR has field 8 and the pairs after it start the continuation line.

    REQUIRE(unresolvedIn(model) == 0);
    Model dangling = readModelText("[case]\nTrimSelection;9\n[model]\nAeroTrim;1;0.5;10.\n", true);
    REQUIRE(unresolvedIn(dangling) == 1); // the command selects a trim that no card defines
    Renumbering::apply(model, RenumberSpace::AeroTrimSet, Renumbering::offset(10));
    REQUIRE((cardAt<AeroTrimTable>(model, 0).id == 11 && cardAt<AeroTrimTable>(model, 1).id == 12));
    REQUIRE(globalCommand(model, "TrimSelection") == 12);

    expectInvalid("AeroTrim;0;0.9;100.", "id");
    expectInvalid("AeroTrim;1;-0.5;100.", "machNumber");
    expectInvalid("AeroTrim;1;1.;100.", "machNumber"); // 1.0 is neither subsonic nor supersonic
    expectInvalid("AeroTrim;1;0.9;0.", "dynamicPressure");
    expectInvalid("AeroTrim;1;0.9;100.;RigidAccelZ", "value1"); // a label with no value
    expectInvalid("AeroTrim;1;0.9;100.;;2.", "label1");
    expectInvalid("AeroTrim;1;0.9;100.;RigidAccelZ;1.;RigidAccelZ;2.",
                  "labels"); // a variable is fixed once, whatever the case
    expectInvalid("AeroTrim;1;0.9;100.;RigidAccelZ;1.;;;2.", "equivalentAspectRatio"); // AEQR is a number from 0 to 1
}
