// Checks of the splines of the aeroelastic cards: SPLINE1 to SPLINE7 and SPLINEX. Each card is read with
// its fields in the places of the format reference, written back in the small format with its defaults left blank and
// its required continuation line written, and shown to list the references it has (and to follow the renumbering of
// what it names). The values the cards refuse are listed with the field that reports them.

#include "oor/base/text/NumberParser.hpp"
#include "oor/model/cards/aero/aeroBeamSpline2/AeroBeamSpline2Table.hpp"
#include "oor/model/cards/aero/aeroBeamSpline5/AeroBeamSpline5Table.hpp"
#include "oor/model/cards/aero/aeroFiniteBeamSpline7/AeroFiniteBeamSpline7Table.hpp"
#include "oor/model/cards/aero/aeroFiniteSurfaceSpline6/AeroFiniteSurfaceSpline6Table.hpp"
#include "oor/model/cards/aero/aeroSplineEquation3/AeroSplineEquation3Table.hpp"
#include "oor/model/cards/aero/aeroSurfaceSpline1/AeroSurfaceSpline1Table.hpp"
#include "oor/model/cards/aero/aeroSurfaceSpline4/AeroSurfaceSpline4Table.hpp"
#include "oor/model/cards/aero/aeroUserSpline/AeroUserSplineTable.hpp"
#include "support/AeroTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The panel the splines of the tests lie on: 10 boxes, numbered from 211 to 220 (the PAERO1 is not in the decks).
const char* const PANEL = "AeroLiftingPanel1;211;1;;5;2;;;1;0.;0.;0.;1.;0.;1.;0.;1.\n";

// The structural points a SET1 lists: 1 to 3.
const char* const POINTS = "IdListSet1;17;1;2;3\nNode;1\nNode;2\nNode;3\n";

} // namespace

TEST_CASE("the splines are one category, and each type refuses a repeated ID of its own", "[model][cards][aero]")
{
    for (const CardTypeInfo* info :
         {&AeroSurfaceSpline1Table::INFO, &AeroBeamSpline2Table::INFO, &AeroSplineEquation3Table::INFO,
          &AeroSurfaceSpline4Table::INFO, &AeroBeamSpline5Table::INFO, &AeroFiniteSurfaceSpline6Table::INFO,
          &AeroFiniteBeamSpline7Table::INFO, &AeroUserSplineTable::INFO})
    {
        REQUIRE(info->category == CardCategory::AeroSpline);
        REQUIRE(info->idSpace == base::IdSpace::PerCardType);
    }
    REQUIRE_THROWS_AS(readBulk("AeroSurfaceSpline1;2;211;214;219;17\nAeroSurfaceSpline1;2;211;214;219;17"),
                      base::DeckError);
    REQUIRE_THROWS_AS(readBulk("AeroUserSpline;71;GROUP;A;B\nAeroUserSpline;71;GROUP;A;B"), base::DeckError);
}

TEST_CASE(
    "a SPLINE1 has its fields in the places of the format reference and the defaults of the method, usage and counts",
    "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroSurfaceSpline1;2;211;214;219;17;0.\n"
        "AeroSurfaceSpline1;4;112;201;230;15;0.5;FiniteSurfaceSpline;Displacement;12;8\n"
        "AeroSurfaceSpline1;5;113;301;301;16;;ThinPlateSpline;Force\n"
        "AeroSurfaceSpline1;6;114;401;410;17;;FiniteSurfaceSpline;;;20");
    const AeroSurfaceSpline1Card minimal = cardAt<AeroSurfaceSpline1Table>(model, 0);
    REQUIRE((minimal.id == 2 && minimal.aeroPanelId == 211 && minimal.firstBox == 214 && minimal.lastBox == 219));
    REQUIRE((minimal.nodeSetId == 17 && minimal.linearFlexibility == 0.0));
    REQUIRE((minimal.method == AeroSurfaceSpline1Method::InfinitePlate && minimal.usage == SplineUsage::Both));
    REQUIRE((minimal.elemCountX == 10 && minimal.elemCountY == 10));
    const AeroSurfaceSpline1Card full = cardAt<AeroSurfaceSpline1Table>(model, 1);
    REQUIRE((full.id == 4 && full.aeroPanelId == 112 && full.firstBox == 201 && full.lastBox == 230 &&
             full.nodeSetId == 15));
    REQUIRE((full.linearFlexibility == 0.5 && full.method == AeroSurfaceSpline1Method::FinitePlate &&
             full.usage == SplineUsage::Displacement));
    REQUIRE((full.elemCountX == 12 && full.elemCountY == 8));
    // The words are read in either case, and a spline may move a single box.
    const AeroSurfaceSpline1Card lower = cardAt<AeroSurfaceSpline1Table>(model, 2);
    REQUIRE((lower.method == AeroSurfaceSpline1Method::ThinPlate && lower.usage == SplineUsage::Force));
    REQUIRE((lower.firstBox == 301 && lower.lastBox == 301));
    const AeroSurfaceSpline1Card oneCount = cardAt<AeroSurfaceSpline1Table>(model, 3);
    REQUIRE((oneCount.elemCountX == 10 && oneCount.elemCountY == 20));
    requireRoundTrip(model);

    // The defaults are blank, and the continuation line appears only when a count is not 10.

    expectInvalid("AeroSurfaceSpline1;0;211;214;219;17", "id");
    expectInvalid("AeroSurfaceSpline1;2;0;214;219;17", "aeroPanelId");
    expectInvalid("AeroSurfaceSpline1;2;211;0;219;17", "firstBox");
    expectInvalid("AeroSurfaceSpline1;2;211;214;0;17", "lastBox");
    expectInvalid("AeroSurfaceSpline1;2;211;219;214;17", "lastBox"); // the last box is not below the first
    expectInvalid("AeroSurfaceSpline1;2;211;214;219", "nodeSetId");
    expectInvalid("AeroSurfaceSpline1;2;211;214;219;17;-1.", "linearFlexibility");
    expectInvalid("AeroSurfaceSpline1;2;211;214;219;17;;XYZ", "method");
    expectInvalid("AeroSurfaceSpline1;2;211;214;219;17;;;SOME", "usage");
    expectInvalid("AeroSurfaceSpline1;2;211;214;219;17;;FiniteSurfaceSpline;;0;10", "elemCountX");
    expectInvalid("AeroSurfaceSpline1;2;211;214;219;17;;FiniteSurfaceSpline;;10;-2", "elemCountY");
}

TEST_CASE("a SPLINE1 names a CAERO, two boxes and a set, and follows their renumbering", "[model][cards][aero]")
{
    Model model = readBulk(std::string(PANEL) + POINTS + "AeroSurfaceSpline1;2;211;214;219;17;0.");
    REQUIRE(unresolvedIn(model) == 1); // the PAERO1 of the panel only
    RecordingVisitor visitor;
    model.table<AeroSurfaceSpline1Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"aeroPanelId", "firstBox", "lastBox", "nodeSetId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroElement, ReferenceTarget::AeroBox,
                                                            ReferenceTarget::AeroBox, ReferenceTarget::Set});

    // The boxes follow the panel that owns them.
    Renumbering::apply(model, RenumberSpace::AeroElement, Renumbering::offset(1000));
    const AeroSurfaceSpline1Card moved = cardAt<AeroSurfaceSpline1Table>(model, 0);
    REQUIRE((moved.aeroPanelId == 1211 && moved.firstBox == 1214 && moved.lastBox == 1219));
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(100));
    REQUIRE(cardAt<AeroSurfaceSpline1Table>(model, 0).nodeSetId == 117);
}

TEST_CASE("a SPLINE2 keeps the sign of its rotational flexibilities and always writes its continuation line",
          "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroBeamSpline2;21;9;13;27;64;0.;1.;3;2.\n"
        "AeroBeamSpline2;22;801;601;800;1001;;;;0.;0.\n"
        "AeroBeamSpline2;23;802;901;928;1002;0.1;2.5;7;-1.;0.5;Force\n"
        "AeroBeamSpline2;24;803;31;31;1003");
    const AeroBeamSpline2Card example = cardAt<AeroBeamSpline2Table>(model, 0);
    REQUIRE((example.id == 21 && example.aeroPanelId == 9 && example.firstBox == 13 && example.lastBox == 27 &&
             example.nodeSetId == 64));
    REQUIRE((example.linearFlexibility == 0.0 && example.bendingTorsionRatio == 1.0 && example.frameId == 3));
    REQUIRE((example.rotationFlexibilityX == 2.0 && example.rotationFlexibilityY == 0.0 &&
             example.usage == SplineUsage::Both)); // a blank DTHY is 0.
    const AeroBeamSpline2Card minimal = cardAt<AeroBeamSpline2Table>(model, 1);
    REQUIRE((minimal.frameId == 0 && minimal.bendingTorsionRatio == 1.0 && minimal.rotationFlexibilityX == 0.0 &&
             minimal.rotationFlexibilityY == 0.0));
    const AeroBeamSpline2Card full = cardAt<AeroBeamSpline2Table>(model, 2);
    REQUIRE((full.linearFlexibility == 0.1 && full.bendingTorsionRatio == 2.5 && full.frameId == 7));
    REQUIRE((full.rotationFlexibilityX == -1.0 && full.rotationFlexibilityY == 0.5 &&
             full.usage == SplineUsage::Force)); // a negative value is kept
    const AeroBeamSpline2Card single = cardAt<AeroBeamSpline2Table>(model, 3);
    REQUIRE((single.firstBox == 31 && single.lastBox == 31 && single.rotationFlexibilityX == 0.0 &&
             single.rotationFlexibilityY == 0.0)); // no continuation
    requireRoundTrip(model);

    // The continuation line is required: the flexibilities are written even when they are zero, and USAGE is field 12.

    expectInvalid("AeroBeamSpline2;0;9;13;27;64", "id");
    expectInvalid("AeroBeamSpline2;21;0;13;27;64", "aeroPanelId");
    expectInvalid("AeroBeamSpline2;21;9;0;27;64", "firstBox");
    expectInvalid("AeroBeamSpline2;21;9;27;13;64", "lastBox"); // the last element is not below the first
    expectInvalid("AeroBeamSpline2;21;9;13;27", "nodeSetId");
    expectInvalid("AeroBeamSpline2;21;9;13;27;64;-1.", "linearFlexibility");
    expectInvalid("AeroBeamSpline2;21;9;13;27;64;;0.", "bendingTorsionRatio");
    expectInvalid("AeroBeamSpline2;21;9;13;27;64;;;-1", "frameId");
    expectInvalid("AeroBeamSpline2;21;9;13;27;64;;;;;;SOME", "usage");
}

TEST_CASE("a SPLINE2 names a CAERO, two boxes, a set and a coordinate system", "[model][cards][aero]")
{
    Model model = readBulk(std::string(PANEL) + POINTS +
                           "RectFrame2;3;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
                           "AeroBeamSpline2;21;211;214;219;17;0.;1.;3;2.\n"
                           "AeroBeamSpline2;22;211;211;211;17;;;;0.;0.");
    REQUIRE(unresolvedIn(model) == 1); // the PAERO1 of the panel only
    RecordingVisitor visitor;
    model.table<AeroBeamSpline2Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"aeroPanelId", "firstBox", "lastBox", "nodeSetId", "frameId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroElement, ReferenceTarget::AeroBox,
                                                            ReferenceTarget::AeroBox, ReferenceTarget::Set,
                                                            ReferenceTarget::Coordinate});
    visitor = RecordingVisitor{};
    model.table<AeroBeamSpline2Table>().visitReferences(1, visitor); // the basic coordinate system is not a reference
    REQUIRE(visitor.fields == std::vector<std::string>{"aeroPanelId", "firstBox", "lastBox", "nodeSetId"});

    Renumbering::apply(model, RenumberSpace::AeroElement, Renumbering::offset(1000));
    const AeroBeamSpline2Card moved = cardAt<AeroBeamSpline2Table>(model, 0);
    REQUIRE((moved.aeroPanelId == 1211 && moved.firstBox == 1214 && moved.lastBox == 1219));
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    REQUIRE(cardAt<AeroBeamSpline2Table>(model, 0).frameId == 13);
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(100));
    REQUIRE(cardAt<AeroBeamSpline2Table>(model, 0).nodeSetId == 117);
}

TEST_CASE("a SPLINE3 is a constraint on one box component with independent points four fields apart",
          "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroSplineEquation3;7400;105;108;4;6;2;1.;;47;3;-1.\n"
        "AeroSplineEquation3;7401;108;110;3;6;2;1.;Force;47;3;-1.;51;0;0.5;52;1;2.5\n"
        "AeroSplineEquation3;7402;109;111;5;12;2;-1.5;Displacement");
    const AeroSplineEquation3Card example = cardAt<AeroSplineEquation3Table>(model, 0);
    REQUIRE((example.id == 7400 && example.aeroPanelId == 105 && example.boxId == 108 && example.component == 4));
    REQUIRE(example.usage == SplineUsage::Both);
    REQUIRE(example.points == std::vector<AeroSplineEquation3Point>{{6, 2, 1.0}, {47, 3, -1.0}});
    // The groups start at fields 5, 9, 13 and 17; the fourth field of the first is USAGE and the others are blank
    // cells.
    const AeroSplineEquation3Card many = cardAt<AeroSplineEquation3Table>(model, 1);
    REQUIRE(many.usage == SplineUsage::Force);
    REQUIRE(many.points == std::vector<AeroSplineEquation3Point>{
                               {6, 2, 1.0}, {47, 3, -1.0}, {51, 0, 0.5}, {52, 1, 2.5}}); // 51 is a scalar point
    const AeroSplineEquation3Card single = cardAt<AeroSplineEquation3Table>(model, 2);
    REQUIRE((single.usage == SplineUsage::Displacement &&
             single.points == std::vector<AeroSplineEquation3Point>{{12, 2, -1.5}}));
    requireRoundTrip(model);

    expectInvalid("AeroSplineEquation3;0;105;108;4;6;2;1.", "id");
    expectInvalid("AeroSplineEquation3;7400;0;108;4;6;2;1.", "aeroPanelId");
    expectInvalid("AeroSplineEquation3;7400;105;0;4;6;2;1.", "boxId");
    expectInvalid("AeroSplineEquation3;7400;105;108;7;6;2;1.", "component");
    expectInvalid("AeroSplineEquation3;7400;105;108;0;6;2;1.", "component");
    expectInvalid("AeroSplineEquation3;7400;105;108;4", "node1"); // at least one independent point
    expectInvalid("AeroSplineEquation3;7400;105;108;4;0;2;1.", "node1");
    expectInvalid("AeroSplineEquation3;7400;105;108;4;6;;1.", "components1");
    expectInvalid("AeroSplineEquation3;7400;105;108;4;6;7;1.", "components1");
    expectInvalid("AeroSplineEquation3;7400;105;108;4;6;2", "coefficient1"); // a group is complete or blank
    expectInvalid("AeroSplineEquation3;7400;105;108;4;6;2;1.;SOME", "usage");
    expectInvalid("AeroSplineEquation3;7400;105;108;4;6;2;1.;;47;8;-1.", "components");
    expectInvalid("AeroSplineEquation3;7400;105;108;4;6;2;1.;;47;3", "coefficients");
}

TEST_CASE("a SPLINE3 names a CAERO, a box and its independent points", "[model][cards][aero]")
{
    Model model = readBulk(std::string(PANEL) + "Node;6\nNode;47\nAeroSplineEquation3;7400;211;214;4;6;2;1.;;47;3;-1.");
    REQUIRE(unresolvedIn(model) == 1); // the PAERO1 of the panel only
    RecordingVisitor visitor;
    model.table<AeroSplineEquation3Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"aeroPanelId", "boxId", "nodes", "nodes"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroElement, ReferenceTarget::AeroBox,
                                                            ReferenceTarget::ScalarOrGridPoint,
                                                            ReferenceTarget::ScalarOrGridPoint});

    Renumbering::apply(model, RenumberSpace::AeroElement, Renumbering::offset(1000));
    const AeroSplineEquation3Card moved = cardAt<AeroSplineEquation3Table>(model, 0);
    REQUIRE((moved.aeroPanelId == 1211 && moved.boxId == 1214));
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    const AeroSplineEquation3Card renumbered = cardAt<AeroSplineEquation3Table>(model, 0);
    REQUIRE((renumbered.points[0].node == 1006 && renumbered.points[1].node == 1047));
    REQUIRE((renumbered.points[0].component == 2 &&
             renumbered.points[1].coefficient == -1.0)); // only the points are renumbered
}

TEST_CASE("a SPLINE4 lists its boxes in an AELIST and has the radial interpolation spline of the format reference",
          "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroSurfaceSpline4;31;211;214;17;0.;InfinitePlateSpline\n"
        "AeroSurfaceSpline4;32;112;116;15;0.2;FiniteSurfaceSpline;Displacement;12;8\n"
        "AeroSurfaceSpline4;33;113;117;16;0.;RigidSurfaceSpline;Force;;;WendlandFunction0;2.5\n"
        "AeroSurfaceSpline4;34;114;118;17;;RigidSurfaceSpline;;;;;1.5\n"
        "AeroSurfaceSpline4;35;115;119;18;;ThinPlateSpline\n"
        "AeroSurfaceSpline4;36;116;120;19;;InfinitePlateSpline;;;;WendlandFunction0;2.5");
    const AeroSurfaceSpline4Card example = cardAt<AeroSurfaceSpline4Table>(model, 0);
    REQUIRE(
        (example.id == 31 && example.aeroPanelId == 211 && example.aeroBoxListId == 214 && example.nodeSetId == 17));
    REQUIRE((example.linearFlexibility == 0.0 && example.method == AeroSurfaceSpline4Method::InfinitePlate &&
             example.usage == SplineUsage::Both));
    REQUIRE((example.elemCountX == 10 && example.elemCountY == 10 &&
             example.functionKind == SplineRadialFunction::Wendland2));
    REQUIRE(!example.coreRadius);
    const AeroSurfaceSpline4Card finite = cardAt<AeroSurfaceSpline4Table>(model, 1);
    REQUIRE((finite.linearFlexibility == 0.2 && finite.method == AeroSurfaceSpline4Method::FinitePlate &&
             finite.usage == SplineUsage::Displacement));
    REQUIRE((finite.elemCountX == 12 && finite.elemCountY == 8));
    const AeroSurfaceSpline4Card radial = cardAt<AeroSurfaceSpline4Table>(model, 2);
    REQUIRE((radial.method == AeroSurfaceSpline4Method::RadialInterpolation && radial.usage == SplineUsage::Force));
    REQUIRE((radial.functionKind == SplineRadialFunction::Wendland0 && radial.coreRadius == 2.5));
    const AeroSurfaceSpline4Card defaultFunction = cardAt<AeroSurfaceSpline4Table>(model, 3);
    REQUIRE((defaultFunction.functionKind == SplineRadialFunction::Wendland2 && defaultFunction.coreRadius == 1.5));
    REQUIRE(cardAt<AeroSurfaceSpline4Table>(model, 4).method == AeroSurfaceSpline4Method::ThinPlate);
    // The radial fields are kept on a card whose method ignores them.
    const AeroSurfaceSpline4Card ignored = cardAt<AeroSurfaceSpline4Table>(model, 5);
    REQUIRE((ignored.method == AeroSurfaceSpline4Method::InfinitePlate &&
             ignored.functionKind == SplineRadialFunction::Wendland0));
    REQUIRE(ignored.coreRadius == 2.5);
    requireRoundTrip(model);

    // Field 4 is a blank cell; the continuation line has the counts, then FTYPE and RCORE.

    expectInvalid("AeroSurfaceSpline4;0;211;214;17", "id");
    expectInvalid("AeroSurfaceSpline4;31;0;214;17", "aeroPanelId");
    expectInvalid("AeroSurfaceSpline4;31;211;0;17", "aeroBoxListId");
    expectInvalid("AeroSurfaceSpline4;31;211;214;0", "nodeSetId");
    expectInvalid("AeroSurfaceSpline4;31;211;214;17;-1.", "linearFlexibility");
    expectInvalid("AeroSurfaceSpline4;31;211;214;17;;XYZ", "method");
    expectInvalid("AeroSurfaceSpline4;31;211;214;17;;;SOME", "usage");
    expectInvalid("AeroSurfaceSpline4;31;211;214;17;;FiniteSurfaceSpline;;0;10", "elemCountX");
    expectInvalid("AeroSurfaceSpline4;31;211;214;17;;FiniteSurfaceSpline;;10;0", "elemCountY");
    expectInvalid("AeroSurfaceSpline4;31;211;214;17;;RigidSurfaceSpline",
                  "coreRadius"); // the radius of support has no default
    expectInvalid("AeroSurfaceSpline4;31;211;214;17;;RigidSurfaceSpline;;;;;0.", "coreRadius");
    expectInvalid("AeroSurfaceSpline4;31;211;214;17;;RigidSurfaceSpline;;;;WF1;2.", "functionKind");
}

TEST_CASE("a SPLINE4 names a CAERO, an AELIST and a set, and follows their renumbering", "[model][cards][aero]")
{
    Model model = readBulk(std::string(PANEL) + POINTS +
                           "AeroBoxList;214;214;Through;219\nAeroSurfaceSpline4;31;211;214;17;0.;InfinitePlateSpline");
    REQUIRE(unresolvedIn(model) == 1); // the PAERO1 of the panel only
    RecordingVisitor visitor;
    model.table<AeroSurfaceSpline4Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"aeroPanelId", "aeroBoxListId", "nodeSetId"});
    // format reference SPLINE4: CAERO is the aerodynamic component, a CAERO panel or the component of an aerodynamic
    // mesh.
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroPanelOrComponent,
                                                            ReferenceTarget::AeroList, ReferenceTarget::Set});

    Renumbering::apply(model, RenumberSpace::AeroList, Renumbering::offset(100));
    REQUIRE(cardAt<AeroSurfaceSpline4Table>(model, 0).aeroBoxListId == 314);
    Renumbering::apply(model, RenumberSpace::AeroElement, Renumbering::offset(1000));
    REQUIRE(cardAt<AeroSurfaceSpline4Table>(model, 0).aeroPanelId == 1211);
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(100));
    REQUIRE(cardAt<AeroSurfaceSpline4Table>(model, 0).nodeSetId == 117);
}

TEST_CASE("a SPLINE4 may name the component of the AEQUAD4 of an aerodynamic mesh, which no renumbering moves",
          "[model][cards][aero]")
{
    Model model = readBulk(std::string(POINTS) +
                           "AeroNode;501;;0.;0.;0.\n"
                           "AeroNode;502;;1.;0.;0.\n"
                           "AeroNode;503;;1.;1.;0.\n"
                           "AeroNode;504;;0.;1.;0.\n"
                           "AeroQuad4;601;7;501;502;503;504\n"
                           "AeroBoxList;214;501;Through;504\n"
                           "AeroSurfaceSpline4;31;7;214;17;0.;InfinitePlateSpline");
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::AeroElement, Renumbering::offset(1000));
    REQUIRE(cardAt<AeroSurfaceSpline4Table>(model, 0).aeroPanelId == 7);
    Model missing =
        readBulk(std::string(POINTS) +
                 "AeroNode;501;;0.;0.;0.\nAeroBoxList;214;501\nAeroSurfaceSpline4;31;7;214;17;0.;InfinitePlateSpline");
    REQUIRE(unresolvedIn(missing) == 1);
}

TEST_CASE("a SPLINE5 ignores its unused fields, keeps the sign of its flexibilities and has the radial spline",
          "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroBeamSpline5;41;9;13;64;;;3;2.;;Both\n"
        "AeroBeamSpline5;42;108001;1;29;0.;1.;101;-1.;-1.;Both\n"
        "AeroBeamSpline5;43;109001;4;33;0.5;2.;9;-1.;0.25;Force;RigidSurfaceSpline;WendlandFunction0;3.5\n"
        "AeroBeamSpline5;44;110001;2;34;;;;0.;0.\n"
        "AeroBeamSpline5;45;111001;5;35;;;1;1.;1.;;RigidSurfaceSpline;;2.");
    const AeroBeamSpline5Card example = cardAt<AeroBeamSpline5Table>(model, 0);
    REQUIRE((example.id == 41 && example.aeroPanelId == 9 && example.aeroBoxListId == 13 && example.nodeSetId == 64));
    REQUIRE((example.linearFlexibility == 0.0 && example.bendingTorsionRatio == 1.0 && example.frameId == 3));
    REQUIRE((example.rotationFlexibilityX == 2.0 && example.rotationFlexibilityY == 0.0 &&
             example.usage == SplineUsage::Both)); // a blank DTHY is 0.
    REQUIRE((example.method == AeroBeamSpline5Method::Beam && example.functionKind == SplineRadialFunction::Wendland2));
    REQUIRE(!example.coreRadius);
    const AeroBeamSpline5Card unused = cardAt<AeroBeamSpline5Table>(model, 1);
    REQUIRE((unused.aeroPanelId == 108001 && unused.frameId == 101 && unused.rotationFlexibilityX == -1.0 &&
             unused.rotationFlexibilityY == -1.0));
    REQUIRE((unused.usage == SplineUsage::Both && unused.method == AeroBeamSpline5Method::Beam));
    const AeroBeamSpline5Card full = cardAt<AeroBeamSpline5Table>(model, 2);
    REQUIRE((full.linearFlexibility == 0.5 && full.bendingTorsionRatio == 2.0 && full.frameId == 9 &&
             full.rotationFlexibilityX == -1.0 && full.rotationFlexibilityY == 0.25));
    REQUIRE((full.usage == SplineUsage::Force && full.method == AeroBeamSpline5Method::RadialInterpolation));
    REQUIRE((full.functionKind == SplineRadialFunction::Wendland0 && full.coreRadius == 3.5));
    const AeroBeamSpline5Card minimal = cardAt<AeroBeamSpline5Table>(model, 3);
    REQUIRE((minimal.frameId == 0 && minimal.rotationFlexibilityX == 0.0 && minimal.rotationFlexibilityY == 0.0 &&
             minimal.method == AeroBeamSpline5Method::Beam));
    const AeroBeamSpline5Card radial = cardAt<AeroBeamSpline5Table>(model, 4);
    REQUIRE((radial.method == AeroBeamSpline5Method::RadialInterpolation && radial.coreRadius == 2.0 &&
             radial.frameId == 1));
    REQUIRE(radial.functionKind == SplineRadialFunction::Wendland2);
    requireRoundTrip(model);

    // The continuation line is required: DTHX and DTHY are written even when zero; USAGE, METH, FTYPE and RCORE are
    // fields 12, 13, 15 and 16.

    expectInvalid("AeroBeamSpline5;0;9;13;64", "id");
    expectInvalid("AeroBeamSpline5;41;0;13;64", "aeroPanelId");
    expectInvalid("AeroBeamSpline5;41;9;0;64", "aeroBoxListId");
    expectInvalid("AeroBeamSpline5;41;9;13;0", "nodeSetId");
    expectInvalid("AeroBeamSpline5;41;9;13;64;-1.", "linearFlexibility");
    expectInvalid("AeroBeamSpline5;41;9;13;64;;0.", "bendingTorsionRatio");
    expectInvalid("AeroBeamSpline5;41;9;13;64;;;-1", "frameId");
    expectInvalid("AeroBeamSpline5;41;9;13;64;;;;0.;0.;SOME", "usage");
    expectInvalid("AeroBeamSpline5;41;9;13;64;;;;0.;0.;;SPIRAL", "method");
    expectInvalid("AeroBeamSpline5;41;9;13;64;;;;0.;0.;;;WF1", "functionKind");
    expectInvalid("AeroBeamSpline5;41;9;13;64;;;;0.;0.;;RigidSurfaceSpline",
                  "coreRadius"); // the radius of support has no default
    expectInvalid("AeroBeamSpline5;41;9;13;64;;;;0.;0.;;RigidSurfaceSpline;;-2.", "coreRadius");
}

TEST_CASE("a SPLINE5 names a CAERO, an AELIST, a set and a coordinate system", "[model][cards][aero]")
{
    Model model = readBulk(std::string(PANEL) + POINTS +
                           "RectFrame2;3;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
                           "AeroBoxList;214;214;Through;219\n"
                           "AeroBeamSpline5;41;211;214;17;;;3;1.;1.");
    REQUIRE(unresolvedIn(model) == 1); // the PAERO1 of the panel only
    RecordingVisitor visitor;
    model.table<AeroBeamSpline5Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"aeroPanelId", "aeroBoxListId", "nodeSetId", "frameId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroElement, ReferenceTarget::AeroList,
                                                            ReferenceTarget::Set, ReferenceTarget::Coordinate});

    Renumbering::apply(model, RenumberSpace::AeroList, Renumbering::offset(100));
    REQUIRE(cardAt<AeroBeamSpline5Table>(model, 0).aeroBoxListId == 314);
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    REQUIRE(cardAt<AeroBeamSpline5Table>(model, 0).frameId == 13);
}

TEST_CASE("a SPLINE6 needs a VSLIST unless the surface is the aerodynamic panel", "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroFiniteSurfaceSpline6;51;9;13;64;;FiniteSurfaceSpline6;Displacement;AeroMesh;3;5;5;VibrationSimple6\n"
        "AeroFiniteSurfaceSpline6;52;8;13;61\n"
        "AeroFiniteSurfaceSpline6;53;9;14;62;0.1;FiniteSurfaceSpline3;Force;Structure;15;5;6;VibrationSimple3;0.2;"
        "CircularBias;4;50.;6;0.5\n"
        "AeroFiniteSurfaceSpline6;54;;16;63;;;;AeroMesh;17");
    const AeroFiniteSurfaceSpline6Card example = cardAt<AeroFiniteSurfaceSpline6Table>(model, 0);
    REQUIRE((example.id == 51 && example.aeroPanelId == 9 && example.aeroBoxListId == 13 && example.nodeSetId == 64));
    REQUIRE((example.linearFlexibility == 0.0 &&
             example.method == AeroFiniteSurfaceSpline6Method::TranslationsAndRotations));
    REQUIRE((example.usage == SplineUsage::Displacement &&
             example.surfaceKind == AeroFiniteSurfaceSpline6SurfaceType::Aero));
    REQUIRE((example.surfaceElemListId == 3 && example.independentPointCount == 5 && example.dependentPointCount == 5));
    REQUIRE((example.surfaceMethod == AeroFiniteSurfaceSpline6SurfaceMethod::TranslationsAndRotations &&
             example.rotationFlexibility == 0.0));
    REQUIRE((example.connectionMethod == AeroFiniteSurfaceSpline6Connection::NodalProximity && example.nodeCount == 1));
    REQUIRE((example.projectionTolerance == 100.0 && example.cycleCount == 3 && example.augmentationWeight == 0.0));
    const AeroFiniteSurfaceSpline6Card minimal = cardAt<AeroFiniteSurfaceSpline6Table>(model, 1);
    REQUIRE((minimal.surfaceElemListId == 0 && minimal.independentPointCount == 3 && minimal.dependentPointCount == 3 &&
             minimal.usage == SplineUsage::Both));
    const AeroFiniteSurfaceSpline6Card full = cardAt<AeroFiniteSurfaceSpline6Table>(model, 2);
    REQUIRE(
        (full.aeroPanelId == 9 && full.aeroBoxListId == 14 && full.nodeSetId == 62 && full.linearFlexibility == 0.1));
    REQUIRE((full.method == AeroFiniteSurfaceSpline6Method::Translations && full.usage == SplineUsage::Force));
    REQUIRE((full.surfaceKind == AeroFiniteSurfaceSpline6SurfaceType::Struc && full.surfaceElemListId == 15 &&
             full.independentPointCount == 5 && full.dependentPointCount == 6));
    REQUIRE(
        (full.surfaceMethod == AeroFiniteSurfaceSpline6SurfaceMethod::Translations && full.rotationFlexibility == 0.2));
    REQUIRE((full.connectionMethod == AeroFiniteSurfaceSpline6Connection::CircularBias && full.nodeCount == 4));
    REQUIRE((full.projectionTolerance == 50.0 && full.cycleCount == 6 && full.augmentationWeight == 0.5));
    const AeroFiniteSurfaceSpline6Card mesh =
        cardAt<AeroFiniteSurfaceSpline6Table>(model, 3); // an aerodynamic mesh: no CAERO, the surface is listed
    REQUIRE(
        (mesh.aeroPanelId == 0 && mesh.aeroBoxListId == 16 && mesh.nodeSetId == 63 && mesh.surfaceElemListId == 17));
    requireRoundTrip(model);

    expectInvalid("AeroFiniteSurfaceSpline6;0;9;13;64", "id");
    expectInvalid("AeroFiniteSurfaceSpline6;51;-1;13;64", "aeroPanelId");
    expectInvalid("AeroFiniteSurfaceSpline6;51;9;0;64", "aeroBoxListId");
    expectInvalid("AeroFiniteSurfaceSpline6;51;9;13;0", "nodeSetId");
    expectInvalid("AeroFiniteSurfaceSpline6;51;9;13;64;-1.", "linearFlexibility");
    expectInvalid("AeroFiniteSurfaceSpline6;51;9;13;64;;FPS4", "method");
    expectInvalid("AeroFiniteSurfaceSpline6;51;9;13;64;;;SOME", "usage");
    expectInvalid("AeroFiniteSurfaceSpline6;51;9;13;64;;;;SURFACE", "surfaceKind");
    expectInvalid("AeroFiniteSurfaceSpline6;51;;13;64", "surfaceElemListId"); // no CAERO to stand for the surface
    expectInvalid("AeroFiniteSurfaceSpline6;51;9;13;64;;;;Structure",
                  "surfaceElemListId"); // a structural surface is always listed
    expectInvalid("AeroFiniteSurfaceSpline6;51;9;13;64;;;;;2;0", "independentPointCount");
    expectInvalid("AeroFiniteSurfaceSpline6;51;9;13;64;;;;;;;0", "dependentPointCount");
    expectInvalid("AeroFiniteSurfaceSpline6;51;9;13;64;;;;;;;;VS9", "surfaceMethod");
    expectInvalid("AeroFiniteSurfaceSpline6;51;9;13;64;;;;;;;;;-1.", "rotationFlexibility");
    expectInvalid("AeroFiniteSurfaceSpline6;51;9;13;64;;;;;;;;;;SPIRAL", "connectionMethod");
    expectInvalid("AeroFiniteSurfaceSpline6;51;9;13;64;;;;;;;;;;CircularBias;0", "nodeCount");
    expectInvalid("AeroFiniteSurfaceSpline6;51;9;13;64;;;;;;;;;;CircularBias;2;50.;0", "cycleCount");
    expectInvalid("AeroFiniteSurfaceSpline6;51;9;13;64;;;;;;;;;;CircularBias;2;50.;3;-0.5", "augmentationWeight");
}

TEST_CASE("a SPLINE6 names a CAERO, two AELIST cards and a set", "[model][cards][aero]")
{
    Model model = readBulk(std::string(PANEL) + POINTS +
                           "AeroBoxList;12;214;Through;219\n"
                           "AeroBoxList;2;211;Through;213\n"
                           "AeroFiniteSurfaceSpline6;51;211;12;17;;;;AeroMesh;2\n"
                           "AeroFiniteSurfaceSpline6;52;;12;17;;;;;2");
    REQUIRE(unresolvedIn(model) == 1); // the PAERO1 of the panel only
    RecordingVisitor visitor;
    model.table<AeroFiniteSurfaceSpline6Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields ==
            std::vector<std::string>{"aeroPanelId", "aeroBoxListId", "nodeSetId", "surfaceElemListId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroElement, ReferenceTarget::AeroList,
                                                            ReferenceTarget::Set, ReferenceTarget::AeroList});
    visitor = RecordingVisitor{};
    model.table<AeroFiniteSurfaceSpline6Table>().visitReferences(1, visitor); // a blank CAERO is not a reference
    REQUIRE(visitor.fields == std::vector<std::string>{"aeroBoxListId", "nodeSetId", "surfaceElemListId"});

    Renumbering::apply(model, RenumberSpace::AeroList, Renumbering::offset(100));
    REQUIRE((cardAt<AeroFiniteSurfaceSpline6Table>(model, 0).aeroBoxListId == 112 &&
             cardAt<AeroFiniteSurfaceSpline6Table>(model, 0).surfaceElemListId == 102));
}

TEST_CASE("a SPLINE7 may leave its CAERO blank and has the defaults of its beam", "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroFiniteBeamSpline7;61;9;13;64;;;3;Both;FiniteBeamSpline6\n"
        "AeroFiniteBeamSpline7;62;;13;61\n"
        "AeroFiniteBeamSpline7;63;9;14;62;0.1;2.5;4;Force;FiniteBeamSpline3;0.2;1.5;0.05");
    const AeroFiniteBeamSpline7Card example = cardAt<AeroFiniteBeamSpline7Table>(model, 0);
    REQUIRE((example.id == 61 && example.aeroPanelId == 9 && example.aeroBoxListId == 13 && example.nodeSetId == 64 &&
             example.frameId == 3));
    REQUIRE((example.usage == SplineUsage::Both &&
             example.method == AeroFiniteBeamSpline7Method::TranslationsAndRotations));
    REQUIRE((example.linearFlexibility == 0.0 && example.bendingTorsionRatio == 1.0 &&
             example.rotationFlexibility == 0.0 && example.bendingExtensionRatio == 1.0));
    REQUIRE(example.minBeamLengthRatio == 0.01);
    const AeroFiniteBeamSpline7Card minimal = cardAt<AeroFiniteBeamSpline7Table>(model, 1);
    REQUIRE(
        (minimal.aeroPanelId == 0 && minimal.aeroBoxListId == 13 && minimal.nodeSetId == 61 && minimal.frameId == 0));
    const AeroFiniteBeamSpline7Card full = cardAt<AeroFiniteBeamSpline7Table>(model, 2);
    REQUIRE((full.aeroPanelId == 9 && full.linearFlexibility == 0.1 && full.bendingTorsionRatio == 2.5 &&
             full.frameId == 4));
    REQUIRE((full.usage == SplineUsage::Force && full.method == AeroFiniteBeamSpline7Method::Translations));
    REQUIRE((full.rotationFlexibility == 0.2 && full.bendingExtensionRatio == 1.5 && full.minBeamLengthRatio == 0.05));
    requireRoundTrip(model);

    expectInvalid("AeroFiniteBeamSpline7;0;9;13;64", "id");
    expectInvalid("AeroFiniteBeamSpline7;61;-1;13;64", "aeroPanelId");
    expectInvalid("AeroFiniteBeamSpline7;61;9;0;64", "aeroBoxListId");
    expectInvalid("AeroFiniteBeamSpline7;61;9;13;0", "nodeSetId");
    expectInvalid("AeroFiniteBeamSpline7;61;9;13;64;-1.", "linearFlexibility");
    expectInvalid("AeroFiniteBeamSpline7;61;9;13;64;;0.", "bendingTorsionRatio");
    expectInvalid("AeroFiniteBeamSpline7;61;9;13;64;;;-2", "frameId");
    expectInvalid("AeroFiniteBeamSpline7;61;9;13;64;;;;SOME", "usage");
    expectInvalid("AeroFiniteBeamSpline7;61;9;13;64;;;;;FBS4", "method");
    expectInvalid("AeroFiniteBeamSpline7;61;9;13;64;;;;;;-1.", "rotationFlexibility");
    expectInvalid("AeroFiniteBeamSpline7;61;9;13;64;;;;;;;0.", "bendingExtensionRatio");
    expectInvalid("AeroFiniteBeamSpline7;61;9;13;64;;;;;;;;0.", "minBeamLengthRatio");
}

TEST_CASE("a SPLINE7 names a CAERO, an AELIST, a set and a coordinate system", "[model][cards][aero]")
{
    Model model = readBulk(std::string(PANEL) + POINTS +
                           "RectFrame2;3;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
                           "AeroBoxList;214;214;Through;219\n"
                           "AeroFiniteBeamSpline7;61;211;214;17;;;3\n"
                           "AeroFiniteBeamSpline7;62;;214;17");
    REQUIRE(unresolvedIn(model) == 1); // the PAERO1 of the panel only
    RecordingVisitor visitor;
    model.table<AeroFiniteBeamSpline7Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"aeroPanelId", "aeroBoxListId", "nodeSetId", "frameId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroElement, ReferenceTarget::AeroList,
                                                            ReferenceTarget::Set, ReferenceTarget::Coordinate});
    visitor = RecordingVisitor{};
    model.table<AeroFiniteBeamSpline7Table>().visitReferences(
        1, visitor); // a blank CAERO and the basic system are not references
    REQUIRE(visitor.fields == std::vector<std::string>{"aeroBoxListId", "nodeSetId"});

    Renumbering::apply(model, RenumberSpace::AeroList, Renumbering::offset(100));
    REQUIRE((cardAt<AeroFiniteBeamSpline7Table>(model, 0).aeroBoxListId == 314 &&
             cardAt<AeroFiniteBeamSpline7Table>(model, 1).aeroBoxListId == 314));
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    REQUIRE(cardAt<AeroFiniteBeamSpline7Table>(model, 0).frameId == 13);
}

TEST_CASE("a SPLINEX names its components, which are labels, and the data of its program", "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroUserSpline;71;SPGROUPA;SKINDP;SKINIP;;;Both;101;201;301\n"
        "AeroUserSpline;72;USRSPL;;;PANLDE;PANLIE\n"
        "AeroUserSpline;73;SPGROUPB;GRIDD;GRIDI;ELEMD;ELEMI;Force;102;202;302\n"
        "AeroUserSpline;74;SPGROUPC;gwd;GWI;;;Displacement");
    const AeroUserSplineCard example = cardAt<AeroUserSplineTable>(model, 0);
    REQUIRE((example.id == 71 && example.groupName == "SPGROUPA" && example.dependentNodeComponent == "SKINDP"));
    REQUIRE((example.independentNodeComponent == "SKINIP" && example.dependentElemComponent.empty() &&
             example.independentElemComponent.empty()));
    REQUIRE((example.usage == SplineUsage::Both && example.aeroBoxListId == 101 && example.factorListId == 201));
    REQUIRE(example.labelListId == 301);
    const AeroUserSplineCard elements = cardAt<AeroUserSplineTable>(model, 1);
    REQUIRE((elements.groupName == "USRSPL" && elements.dependentNodeComponent.empty() &&
             elements.independentNodeComponent.empty()));
    REQUIRE((elements.dependentElemComponent == "PANLDE" && elements.independentElemComponent == "PANLIE"));
    REQUIRE((elements.aeroBoxListId == 0 && elements.factorListId == 0 && elements.labelListId == 0));
    const AeroUserSplineCard full = cardAt<AeroUserSplineTable>(model, 2);
    REQUIRE((full.dependentNodeComponent == "GRIDD" && full.independentNodeComponent == "GRIDI" &&
             full.dependentElemComponent == "ELEMD" && full.independentElemComponent == "ELEMI"));
    REQUIRE((full.usage == SplineUsage::Force && full.aeroBoxListId == 102 && full.factorListId == 202 &&
             full.labelListId == 302));
    REQUIRE((cardAt<AeroUserSplineTable>(model, 3).dependentNodeComponent == "gwd")); // a name is kept as it is written
    REQUIRE(cardAt<AeroUserSplineTable>(model, 3).usage == SplineUsage::Displacement);
    requireRoundTrip(model);

    expectInvalid("AeroUserSpline;0;SPGROUPA;SKINDP;SKINIP", "id");
    expectInvalid("AeroUserSpline;71;;SKINDP;SKINIP", "groupName");
    requireLiteralRoundTrip("AeroUserSpline;71;SPGROUPAB;SKINDP;SKINIP", "SPGROUPAB");
    requireLiteralRoundTrip("AeroUserSpline;71;SPGROUPA;SKINDP;Long mixedCase.label", "Long mixedCase.label");
    expectInvalid("AeroUserSpline;71;SPGROUPA;;SKINIP",
                  "dependentNodeComponent"); // the dependent mesh has neither points nor elements
    expectInvalid("AeroUserSpline;71;SPGROUPA;SKINDP", "independentNodeComponent"); // the independent mesh has neither
    expectInvalid("AeroUserSpline;71;SPGROUPA;SKINDP;SKINIP;;;SOME", "usage");
    expectInvalid("AeroUserSpline;71;SPGROUPA;SKINDP;SKINIP;;;;-1", "aeroBoxListId");
}

TEST_CASE("a SPLINEX names an AELIST and an AEFACT; the components and the AELISTC are not references",
          "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroBoxList;101;1;2\n"
        "AeroFactorList;201;0.;1.\n"
        "AeroUserSpline;71;SPGROUPA;SKINDP;SKINIP;;;Both;101;201;301\n"
        "AeroUserSpline;72;USRSPL;;;PANLDE;PANLIE");
    REQUIRE(unresolvedIn(model) == 0);
    RecordingVisitor visitor;
    model.table<AeroUserSplineTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"aeroBoxListId", "factorListId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroList, ReferenceTarget::AeroFactor});
    visitor = RecordingVisitor{};
    model.table<AeroUserSplineTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields.empty());

    Renumbering::apply(model, RenumberSpace::AeroList, Renumbering::offset(100));
    Renumbering::apply(model, RenumberSpace::AeroFactor, Renumbering::offset(1000));
    const AeroUserSplineCard moved = cardAt<AeroUserSplineTable>(model, 0);
    REQUIRE((moved.aeroBoxListId == 201 && moved.factorListId == 1201));
    REQUIRE((moved.labelListId == 301 && moved.dependentNodeComponent == "SKINDP")); // the plain ID and the labels stay
}
