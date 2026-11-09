// The geometry of two element cards: the RBAX3D (a rigid connection between two grid
// points, as the RBAR is: the middle of the two points and their distance) and the PLTSURF (a plot surface of 3, 4, 6
// or 8 points: a surface with no thickness, whose planar quantities come from the corners). The RBAX3D is in the node
// to element index and has the role of a rigid element; the PLTSURF has an ID space of its own and is, like the PLOTEL,
// not an element of the connectivity.

#include "oor/analysis/adjacency/MeshAdjacency.hpp"
#include "oor/analysis/components/ElementRoles.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/model/cards/elements/plotSurface/PlotSurfaceTable.hpp"
#include "oor/model/cards/rigid_elements/rigidAxisymConnection/RigidAxisymConnectionTable.hpp"
#include "support/ModelTestSupport.hpp"

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

// A node record: id, x, y, z (in the basic system).
std::string grid(int id, double x, double y, double z)
{
    return "Node;" + std::to_string(id) + ";;" + std::to_string(x) + ";" + std::to_string(y) + ";" + std::to_string(z) +
           "\n";
}

void requireVec(const base::Vec3& actual, double x, double y, double z)
{
    REQUIRE(actual.x == Approx(x).margin(EPS));
    REQUIRE(actual.y == Approx(y).margin(EPS));
    REQUIRE(actual.z == Approx(z).margin(EPS));
}

ElementValues elementGeometry(const Model& model, base::EntityId eid)
{
    const auto location = model.locate(base::IdSpace::Element, eid);
    REQUIRE(location.has_value());
    return ElementGeometry::of(model, *location);
}

ElementValues plotGeometry(const Model& model, base::EntityId eid)
{
    const auto location = model.locateInType(typeIndexOf<PlotSurfaceTable>(), eid);
    REQUIRE(location.has_value());
    return ElementGeometry::of(model, *location);
}

} // namespace

TEST_CASE("an RBAX3D has the middle of its two points as centre and their distance as length", "[analysis][geometry]")
{
    REQUIRE(ElementGeometry::supports(typeIndexOf<RigidAxisymConnectionTable>()));
    // The point on the axis of the second element has an ID above 100,000,000, as the format's describer asks.
    const Model model =
        readModelText(grid(10, 0, 0, 1) + grid(20, 2, 0, 1) + grid(100000012, 0, 0, 3) + grid(21, 3, 4, 3) +
                      "RigidAxisymConnection;100;10;20\nRigidAxisymConnection;101;100000012;21\n");
    const ElementValues radial = elementGeometry(model, 100);
    requireVec(radial.center, 1.0, 0.0, 1.0);
    REQUIRE(radial.length == Approx(2.0)); // the radius of the point GAX
    const ElementValues slanted = elementGeometry(model, 101);
    requireVec(slanted.center, 1.5, 2.0, 3.0);
    REQUIRE(slanted.length == Approx(5.0));
    // A rigid connection has no area, volume, normal, thickness or frame.
    for (const ElementValues& values : {radial, slanted})
    {
        REQUIRE(std::isnan(values.area));
        REQUIRE(std::isnan(values.volume));
        REQUIRE(std::isnan(values.thickness));
        REQUIRE(std::isnan(values.normal.x));
        REQUIRE(std::isnan(values.frame.values[0]));
    }
}

TEST_CASE("an RBAX3D is attached to its two points and is a rigid element", "[analysis][geometry]")
{
    const Model model =
        readModelText(grid(10, 0, 0, 1) + grid(20, 2, 0, 1) + grid(30, 5, 0, 1) + "RigidAxisymConnection;100;10;20\n");
    REQUIRE(MeshAdjacency::elementsOfNode(model, 10) == std::vector<base::EntityId>{100});
    REQUIRE(MeshAdjacency::elementsOfNode(model, 20) == std::vector<base::EntityId>{100});
    REQUIRE(MeshAdjacency::elementsOfNode(model, 30).empty());
    REQUIRE(roleOfCard("RigidAxisymConnection") == ElementRole::Rigid);
    REQUIRE(roleOfType(typeIndexOf<RigidAxisymConnectionTable>()) == ElementRole::Rigid);
}

TEST_CASE("an RBAX3D whose point does not exist is reported with the element", "[analysis][geometry]")
{
    const Model model = readModelText(grid(10, 0, 0, 1) + "RigidAxisymConnection;100;10;77\n");
    const auto location = model.locate(base::IdSpace::Element, 100);
    REQUIRE(location.has_value());
    REQUIRE_THROWS_AS(ElementGeometry::of(model, *location), GeometryError);
}

TEST_CASE("a PLTSURF is a surface whatever its number of points: area, normal by the right-hand rule and centre",
          "[analysis][geometry]")
{
    REQUIRE(ElementGeometry::supports(typeIndexOf<PlotSurfaceTable>()));
    // A rectangle of 2 x 1 with the points in the middle of its sides (5 to 8) and a point in the middle of the line
    // from the third corner to the first (9).
    const Model model =
        readModelText(grid(1, 0, 0, 0) + grid(2, 2, 0, 0) + grid(3, 2, 1, 0) + grid(4, 0, 1, 0) + grid(5, 1, 0, 0) +
                      grid(6, 2, 0.5, 0) + grid(7, 1, 1, 0) + grid(8, 0, 0.5, 0) + grid(9, 1, 0.5, 0) +
                      "PlotSurface;10;1;2;3\n"
                      "PlotSurface;11;1;2;3;4\n"
                      "PlotSurface;12;4;3;2;1\n"
                      "PlotSurface;13;1;2;3;5;6;9\n"
                      "PlotSurface;14;1;2;3;4;5;6;7;8\n");

    const ElementValues triangle = plotGeometry(model, 10);
    REQUIRE(triangle.area == Approx(1.0));
    requireVec(triangle.normal, 0.0, 0.0, 1.0);
    requireVec(triangle.center, 4.0 / 3.0, 1.0 / 3.0, 0.0);
    const ElementValues rectangle = plotGeometry(model, 11);
    REQUIRE(rectangle.area == Approx(2.0));
    requireVec(rectangle.normal, 0.0, 0.0, 1.0);
    requireVec(rectangle.center, 1.0, 0.5, 0.0);
    requireVec(plotGeometry(model, 12).normal, 0.0, 0.0, -1.0); // the other way round
    // The six and eight point forms: the corners are the first three or four points, the others do not count.
    const ElementValues sixPoints = plotGeometry(model, 13);
    REQUIRE(sixPoints.area == Approx(1.0));
    requireVec(sixPoints.normal, 0.0, 0.0, 1.0);
    requireVec(sixPoints.center, 4.0 / 3.0, 1.0 / 3.0, 0.0);
    const ElementValues eightPoints = plotGeometry(model, 14);
    REQUIRE(eightPoints.area == Approx(2.0));
    requireVec(eightPoints.normal, 0.0, 0.0, 1.0);
    requireVec(eightPoints.center, 1.0, 0.5, 0.0);
    // Nothing is drawn but the surface.
    for (const ElementValues& values : {triangle, rectangle, sixPoints, eightPoints})
    {
        REQUIRE(std::isnan(values.thickness));
        REQUIRE(std::isnan(values.volume));
        REQUIRE(std::isnan(values.length));
    }
}

TEST_CASE("a PLTSURF is not an element of the connectivity and does not take the ID of a structural element",
          "[analysis][geometry]")
{
    const Model model =
        readModelText(grid(1, 0, 0, 0) + grid(2, 2, 0, 0) + grid(3, 2, 1, 0) +
                      "IsoMat;1;2.1E+11;;0.3\nRodProp;1;1;1.\nRod2;10;1;1;2\nPlotSurface;10;1;2;3\n");
    // The plot surface is in no node to element index: only the rod is attached to the grid point 1.
    REQUIRE(MeshAdjacency::elementsOfNode(model, 1) == std::vector<base::EntityId>{10});
    REQUIRE(MeshAdjacency::elementsOfNode(model, 3).empty());
    REQUIRE(elementGeometry(model, 10).length == Approx(2.0)); // the CROD 10 of the element space
    REQUIRE(plotGeometry(model, 10).area == Approx(1.0));      // and the PLTSURF 10 of its own
}

TEST_CASE("a PLTSURF with a point that no card defines is an error, as for every element", "[analysis][geometry]")
{
    const Model model =
        readModelText(grid(1, 0, 0, 0) + grid(2, 1, 0, 0) + grid(3, 0, 1, 0) + "PlotSurface;30;1;2;99\n");
    REQUIRE_THROWS_AS(plotGeometry(model, 30), GeometryError);
}
