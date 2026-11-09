// The geometry of the dummy plot elements PLOTEL3 and PLOTEL4: surfaces with no thickness, as the
// acoustic absorber CAABSF is: the centre, the normal by the right-hand rule on the order of the points and the area.
// The plot elements have an ID space of their own, so they are found by their card type.

#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/model/cards/elements/plotQuad4/PlotQuad4Table.hpp"
#include "oor/model/cards/elements/plotTria3/PlotTria3Table.hpp"
#include "support/ModelRecord.hpp"
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

constexpr double EPS = 1.0e-12;

ElementValues geometryOf(const Model& model, std::uint16_t type, base::EntityId eid)
{
    const auto location = model.locateInType(type, eid);
    REQUIRE(location.has_value());
    return ElementGeometry::of(model, *location);
}

void requireVec(const base::Vec3& actual, double x, double y, double z)
{
    REQUIRE(actual.x == Approx(x).margin(EPS));
    REQUIRE(actual.y == Approx(y).margin(EPS));
    REQUIRE(actual.z == Approx(z).margin(EPS));
}

// The record of a node: id, x, y, z (in the basic system).
std::string grid(int id, double x, double y, double z)
{
    return oor::testing::modelRecord("Node", id, "", x, y, z);
}

} // namespace

TEST_CASE("the plot elements with three and four points have geometry", "[analysis][geometry][plot]")
{
    REQUIRE(ElementGeometry::supports(typeIndexOf<PlotTria3Table>()));
    REQUIRE(ElementGeometry::supports(typeIndexOf<PlotQuad4Table>()));
}

TEST_CASE("a PLOTEL3 is a surface: area, normal by the right-hand rule and centre, no thickness",
          "[analysis][geometry][plot]")
{
    const Model model = readModelText(grid(1, 0, 0, 0) + grid(2, 2, 0, 0) + grid(3, 0, 2, 0) +
                                      "PlotTria3;10;1;2;3\nPlotTria3;11;1;3;2\n");
    const std::uint16_t type = typeIndexOf<PlotTria3Table>();
    const ElementValues triangle = geometryOf(model, type, 10);
    REQUIRE(triangle.area == Approx(2.0));
    requireVec(triangle.normal, 0.0, 0.0, 1.0);
    requireVec(triangle.center, 2.0 / 3.0, 2.0 / 3.0, 0.0);
    // Nothing is drawn but the surface: no thickness, no volume, no length.
    REQUIRE(std::isnan(triangle.thickness));
    REQUIRE(std::isnan(triangle.volume));
    REQUIRE(std::isnan(triangle.length));
    // The other way round: the opposite normal.
    requireVec(geometryOf(model, type, 11).normal, 0.0, 0.0, -1.0);
    // A plot element is not an element of the element ID space.
    REQUIRE_FALSE(model.locate(base::IdSpace::Element, 10).has_value());
}

TEST_CASE("a PLOTEL4 is a surface: area, normal by the right-hand rule and centre, no thickness",
          "[analysis][geometry][plot]")
{
    const Model model = readModelText(grid(1, 0, 0, 0) + grid(2, 2, 0, 0) + grid(3, 2, 1, 0) + grid(4, 0, 1, 0) +
                                      "PlotQuad4;20;1;2;3;4\nPlotQuad4;21;4;3;2;1\n");
    const std::uint16_t type = typeIndexOf<PlotQuad4Table>();
    const ElementValues rectangle = geometryOf(model, type, 20);
    REQUIRE(rectangle.area == Approx(2.0));
    requireVec(rectangle.normal, 0.0, 0.0, 1.0);
    requireVec(rectangle.center, 1.0, 0.5, 0.0);
    REQUIRE(std::isnan(rectangle.thickness));
    REQUIRE(std::isnan(rectangle.volume));
    REQUIRE(std::isnan(rectangle.length));
    requireVec(geometryOf(model, type, 21).normal, 0.0, 0.0, -1.0);
}

TEST_CASE("a plot element with a point that no card defines is an error, as for every element",
          "[analysis][geometry][plot]")
{
    const Model model = readModelText(grid(1, 0, 0, 0) + grid(2, 1, 0, 0) + grid(3, 0, 1, 0) + grid(4, 1, 1, 0) +
                                      "PlotTria3;30;1;2;99\nPlotQuad4;31;1;2;4;99\n");
    REQUIRE_THROWS_AS(geometryOf(model, typeIndexOf<PlotTria3Table>(), 30), GeometryError);
    REQUIRE_THROWS_AS(geometryOf(model, typeIndexOf<PlotQuad4Table>(), 31), GeometryError);
}
