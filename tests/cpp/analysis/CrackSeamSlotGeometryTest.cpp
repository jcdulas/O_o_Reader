// The geometry of the crack tip, seam, slot and spring elements: CSPR and CSEAM are segments,
// CSLOT3 and CSLOT4 are surfaces with no thickness, CRAC2D and CRAC3D have their nodes and no quantity.

#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/model/cards/elements/acousticSlot3/AcousticSlot3Table.hpp"
#include "oor/model/cards/elements/acousticSlot4/AcousticSlot4Table.hpp"
#include "oor/model/cards/elements/crackTip2D/CrackTip2DTable.hpp"
#include "oor/model/cards/elements/crackTip3D/CrackTip3DTable.hpp"
#include "oor/model/cards/elements/seam/SeamTable.hpp"
#include "oor/model/cards/elements/springOffsetExplicit/SpringOffsetExplicitTable.hpp"
#include "support/ModelRecord.hpp"
#include "support/ModelTestSupport.hpp"
#include "support/SlotRecord.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::analysis;
using namespace oor::model;
using Catch::Approx;
using oor::testing::readModelText;

namespace
{

constexpr double EPS = 1.0e-12;

ElementValues geometryOf(const Model& model, base::EntityId eid)
{
    const auto location = model.locate(base::IdSpace::Element, eid);
    REQUIRE(location.has_value());
    return ElementGeometry::of(model, *location);
}

void requireVec(const base::Vec3& actual, double x, double y, double z)
{
    REQUIRE(actual.x == Approx(x).margin(EPS));
    REQUIRE(actual.y == Approx(y).margin(EPS));
    REQUIRE(actual.z == Approx(z).margin(EPS));
}

// Every quantity of the element is undefined (NaN).
void requireNothingDefined(const ElementValues& values)
{
    REQUIRE_FALSE(isDefined(values.center));
    REQUIRE_FALSE(isDefined(values.normal));
    REQUIRE(std::isnan(values.area));
    REQUIRE(std::isnan(values.volume));
    REQUIRE(std::isnan(values.length));
    REQUIRE(std::isnan(values.thickness));
}

// The record of a node: id, x, y, z (in the basic system).
std::string grid(int id, double x, double y, double z)
{
    return oor::testing::modelRecord("Node", id, "", x, y, z);
}

} // namespace

TEST_CASE("the six card types of the crack, seam, slot and spring elements have geometry",
          "[analysis][geometry][crack-seam-slot]")
{
    REQUIRE(ElementGeometry::supports(typeIndexOf<CrackTip2DTable>()));
    REQUIRE(ElementGeometry::supports(typeIndexOf<CrackTip3DTable>()));
    REQUIRE(ElementGeometry::supports(typeIndexOf<SeamTable>()));
    REQUIRE(ElementGeometry::supports(typeIndexOf<AcousticSlot3Table>()));
    REQUIRE(ElementGeometry::supports(typeIndexOf<AcousticSlot4Table>()));
    REQUIRE(ElementGeometry::supports(typeIndexOf<SpringOffsetExplicitTable>()));
}

TEST_CASE("a CSPR is a segment between its two grid points", "[analysis][geometry][crack-seam-slot]")
{
    const Model model = readModelText(grid(1, 0, 0, 0) + grid(2, 3, 4, 0) + "SpringOffsetExplicit;10;22;1;2\n");
    const ElementValues values = geometryOf(model, 10);
    requireVec(values.center, 1.5, 2.0, 0.0);
    REQUIRE(values.length == Approx(5.0));
    // The format reference gives no section: no area, no volume, no normal.
    REQUIRE_FALSE(isDefined(values.normal));
    REQUIRE(std::isnan(values.area));
    REQUIRE(std::isnan(values.volume));
}

TEST_CASE("a CSEAM given by grid points is a segment, and one given by coordinates has no geometry",
          "[analysis][geometry][crack-seam-slot]")
{
    const Model model = readModelText(grid(1, 1, 1, 0) + grid(2, 1, 1, 12) + "Seam;20;5;;;43;48;;;1;;2\n" +
                                      "Seam;21;5;;;43;48;;;;0.;;0.;0.;3.;4.;0.\n");
    // The format reference: the length of the seam is the distance between GS and GE.
    const ElementValues segment = geometryOf(model, 20);
    REQUIRE(segment.length == Approx(12.0));
    requireVec(segment.center, 1.0, 1.0, 6.0);
    REQUIRE(std::isnan(segment.area));
    REQUIRE(std::isnan(segment.volume));
    // The alternate format has no grid point: its ends are coordinates, which no quantity is made from.
    requireNothingDefined(geometryOf(model, 21));
}

TEST_CASE("a CSLOT3 and a CSLOT4 are surfaces: area, normal by the right-hand rule and centre, no thickness",
          "[analysis][geometry][crack-seam-slot]")
{
    // The slot points are GRIDS cards in the format reference; they share the node IDs, so grid points stand for them
    // here.
    const Model model =
        readModelText(grid(1, 0, 0, 0) + grid(2, 2, 0, 0) + grid(3, 0, 2, 0) + grid(4, 1, 0, 0) + grid(5, 1, 1, 0) +
                      grid(6, 0, 1, 0) + "AcousticSlot3;30;1;2;3\nAcousticSlot3;31;1;3;2\n" +
                      "AcousticSlot4;32;1;4;5;6\n");
    const ElementValues triangle = geometryOf(model, 30);
    REQUIRE(triangle.area == Approx(2.0));
    requireVec(triangle.normal, 0.0, 0.0, 1.0);
    requireVec(triangle.center, 2.0 / 3.0, 2.0 / 3.0, 0.0);
    REQUIRE(std::isnan(triangle.thickness));
    REQUIRE(std::isnan(triangle.volume));
    REQUIRE(std::isnan(triangle.length));
    // The other way round: the opposite normal.
    requireVec(geometryOf(model, 31).normal, 0.0, 0.0, -1.0);

    const ElementValues square = geometryOf(model, 32);
    REQUIRE(square.area == Approx(1.0));
    requireVec(square.normal, 0.0, 0.0, 1.0);
    requireVec(square.center, 0.5, 0.5, 0.0);
    REQUIRE(std::isnan(square.thickness));
    REQUIRE(std::isnan(square.volume));
}

TEST_CASE("a slot point that is not a grid point has no position, and one that no card defines is an error",
          "[analysis][geometry][crack-seam-slot]")
{
    // The GRIDS of a slot are not grid points: where a card of the node space exists with the ID (a scalar point stands
    // for it here) the element has no position and every quantity is NaN.
    const Model model = readModelText(
        "ScalarPoint;1;2;3;4\nAcousticSlot3;40;1;2;3\nAcousticSlot4;41;1;2;3;4\nAcousticSlot3;42;1;2;99\n");
    requireNothingDefined(geometryOf(model, 40));
    requireNothingDefined(geometryOf(model, 41));
    // A point that nothing defines is refused, as for every element.
    REQUIRE_THROWS_AS(geometryOf(model, 42), GeometryError);
}

TEST_CASE("the elements of a slot cavity lie in the plane of their GRIDS points: x is R, y is Z",
          "[analysis][geometry][crack-seam-slot]")
{
    // GRIDS id R Z W: the slot width W is not a coordinate, so a point with a width and one without are in the same
    // plane. (R is not zero for a GRIDS.)
    const Model model = readModelText(
        "AcousticSlotControl;0.003;150.;3;0.75;6\n"
        "AcousticSlotSurfacePoint;1;1.;0.;0.5\n"
        "AcousticSlotSurfacePoint;2;3.;0.\n"
        "AcousticSlotSurfacePoint;3;1.;2.;0.1\n"
        "AcousticSlotSurfacePoint;4;2.;0.\n"
        "AcousticSlotSurfacePoint;5;2.;1.\n"
        "AcousticSlotSurfacePoint;6;1.;1.\n"
        "AcousticSlot3;30;1;2;3\n"
        "AcousticSlot4;32;1;4;5;6\n"
        "AcousticSlot3;33;1;2;99\n");
    const ElementValues triangle = geometryOf(model, 30);
    REQUIRE(triangle.area == Approx(2.0));
    requireVec(triangle.normal, 0.0, 0.0, 1.0);
    requireVec(triangle.center, 5.0 / 3.0, 2.0 / 3.0, 0.0);
    const ElementValues square = geometryOf(model, 32);
    REQUIRE(square.area == Approx(1.0));
    requireVec(square.center, 1.5, 0.5, 0.0);
    // A slot point that no card defines is still an error.
    REQUIRE_THROWS_AS(geometryOf(model, 33), GeometryError);
}

TEST_CASE("the crack tip elements have their nodes and no quantity", "[analysis][geometry][crack-seam-slot]")
{
    std::string deck;
    for (int id = 1; id <= 28; ++id)
        deck += grid(id, id, 0.0, 0.0);
    deck += "CrackTip2D;50;1;1;2;3;4;5;6;7;8;9;10\n";
    // A CRAC3D with the required G1 to G10 and G19 to G28: G number n is slot n + 2.
    std::vector<std::string> fields(30);
    fields[0] = "51";
    fields[1] = "1";
    for (int number = 1; number <= 10; ++number)
        fields[static_cast<std::size_t>(number) + 1] = std::to_string(number);
    for (int number = 19; number <= 28; ++number)
        fields[static_cast<std::size_t>(number) + 1] = std::to_string(number);
    deck += oor::testing::slotRecord("CrackTip3D", fields);
    const Model model = readModelText(deck);
    requireNothingDefined(geometryOf(model, 50));
    requireNothingDefined(geometryOf(model, 51));
}
