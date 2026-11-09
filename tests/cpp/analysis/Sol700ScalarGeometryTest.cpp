// The geometry of the scalar springs and dampers of SOL 700 (CELAS1D, CELAS2D, CDAMP1D, CDAMP2D): the length and the
// centre of the segment between their two points, and nothing else (the format reference documents no area, volume or
// frame for them).

#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>

using namespace oor;
using namespace oor::analysis;
using namespace oor::model;
using Catch::Approx;
using oor::testing::readModelText;

namespace
{

ElementValues geometryOf(const Model& model, base::EntityId eid)
{
    const auto location = model.locate(base::IdSpace::Element, eid);
    REQUIRE(location.has_value());
    return ElementGeometry::of(model, *location);
}

void requireVec(const base::Vec3& actual, double x, double y, double z)
{
    REQUIRE(actual.x == Approx(x).margin(1.0e-12));
    REQUIRE(actual.y == Approx(y).margin(1.0e-12));
    REQUIRE(actual.z == Approx(z).margin(1.0e-12));
}

// Two grid points 5 apart (3, 4, 0), a grid point at (0, 0, 2) and a scalar point, with the properties the elements
// name.
const char* const DECK =
    "Node;1;;0.;0.;0.\n"
    "Node;2;;3.;4.;0.\n"
    "Node;3;;0.;0.;2.\n"
    "ScalarPoint;50;51\n"
    "TableDynamic1;10;;;0.;0.;1.;100.;EndTable\n"
    "SpringPropTable;5;10\n"
    "DamperProp;6;0.5\n"
    "SpringDofPropertyExplicit;11;5;1;1;2;1\n"
    "SpringDofInlineExplicit;12;100.;1;1;2;1;7\n"
    "DamperDofPropertyExplicit;13;6;1;1;2;1\n"
    "DamperDofInlineExplicit;14;0.5;1;1;2;1\n"
    "SpringDofPropertyExplicit;21;5;1;1;50\n"
    "SpringDofInlineExplicit;22;100.;3;3;50\n"
    "DamperDofPropertyExplicit;23;6;50;;51\n"
    "DamperDofInlineExplicit;24;0.5;2;1\n"
    "SpringDofPropertyExplicit;25;5;3;1;3;3\n"
    "RectFrame2;7;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n";

} // namespace

TEST_CASE("the four SOL 700 scalar elements have the length and the centre of the segment between their points",
          "[analysis][geometry]")
{
    const Model model = readModelText(DECK);
    for (const base::EntityId eid : {11, 12, 13, 14})
    {
        INFO("element " << eid);
        const ElementValues values = geometryOf(model, eid);
        REQUIRE(values.length == Approx(5.0));
        requireVec(values.center, 1.5, 2.0, 0.0);
        // The format reference draws no area, volume, normal or frame for them.
        REQUIRE(std::isnan(values.area));
        REQUIRE(std::isnan(values.volume));
        REQUIRE(std::isnan(values.normal.x));
        REQUIRE(std::isnan(values.frame.values[0]));
    }
}

TEST_CASE("a SOL 700 scalar element with a scalar point or ground at one end sits at its grid point and has no length",
          "[analysis][geometry]")
{
    const Model model = readModelText(DECK);
    // A scalar point at the other end: the centre is the grid point, and there is no length.
    const ElementValues toScalar = geometryOf(model, 21);
    requireVec(toScalar.center, 0.0, 0.0, 0.0);
    REQUIRE(std::isnan(toScalar.length));
    requireVec(geometryOf(model, 22).center, 0.0, 0.0, 2.0);
    // Ground (a blank point) at the other end behaves the same way.
    const ElementValues grounded = geometryOf(model, 24);
    requireVec(grounded.center, 3.0, 4.0, 0.0);
    REQUIRE(std::isnan(grounded.length));
    // A spring between two components of the same grid point: both ends are there, so the segment has length zero.
    const ElementValues sameGrid = geometryOf(model, 25);
    requireVec(sameGrid.center, 0.0, 0.0, 2.0);
    REQUIRE(sameGrid.length == Approx(0.0).margin(1.0e-12));
}

TEST_CASE("a SOL 700 scalar element between two scalar points has no geometry at all", "[analysis][geometry]")
{
    const Model model = readModelText(DECK);
    const ElementValues values = geometryOf(model, 23);
    REQUIRE(std::isnan(values.center.x));
    REQUIRE(std::isnan(values.length));
}
