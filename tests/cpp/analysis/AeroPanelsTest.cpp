// The boxes of the aerodynamic panels, checked against numbers worked out by hand on a swept wing: the
// corners of each box, its area, centre, control and load points, the numbering of the boxes, the division lists of an
// AEFACT, the coordinate system of the leading edge points and the aerodynamic coordinate system of the AERO card.

#include "oor/analysis/coordinates/CoordinateErrors.hpp"
#include "oor/analysis/geometry/AeroPanels.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace oor;
using namespace oor::analysis;
using namespace oor::model;
using Catch::Approx;
using oor::testing::readModelText;

namespace
{

constexpr double TOLERANCE = 1.0e-12;

Model readBulk(const std::string& cards)
{
    return readModelText(cards, true);
}

void requirePoint(const base::Vec3& actual, double x, double y, double z)
{
    REQUIRE(actual.x == Approx(x).margin(TOLERANCE));
    REQUIRE(actual.y == Approx(y).margin(TOLERANCE));
    REQUIRE(actual.z == Approx(z).margin(TOLERANCE));
}

// A swept wing: the leading edge goes from (0, 0, 0) to (1, 4, 0) and both chords are 2, cut in 2 boxes along the span
// and 4 along the chord. Every box is 0.5 long at mid span and 2 wide, so its area is 1 and the eight boxes make the 8
// of the panel.
const char* const SWEPT_WING = "AeroLiftingPanel1;1000;1;;2;4;;;1;0.;0.;0.;2.;1.;4.;0.;2.";

} // namespace

TEST_CASE("a CAERO1 is cut into boxes numbered along the chord first, then span by span", "[analysis][aero]")
{
    const Model model = readBulk(SWEPT_WING);
    const AeroPanelBoxes boxes = aeroPanelBoxes(model, 1000);
    REQUIRE((boxes.eid == 1000 && boxes.kind == AeroPanelKind::AeroLiftingPanel1));
    REQUIRE((boxes.chordBoxes == 4 && boxes.spanBoxes == 2 && boxes.boxCount() == 8));
    REQUIRE(boxes.boxId == std::vector<base::EntityId>{1000, 1001, 1002, 1003, 1004, 1005, 1006, 1007});
    REQUIRE(boxes.chordIndex == std::vector<std::int32_t>{0, 1, 2, 3, 0, 1, 2, 3});
    REQUIRE(boxes.spanIndex == std::vector<std::int32_t>{0, 0, 0, 0, 1, 1, 1, 1});
    double total = 0.0;
    for (const double area : boxes.area)
    {
        REQUIRE(area == Approx(1.0).margin(TOLERANCE));
        total += area;
    }
    REQUIRE(total == Approx(8.0)); // the trapezoid of the panel: (2 + 2) / 2 times 4
    for (const double chord : boxes.chord)
        REQUIRE(chord == Approx(0.5).margin(TOLERANCE));
    for (const base::Vec3& normal : boxes.normal)
        requirePoint(normal, 0.0, 0.0, 1.0); // the outboard edge is on the +y side: the normal is up
}

TEST_CASE("the corners, the centre, the control point and the load point of a box", "[analysis][aero]")
{
    const Model model = readBulk(SWEPT_WING);
    const AeroPanelBoxes boxes = aeroPanelBoxes(model, 1000);
    // The first box: leading edge inboard, trailing edge inboard, trailing edge outboard, leading edge outboard.
    requirePoint(boxes.corner1[0], 0.0, 0.0, 0.0);
    requirePoint(boxes.corner2[0], 0.5, 0.0, 0.0);
    requirePoint(boxes.corner3[0], 1.0, 2.0, 0.0);
    requirePoint(boxes.corner4[0], 0.5, 2.0, 0.0);
    requirePoint(boxes.center[0], 0.5, 1.0, 0.0);
    // Mid span of the first box is y = 1, where the leading edge is at x = 0.25 and the chord is 2. The box covers the
    // first quarter of the chord: its three-quarter point is at 0.1875 of the chord, its quarter point at 0.0625.
    requirePoint(boxes.controlPoint[0], 0.25 + 0.1875 * 2.0, 1.0, 0.0);
    requirePoint(boxes.loadPoint[0], 0.25 + 0.0625 * 2.0, 1.0, 0.0);
    // The last box: the trailing edge outboard corner is the corner 3 of the panel.
    requirePoint(boxes.corner1[7], 2.0, 2.0, 0.0);
    requirePoint(boxes.corner2[7], 2.5, 2.0, 0.0);
    requirePoint(boxes.corner3[7], 3.0, 4.0, 0.0);
    requirePoint(boxes.corner4[7], 2.5, 4.0, 0.0);
    // In every box the control point is half a box chord behind the load point (three quarters against one quarter).
    REQUIRE(boxes.controlPoint[2].x - boxes.loadPoint[2].x == Approx(0.25).margin(TOLERANCE));
}

TEST_CASE("the divisions of a CAERO1 can be the points of an AEFACT, used as given", "[analysis][aero]")
{
    // The span is cut at 0, a quarter and the whole of it; the chord has one box. The panel is a rectangle 2 deep from
    // y 2 to 6.
    const Model model =
        readBulk("AeroLiftingPanel1;2000;2;;;1;10;;2;1.;2.;3.;2.;1.;6.;3.;2.\nAeroFactorList;10;0.;0.25;1.");
    const AeroPanelBoxes boxes = aeroPanelBoxes(model, 2000);
    REQUIRE((boxes.chordBoxes == 1 && boxes.spanBoxes == 2));
    REQUIRE(boxes.boxId == std::vector<base::EntityId>{2000, 2001});
    requirePoint(boxes.corner1[0], 1.0, 2.0, 3.0);
    requirePoint(boxes.corner4[0], 1.0, 3.0, 3.0);
    requirePoint(boxes.corner3[1], 3.0, 6.0, 3.0);
    REQUIRE(boxes.area[0] == Approx(2.0).margin(TOLERANCE));
    REQUIRE(boxes.area[1] == Approx(6.0).margin(TOLERANCE));
    // Division points that do not start at 0 and end at 1 leave the corners of the panel outside the boxes.
    const Model inner =
        readBulk("AeroLiftingPanel1;2100;2;;;1;10;;2;0.;0.;0.;2.;0.;4.;0.;2.\nAeroFactorList;10;0.25;0.5");
    const AeroPanelBoxes part = aeroPanelBoxes(inner, 2100);
    requirePoint(part.corner1[0], 0.0, 1.0, 0.0);
    requirePoint(part.corner4[0], 0.0, 2.0, 0.0);
}

TEST_CASE("the leading edge points go through their system and the chords along the aerodynamic x axis",
          "[analysis][aero]")
{
    // CP 3 is the basic system moved to (10, 20, 30); the aerodynamic system 5 has its x axis along the basic y axis.
    // The leading edge runs up from (10, 20, 30) to (10, 20, 34) and the chords are 2 along +y: a rectangle in the
    // plane x = 10.
    const Model model = readBulk(
        "RectFrame2;3;;10.;20.;30.;10.;20.;31.;11.;20.;30.\n"
        "RectFrame2;5;;0.;0.;0.;0.;0.;1.;0.;1.;0.\n"
        "AeroUnsteadyParams;5;100.;1.;1.2\n"
        "AeroLiftingPanel1;3000;1;3;1;1;;;1;0.;0.;0.;2.;0.;0.;4.;2.");
    const AeroPanelBoxes boxes = aeroPanelBoxes(model, 3000);
    REQUIRE(boxes.boxCount() == 1);
    requirePoint(boxes.corner1[0], 10.0, 20.0, 30.0);
    requirePoint(boxes.corner2[0], 10.0, 22.0, 30.0);
    requirePoint(boxes.corner3[0], 10.0, 22.0, 34.0);
    requirePoint(boxes.corner4[0], 10.0, 20.0, 34.0);
    REQUIRE(boxes.area[0] == Approx(8.0).margin(TOLERANCE));
    requirePoint(boxes.normal[0], 1.0, 0.0, 0.0);
    requirePoint(boxes.center[0], 10.0, 21.0, 32.0);

    // Without the AERO card the chords run along the basic x axis.
    const Model plain = readBulk(
        "RectFrame2;3;;10.;20.;30.;10.;20.;31.;11.;20.;30.\nAeroLiftingPanel1;3000;1;3;1;1;;;1;0.;0.;0.;2.;0.;0.;4.;"
        "2.");
    requirePoint(aeroPanelBoxes(plain, 3000).corner2[0], 12.0, 20.0, 30.0);
}

TEST_CASE("a panel that cannot be cut is reported, the others are still made", "[analysis][aero]")
{
    const Model model = readBulk(std::string(SWEPT_WING) +
                                 "\n"
                                 "AeroLiftingPanel1;2000;1;;;1;99;;1;0.;0.;0.;1.;0.;1.;0.;1.\n"
                                 "AeroLiftingPanel1;2500;1;;1;1;;;1;0.;0.;0.;1.;0.;1.;0.;1.");
    REQUIRE_THROWS_AS(aeroPanelBoxes(model, 2000), GeometryError); // the AEFACT 99 does not exist
    REQUIRE_THROWS_AS(aeroPanelBoxes(model, 77), GeometryError);   // no such panel
    std::vector<base::EntityId> skipped;
    const std::vector<AeroPanelBoxes> all = allAeroPanelBoxes(model, &skipped);
    REQUIRE(skipped == std::vector<base::EntityId>{2000});
    REQUIRE(all.size() == 2);
    REQUIRE((all[0].eid == 1000 && all[1].eid == 2500));
    REQUIRE(all[1].boxCount() == 1);
}

TEST_CASE("the span between the chords is the part of the leading edge that is not along the flow", "[analysis][aero]")
{
    // The aerodynamic system 5 has its x axis along the basic y axis: the flow goes along +y, so the chords run along y
    // and the distance between two chords is measured across y. A leading edge from (0, 0, 0) to (3, 4, 0) is 3 apart
    // (the 4 along y is a sweep), whereas with the flow along x it is 4 apart.
    const Model along = readBulk("RectFrame2;5;;0.;0.;0.;0.;0.;1.;0.;1.;0.\nAeroUnsteadyParams;5;100.;1.;1.2");
    PanelFrame frame(along);
    requirePoint(frame.flowDirection(), 0.0, 1.0, 0.0);
    REQUIRE(frame.spanBetween(base::Vec3{0.0, 0.0, 0.0}, base::Vec3{3.0, 4.0, 0.0}) == Approx(3.0).margin(TOLERANCE));

    const Model plain = readBulk("RectFrame2;3;;10.;20.;30.;10.;20.;31.;11.;20.;30.");
    PanelFrame basic(plain);
    requirePoint(basic.flowDirection(), 1.0, 0.0, 0.0);
    REQUIRE(basic.spanBetween(base::Vec3{0.0, 0.0, 0.0}, base::Vec3{3.0, 4.0, 0.0}) == Approx(4.0).margin(TOLERANCE));
    // a point given in the system 3 is moved to the basic system; the basic system leaves it alone
    requirePoint(basic.toBasic(3, base::Vec3{1.0, 2.0, 3.0}), 11.0, 22.0, 33.0);
    requirePoint(basic.toBasic(0, base::Vec3{1.0, 2.0, 3.0}), 1.0, 2.0, 3.0);
    REQUIRE_THROWS_AS(basic.toBasic(77, base::Vec3{}), CoordinateSystemError);
    // an aerodynamic system that does not exist is found when the flow is asked for
    const Model missing = readBulk("AeroUnsteadyParams;9;100.;1.;1.2");
    PanelFrame lost(missing);
    REQUIRE_THROWS_AS(lost.flowDirection(), CoordinateSystemError);
}
