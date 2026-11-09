// The cards that describe the aerodynamic panels and bodies: CAERO2 (a slender body), CAERO3
// (a Mach box surface), CAERO4 (strips), CAERO5 (piston theory strips) and their properties PAERO1, PAERO2 and PAERO3.
// Each case reads the fields of a card in the places of the format reference, the references it lists, what a
// renumbering moves, the ID space it shares with the other panel cards and the values it refuses.

#include "oor/model/cards/aero/AeroBoxRanges.hpp"
#include "oor/model/cards/aero/aeroBody2/AeroBody2Table.hpp"
#include "oor/model/cards/aero/aeroBodyProp2/AeroBodyProp2Table.hpp"
#include "oor/model/cards/aero/aeroLiftingPanelProp1/AeroLiftingPanelProp1Table.hpp"
#include "oor/model/cards/aero/aeroMachBoxPanel3/AeroMachBoxPanel3Table.hpp"
#include "oor/model/cards/aero/aeroMachBoxProp3/AeroMachBoxProp3Table.hpp"
#include "oor/model/cards/aero/aeroPistonPanel5/AeroPistonPanel5Table.hpp"
#include "oor/model/cards/aero/aeroStripPanel4/AeroStripPanel4Table.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

using Bodies = std::array<base::EntityId, 6>;

// The references that the card at `row` of the table lists, in the order of its fields.
template <typename TableT>
RecordingVisitor referencesOf(Model& model, std::uint32_t row)
{
    RecordingVisitor visitor;
    model.table<TableT>().visitReferences(row, visitor);
    return visitor;
}

} // namespace

TEST_CASE("a CAERO2 body has two cuts, a group that may be zero, and the elements it owns", "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroBody2;1400;2;110;;5;98;;1;-2.;80.;-25.;150.\n"
        "AeroBody2;1600;3;;2;3;;;0;0.;0.;0.;50.\n"
        "AeroFactorList;98;0.;0.5;1.\n"
        "RectFrame2;110;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "ShellQuad4;1400;1;1;2;3;4\n");
    const AeroBody2Card first = cardAt<AeroBody2Table>(model, 0);
    REQUIRE((first.id == 1400 && first.propId == 2 && first.positionFrameId == 110 && first.slenderElemCount == 0 &&
             first.interferenceElemCount == 5));
    REQUIRE((first.slenderListId == 98 && first.interferenceListId == 0 && first.interferenceGroupId == 1));
    REQUIRE((first.point1 == std::array<double, 3>{-2.0, 80.0, -25.0} && first.length == 150.0));
    const AeroBody2Card second = cardAt<AeroBody2Table>(model, 1);
    REQUIRE((second.positionFrameId == 0 && second.slenderElemCount == 2 && second.interferenceElemCount == 3 &&
             second.interferenceGroupId == 0 && second.length == 50.0));
    requireRoundTrip(model);

    // The aerodynamic elements are apart from the structural ones: a CQUAD4 1400 and a CAERO2 1400 coexist.
    REQUIRE(model.locate(base::IdSpace::AeroElement, 1400).has_value());
    REQUIRE(model.locate(base::IdSpace::Element, 1400).has_value());

    // The elements: NSB + NINT, the numbers or else one less than the points of the AEFACT.
    const AeroBoxRanges ranges(model);
    REQUIRE(ranges.ranges().size() == 2);
    REQUIRE((ranges.ranges()[0].panelId == 1400 && ranges.ranges()[0].boxCount == 7));
    REQUIRE((ranges.ranges()[1].panelId == 1600 && ranges.ranges()[1].boxCount == 5));

    // The property, the coordinate system and the list of the slender body; the PAERO2 cards are not in this deck.
    REQUIRE(referencesOf<AeroBody2Table>(model, 0).fields ==
            std::vector<std::string>{"propId", "positionFrameId", "slenderListId"});
    REQUIRE(referencesOf<AeroBody2Table>(model, 0).targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::AeroProperty, ReferenceTarget::Coordinate,
                                         ReferenceTarget::AeroFactor});
    REQUIRE(referencesOf<AeroBody2Table>(model, 1).fields == std::vector<std::string>{"propId"});
    REQUIRE(unresolvedIn(model) == 7); // two PAERO2, and the property and four grid points of the CQUAD4

    Renumbering::apply(model, RenumberSpace::AeroFactor, Renumbering::offset(100));
    REQUIRE(cardAt<AeroBody2Table>(model, 0).slenderListId == 198);
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    REQUIRE(cardAt<AeroBody2Table>(model, 0).positionFrameId == 120);

    // A number for the slender body and a list for the interference elements.
    const Model lists = readBulk("AeroBody2;10;1;;3;;;6;2;0.;0.;0.;5.");
    REQUIRE(
        (cardAt<AeroBody2Table>(lists).slenderElemCount == 3 && cardAt<AeroBody2Table>(lists).interferenceListId == 6));

    expectInvalid("AeroBody2;0;1;;2;1;;;1;;;;5.", "id");
    expectInvalid("AeroBody2;1;0;;2;1;;;1;;;;5.", "propId");
    expectInvalid("AeroBody2;1;1;-1;2;1;;;1;;;;5.", "positionFrameId");
    expectInvalid("AeroBody2;1;1;;;1;;;1;;;;5.",
                  "slenderElemCount"); // neither a number nor a list of slender body elements
    expectInvalid("AeroBody2;1;1;;2;;;;1;;;;5.", "interferenceElemCount"); // an interference element is required
    expectInvalid("AeroBody2;1;1;;-2;1;;;1;;;;5.", "slenderElemCount");
    expectInvalid("AeroBody2;1;1;;2;-1;;;1;;;;5.", "interferenceElemCount");
    expectInvalid("AeroBody2;1;1;;2;1;-3;;1;;;;5.", "slenderListId");
    expectInvalid("AeroBody2;1;1;;2;1;;-3;1;;;;5.", "interferenceListId");
    expectInvalid("AeroBody2;1;1;;2;1;;;;;;;5.", "interferenceGroupId"); // the group is required, even when it is zero
    expectInvalid("AeroBody2;1;1;;2;1;;;-1;;;;5.", "interferenceGroupId");
    expectInvalid("AeroBody2;1;1;;2;1;;;1;1.;2.;3.", "length"); // the length is required
    expectInvalid("AeroBody2;1;1;;2;1;;;1;;;;0.", "length");
    expectInvalid("AeroBody2;1;1;;2;1;;;1;;;;-5.", "length");
}

TEST_CASE("a PAERO1 lists up to six bodies with no blank between them", "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroLiftingPanelProp1;10;1500;1600\n"
        "AeroLiftingPanelProp1;11\n"
        "AeroLiftingPanelProp1;12;3;4;5;6;7;8\n"
        "AeroBodyProp2;20;ZDirection;1.;1.\n"
        "AeroBody2;1500;20;;2;1;;;1;;;;5.\n"
        "AeroBody2;1600;20;;2;1;;;1;;;;5.");
    REQUIRE((cardAt<AeroLiftingPanelProp1Table>(model, 0).id == 10 &&
             cardAt<AeroLiftingPanelProp1Table>(model, 0).bodies == Bodies{1500, 1600, 0, 0, 0, 0}));
    REQUIRE(cardAt<AeroLiftingPanelProp1Table>(model, 1).bodies == Bodies{});
    REQUIRE(cardAt<AeroLiftingPanelProp1Table>(model, 2).bodies == Bodies{3, 4, 5, 6, 7, 8});
    requireRoundTrip(model);

    // Every body is a CAERO element: the field is the same for each, and a blank is not visited.
    const RecordingVisitor named = referencesOf<AeroLiftingPanelProp1Table>(model, 0);
    REQUIRE(named.fields == std::vector<std::string>{"bodies", "bodies"});
    REQUIRE(named.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroElement, ReferenceTarget::AeroElement});
    REQUIRE(referencesOf<AeroLiftingPanelProp1Table>(model, 1).fields.empty());
    REQUIRE(unresolvedIn(model) == 6); // the bodies 3 to 8 of the last card

    // The bodies follow their CAERO2 when the aerodynamic elements are renumbered.
    Renumbering::apply(model, RenumberSpace::AeroElement, Renumbering::offset(500));
    REQUIRE((cardAt<AeroBody2Table>(model, 0).id == 2000 && cardAt<AeroBody2Table>(model, 1).id == 2100));
    REQUIRE(cardAt<AeroLiftingPanelProp1Table>(model, 0).bodies == Bodies{2000, 2100, 0, 0, 0, 0});
    // The PID is in the space that every PAERO shares, so the CAERO2 that names the PAERO2 follows it.
    Renumbering::apply(model, RenumberSpace::AeroProperty, Renumbering::offset(100));
    REQUIRE((cardAt<AeroLiftingPanelProp1Table>(model, 0).id == 110 && cardAt<AeroBodyProp2Table>(model).id == 120));
    REQUIRE(cardAt<AeroBody2Table>(model, 0).propId == 120);

    // A PAERO1 and a PAERO2 cannot have the same PID.
    REQUIRE_THROWS_AS(readBulk("AeroLiftingPanelProp1;5\nAeroBodyProp2;5;ZDirection;1.;1."), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("AeroLiftingPanelProp1;5\nAeroLiftingPanelProp1;5"), base::DeckError);

    expectInvalid("AeroLiftingPanelProp1;0", "id");
    expectInvalid("AeroLiftingPanelProp1", "id");
    expectInvalid("AeroLiftingPanelProp1;1;-3", "body1");
    expectInvalid("AeroLiftingPanelProp1;1;;3", "body2"); // embedded blanks are not allowed
    expectInvalid("AeroLiftingPanelProp1;1;1;;3", "body3");
}

TEST_CASE("a PAERO2 has its half-width lists, its angle arrays and two ranges of interference elements",
          "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroBodyProp2;8201;ZDirection;4.;1.5;24;93;104;;2;5\n"
        "AeroBodyProp2;8202;PlaneZy;1.;2.\n"
        "AeroBodyProp2;8203;YDirection;2.5;1.5;3;4;5;6;1;4;5;9\n"
        "AeroFactorList;24;0.;1.;2.");
    const AeroBodyProp2Card first = cardAt<AeroBodyProp2Table>(model, 0);
    REQUIRE((first.id == 8201 && first.orientation == BodyOrientation::Z && first.width == 4.0 &&
             first.aspectRatio == 1.5));
    REQUIRE((first.slenderHalfWidthListId == 24 && first.interferenceHalfWidthListId == 93 &&
             first.angleListId1 == 104 && first.angleListId2 == 0));
    REQUIRE((first.angleList1FirstElem == 2 && first.angleList1LastElem == 5 && first.angleList2FirstElem == 0 &&
             first.angleList2LastElem == 0));
    const AeroBodyProp2Card second = cardAt<AeroBodyProp2Table>(model, 1);
    REQUIRE((second.orientation == BodyOrientation::ZY && second.slenderHalfWidthListId == 0 &&
             second.interferenceHalfWidthListId == 0 && second.angleList1LastElem == 0));
    const AeroBodyProp2Card full = cardAt<AeroBodyProp2Table>(model, 2);
    REQUIRE((full.orientation == BodyOrientation::Y && full.slenderHalfWidthListId == 3 &&
             full.interferenceHalfWidthListId == 4 && full.angleListId1 == 5 && full.angleListId2 == 6));
    REQUIRE((full.angleList1FirstElem == 1 && full.angleList1LastElem == 4 && full.angleList2FirstElem == 5 &&
             full.angleList2LastElem == 9));
    requireRoundTrip(model);

    // The word is written as the format reference has it, whatever the case it was read in, and the second line only
    // when it is used.

    // The four AEFACT lists are references, the ranges are element numbers of the body and are not.
    const RecordingVisitor lists = referencesOf<AeroBodyProp2Table>(model, 2);
    REQUIRE(lists.fields == std::vector<std::string>{"slenderHalfWidthListId", "interferenceHalfWidthListId",
                                                     "angleListId1", "angleListId2"});
    REQUIRE(lists.targets == std::vector<ReferenceTarget>(4, ReferenceTarget::AeroFactor));
    REQUIRE(referencesOf<AeroBodyProp2Table>(model, 1).fields.empty());
    REQUIRE(unresolvedIn(model) == 6); // 93 and 104 of the first card, the four lists of the third

    Renumbering::apply(model, RenumberSpace::AeroFactor, Renumbering::offset(100));
    REQUIRE(cardAt<AeroBodyProp2Table>(model, 0).slenderHalfWidthListId == 124);
    Renumbering::apply(model, RenumberSpace::AeroProperty, Renumbering::offset(1000));
    REQUIRE((cardAt<AeroBodyProp2Table>(model, 0).id == 9201 && cardAt<AeroBodyProp2Table>(model, 2).id == 9203));

    expectInvalid("AeroBodyProp2;0;ZDirection;1.;1.", "id");
    expectInvalid("AeroBodyProp2;1;;1.;1.", "orientation"); // the orientation is required
    expectInvalid("AeroBodyProp2;1;X;1.;1.", "orientation");
    expectInvalid("AeroBodyProp2;1;ZDirection;;1.", "width");
    expectInvalid("AeroBodyProp2;1;ZDirection;0.;1.", "width");
    expectInvalid("AeroBodyProp2;1;ZDirection;1.", "aspectRatio");
    expectInvalid("AeroBodyProp2;1;ZDirection;1.;0.", "aspectRatio");
    expectInvalid("AeroBodyProp2;1;ZDirection;1.;1.;-1", "slenderHalfWidthListId");
    expectInvalid("AeroBodyProp2;1;ZDirection;1.;1.;;-1", "interferenceHalfWidthListId");
    expectInvalid("AeroBodyProp2;1;ZDirection;1.;1.;;;-1", "angleListId1");
    expectInvalid("AeroBodyProp2;1;ZDirection;1.;1.;;;;-1", "angleListId2");
    expectInvalid("AeroBodyProp2;1;ZDirection;1.;1.;;;;;-1", "angleList1FirstElem");
    expectInvalid("AeroBodyProp2;1;ZDirection;1.;1.;;;;;5;3", "angleList1LastElem"); // a range does not go backwards
    expectInvalid("AeroBodyProp2;1;ZDirection;1.;1.;;;;;;;5;3", "angleList2LastElem");
}

TEST_CASE("a CAERO3 has the wing and two control surface lists, and owns one ID per point", "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroMachBoxPanel3;2500;2003;0;24;35;;2.;0.;0.;80.;12.;110.;0.;80.\n"
        "AeroMachBoxPanel3;4301;1;;2;;;;;;10.\n"
        "AeroMachBoxPanel3;4302;5;6;7;8;9;1.5;2.5;3.5;40.;2.;60.;4.;30.\n"
        "AeroFactorList;24;0.;0.;0.5;0.5;1.;1.\n"
        "AeroFactorList;35;0.;0.5;0.5;0.5\n");
    const AeroMachBoxPanel3Card first = cardAt<AeroMachBoxPanel3Table>(model, 0);
    REQUIRE((first.id == 2500 && first.propId == 2003 && first.positionFrameId == 0 && first.wingPointListId == 24 &&
             first.controlPointListId1 == 35));
    REQUIRE((first.controlPointListId2 == 0 && first.point1 == std::array<double, 3>{2.0, 0.0, 0.0} &&
             first.rootChord == 80.0));
    REQUIRE((first.point4 == std::array<double, 3>{12.0, 110.0, 0.0} && first.tipChord == 80.0));
    const AeroMachBoxPanel3Card minimal = cardAt<AeroMachBoxPanel3Table>(model, 1);
    REQUIRE((minimal.wingPointListId == 2 && minimal.controlPointListId1 == 0 && minimal.rootChord == 10.0 &&
             minimal.tipChord == 0.0)); // a triangle
    const AeroMachBoxPanel3Card full = cardAt<AeroMachBoxPanel3Table>(model, 2);
    REQUIRE((full.positionFrameId == 6 && full.wingPointListId == 7 && full.controlPointListId1 == 8 &&
             full.controlPointListId2 == 9));
    REQUIRE(
        (full.point1 == std::array<double, 3>{1.5, 2.5, 3.5} && full.point4 == std::array<double, 3>{2.0, 60.0, 4.0}));
    requireRoundTrip(model);

    // The points are numbered from the EID, the pairs of LISTW first and then those of LISTC1 and LISTC2: (6 + 4) / 2
    // for the first card, and one ID at least for a card whose lists are missing.
    const AeroBoxRanges ranges(model);
    REQUIRE(ranges.ranges().size() == 3);
    REQUIRE((ranges.ranges()[0].panelId == 2500 && ranges.ranges()[0].boxCount == 5));
    REQUIRE((ranges.ranges()[1].panelId == 4301 && ranges.ranges()[1].boxCount == 1));

    // The PAERO3, the system CP and the three lists; a blank CP and a blank list are not visited.
    REQUIRE(referencesOf<AeroMachBoxPanel3Table>(model, 0).fields ==
            std::vector<std::string>{"propId", "wingPointListId", "controlPointListId1"});
    const RecordingVisitor every = referencesOf<AeroMachBoxPanel3Table>(model, 2);
    REQUIRE(every.fields == std::vector<std::string>{"propId", "positionFrameId", "wingPointListId",
                                                     "controlPointListId1", "controlPointListId2"});
    REQUIRE(every.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroProperty, ReferenceTarget::Coordinate,
                                                          ReferenceTarget::AeroFactor, ReferenceTarget::AeroFactor,
                                                          ReferenceTarget::AeroFactor});
    Model resolved =
        readBulk("AeroMachBoxProp3;1;10;0\nAeroFactorList;2;0.;0.;1.;1.\nAeroMachBoxPanel3;100;1;;2;;;;;;10.");
    REQUIRE(unresolvedIn(resolved) == 0);

    Renumbering::apply(model, RenumberSpace::AeroFactor, Renumbering::offset(100));
    REQUIRE((cardAt<AeroMachBoxPanel3Table>(model, 0).wingPointListId == 124 &&
             cardAt<AeroMachBoxPanel3Table>(model, 0).controlPointListId1 == 135));
    Renumbering::apply(model, RenumberSpace::AeroElement, Renumbering::offset(10));
    REQUIRE(cardAt<AeroMachBoxPanel3Table>(model, 0).id == 2510);

    // The CAERO cards share one ID space.
    REQUIRE_THROWS_AS(readBulk("AeroMachBoxPanel3;5;1;;2;;;;;;10.\nAeroMachBoxPanel3;5;1;;2;;;;;;10."),
                      base::DeckError);
    REQUIRE_THROWS_AS(readBulk("AeroMachBoxPanel3;5;1;;2;;;;;;10.\nAeroBody2;5;1;;2;1;;;1;;;;5."), base::DeckError);

    expectInvalid("AeroMachBoxPanel3;0;1;;2;;;;;;10.", "id");
    expectInvalid("AeroMachBoxPanel3;1;0;;2;;;;;;10.", "propId");
    expectInvalid("AeroMachBoxPanel3;1;1;-1;2;;;;;;10.", "positionFrameId");
    expectInvalid("AeroMachBoxPanel3;1;1;;;;;;;;10.", "wingPointListId"); // the list of the wing is required
    expectInvalid("AeroMachBoxPanel3;1;1;;2;-3;;;;;10.", "controlPointListId1");
    expectInvalid("AeroMachBoxPanel3;1;1;;2;;-3;;;;10.", "controlPointListId2");
    expectInvalid("AeroMachBoxPanel3;1;1;;2;;;1.;2.;3.", "rootChord"); // the chord is required
    expectInvalid("AeroMachBoxPanel3;1;1;;2;;;;;;0.", "rootChord");
    expectInvalid("AeroMachBoxPanel3;1;1;;2;;;;;;-1.", "rootChord");
    expectInvalid("AeroMachBoxPanel3;1;1;;2;;;;;;10.;;;;-1.", "tipChord");
}

TEST_CASE("a CAERO4 is a panel of strips: NSPAN or LSPAN, the chords of a trapezoid, one ID a strip",
          "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroStripPanel4;6200;6004;120;;318;0.;0.;0.;1.5;0.3;2.;0.;0.9\n"
        "AeroStripPanel4;4402;2;3;4;5;1.5;2.5;3.5;10.;2.;30.;4.;8.\n"
        "AeroStripPanel4;4403;1;;2;;0.;0.;0.;5.;1.;4.;0.;0.\n"
        "AeroFactorList;318;0.;0.25;0.5;1.\n"
        "RectFrame2;120;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n");
    const AeroStripPanel4Card first = cardAt<AeroStripPanel4Table>(model, 0);
    REQUIRE((first.id == 6200 && first.propId == 6004 && first.positionFrameId == 120 && first.spanBoxCount == 0 &&
             first.spanDivisionListId == 318));
    REQUIRE((first.point1 == std::array<double, 3>{0.0, 0.0, 0.0} && first.rootChord == 1.5));
    REQUIRE((first.point4 == std::array<double, 3>{0.3, 2.0, 0.0} && first.tipChord == 0.9));
    const AeroStripPanel4Card full = cardAt<AeroStripPanel4Table>(model, 1); // both NSPAN and LSPAN are kept
    REQUIRE((full.positionFrameId == 3 && full.spanBoxCount == 4 && full.spanDivisionListId == 5 &&
             full.point1[2] == 3.5 && full.point4[1] == 30.0));
    const AeroStripPanel4Card triangle = cardAt<AeroStripPanel4Table>(model, 2);
    REQUIRE((triangle.spanBoxCount == 2 && triangle.spanDivisionListId == 0 && triangle.rootChord == 5.0 &&
             triangle.tipChord == 0.0));
    requireRoundTrip(model);

    // One strip a division of the span: three from the four points of the AEFACT, the numbers of the others.
    const AeroBoxRanges ranges(model);
    REQUIRE(ranges.ranges().size() == 3);
    REQUIRE((ranges.ranges()[0].panelId == 4402 && ranges.ranges()[0].boxCount == 4));
    REQUIRE((ranges.ranges()[1].panelId == 4403 && ranges.ranges()[1].boxCount == 2));
    REQUIRE((ranges.ranges()[2].panelId == 6200 && ranges.ranges()[2].boxCount == 3));

    const RecordingVisitor listed = referencesOf<AeroStripPanel4Table>(model, 0);
    REQUIRE(listed.fields == std::vector<std::string>{"propId", "positionFrameId", "spanDivisionListId"});
    REQUIRE(listed.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroProperty, ReferenceTarget::Coordinate,
                                                           ReferenceTarget::AeroFactor});
    REQUIRE(referencesOf<AeroStripPanel4Table>(model, 2).fields == std::vector<std::string>{"propId"});
    REQUIRE(unresolvedIn(model) == 5); // the PAERO4 of the three cards, and the AEFACT 5 and the system 3 of the second

    Renumbering::apply(model, RenumberSpace::AeroFactor, Renumbering::offset(100));
    REQUIRE(cardAt<AeroStripPanel4Table>(model, 0).spanDivisionListId == 418);
    Renumbering::apply(model, RenumberSpace::AeroElement, Renumbering::offset(10000));
    REQUIRE((cardAt<AeroStripPanel4Table>(model, 0).id == 16200 && cardAt<AeroStripPanel4Table>(model, 2).id == 14403));

    // A CAERO1 and a CAERO4 cannot have the same ID.
    REQUIRE_THROWS_AS(readBulk("AeroStripPanel4;5;1;;2;;;;;1.\nAeroLiftingPanel1;5;1;;1;1;;;1;0.;0.;0.;1.;0.;1.;0.;1."),
                      base::DeckError);

    expectInvalid("AeroStripPanel4;0;1;;2;;;;;1.", "id");
    expectInvalid("AeroStripPanel4;1;0;;2;;;;;1.", "propId");
    expectInvalid("AeroStripPanel4;1;1;-1;2;;;;;1.", "positionFrameId");
    expectInvalid("AeroStripPanel4;1;1;;;;;;;1.", "spanBoxCount"); // neither a number nor a list of strips
    expectInvalid("AeroStripPanel4;1;1;;-2;;;;;1.", "spanBoxCount");
    expectInvalid("AeroStripPanel4;1;1;;2;-5;;;;1.", "spanDivisionListId");
    expectInvalid("AeroStripPanel4;1;1;;2;;;;;0.", "rootChord"); // both chords zero: no area
    expectInvalid("AeroStripPanel4;1;1;;2;;;;;-1.;;;;1.", "rootChord");
    expectInvalid("AeroStripPanel4;1;1;;2;;;;;1.;;;;-1.", "tipChord");
}

TEST_CASE("a CAERO5 adds the theory and the thickness integrals to the strips of a CAERO4", "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroPistonPanel5;7000;6004;120;;318;0;0;0.;0.;0.;1.5;0.3;2.;0.;0.9\n"
        "AeroPistonPanel5;4502;2;3;4;5;2;6;1.5;2.5;3.5;10.;2.;30.;4.;8.\n"
        "AeroPistonPanel5;4503;1;;2;;1;;0.;0.;0.;5.;1.;4.;0.;0.\n"
        "AeroFactorList;318;0.;0.25;0.5;1.\n"
        "AeroFactorList;6;0.;0.1;0.2\n");
    const AeroPistonPanel5Card first = cardAt<AeroPistonPanel5Table>(model, 0);
    REQUIRE((first.id == 7000 && first.propId == 6004 && first.positionFrameId == 120 && first.spanBoxCount == 0 &&
             first.spanDivisionListId == 318));
    REQUIRE(
        (first.theory == 0 && first.thicknessIntegralListId == 0 && first.rootChord == 1.5 && first.tipChord == 0.9));
    const AeroPistonPanel5Card full = cardAt<AeroPistonPanel5Table>(model, 1);
    REQUIRE((full.spanBoxCount == 4 && full.spanDivisionListId == 5 && full.theory == 2 &&
             full.thicknessIntegralListId == 6));
    REQUIRE(
        (full.point1 == std::array<double, 3>{1.5, 2.5, 3.5} && full.point4 == std::array<double, 3>{2.0, 30.0, 4.0}));
    REQUIRE(cardAt<AeroPistonPanel5Table>(model, 2).theory == 1);
    requireRoundTrip(model);

    const AeroBoxRanges ranges(model);
    REQUIRE(ranges.ranges().size() == 3);
    REQUIRE((ranges.ranges()[0].panelId == 4502 && ranges.ranges()[0].boxCount == 4));
    REQUIRE((ranges.ranges()[2].panelId == 7000 && ranges.ranges()[2].boxCount == 3));

    // The thickness integrals are an AEFACT; the theory is a number and names nothing.
    const RecordingVisitor thick = referencesOf<AeroPistonPanel5Table>(model, 1);
    REQUIRE(thick.fields ==
            std::vector<std::string>{"propId", "positionFrameId", "spanDivisionListId", "thicknessIntegralListId"});
    REQUIRE(thick.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroProperty, ReferenceTarget::Coordinate,
                                                          ReferenceTarget::AeroFactor, ReferenceTarget::AeroFactor});
    REQUIRE(referencesOf<AeroPistonPanel5Table>(model, 0).fields ==
            std::vector<std::string>{"propId", "positionFrameId", "spanDivisionListId"});

    Renumbering::apply(model, RenumberSpace::AeroFactor, Renumbering::offset(100));
    REQUIRE((cardAt<AeroPistonPanel5Table>(model, 0).spanDivisionListId == 418 &&
             cardAt<AeroPistonPanel5Table>(model, 1).thicknessIntegralListId == 106));

    REQUIRE_THROWS_AS(readBulk("AeroPistonPanel5;5;1;;2;;;;;;;1.\nAeroStripPanel4;5;1;;2;;;;;1."), base::DeckError);

    expectInvalid("AeroPistonPanel5;0;1;;2;;;;;;;1.", "id");
    expectInvalid("AeroPistonPanel5;1;0;;2;;;;;;;1.", "propId");
    expectInvalid("AeroPistonPanel5;1;1;;;;;;;;;1.", "spanBoxCount");
    expectInvalid("AeroPistonPanel5;1;1;;2;;3;;;;;1.",
                  "theory"); // piston theory, or van Dyke's theory without or with a correction
    expectInvalid("AeroPistonPanel5;1;1;;2;;-1;;;;;1.", "theory");
    expectInvalid("AeroPistonPanel5;1;1;;2;;;-4;;;;1.", "thicknessIntegralListId");
    expectInvalid("AeroPistonPanel5;1;1;;2;;;;;;;0.", "rootChord");
    expectInvalid("AeroPistonPanel5;1;1;;2;;;;;;;1.;;;;-1.", "tipChord");
}
