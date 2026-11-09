#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/analysis/frames/FrameMath.hpp"
#include "oor/analysis/frames/NodeDisplacementFrames.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "support/ModelRecord.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <numbers>
#include <string>

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

void requireFrame(const base::Mat3& frame, const base::Vec3& x, const base::Vec3& y, const base::Vec3& z,
                  double margin = EPS)
{
    const base::Vec3 axes[3] = {x, y, z};
    for (std::size_t axis = 0; axis < 3; ++axis)
    {
        REQUIRE(frame.column(axis).x == Approx(axes[axis].x).margin(margin));
        REQUIRE(frame.column(axis).y == Approx(axes[axis].y).margin(margin));
        REQUIRE(frame.column(axis).z == Approx(axes[axis].z).margin(margin));
    }
}

std::string grid(int id, double x, double y, double z)
{
    return oor::testing::modelRecord("Node", id, "", x, y, z);
}

constexpr double ROOT_HALF = 0.70710678118654752440;

const char* const SOLIDS = "IsoMat;1;2.1E+5;;0.3\nSolidProp;1;1\n";

} // namespace

TEST_CASE("the element system of a hexahedron follows a rotated and stretched box", "[analysis][frames]")
{
    // A box of 2 x 1 x 3 turned 30 degrees about z.
    const double angle = 30.0 * std::numbers::pi / 180.0;
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    const double box[8][3] = {{0, 0, 0}, {2, 0, 0}, {2, 1, 0}, {0, 1, 0}, {0, 0, 3}, {2, 0, 3}, {2, 1, 3}, {0, 1, 3}};
    std::string deck = SOLIDS;
    for (int node = 0; node < 8; ++node)
        deck += grid(node + 1, c * box[node][0] - s * box[node][1], s * box[node][0] + c * box[node][1], box[node][2]);
    deck += "SolidHexa;10;1;1;2;3;4;5;6;7;8\n";
    const Model model = readModelText(deck);
    const ElementValues values = geometryOf(model, 10);
    requireFrame(values.frame, {c, s, 0}, {-s, c, 0}, {0, 0, 1}, 1.0e-6);
    REQUIRE(values.volume == Approx(6.0));
}

TEST_CASE("the element system of a tetrahedron is the rotation closest to R, S and T of the format reference",
          "[analysis][frames]")
{
    const Model model = readModelText(std::string(SOLIDS) + grid(1, 0, 0, 0) + grid(2, 1, 0, 0) + grid(3, 0, 1, 0) +
                                      grid(4, 0, 0, 1) + "SolidTetra;10;1;1;2;3;4\n");
    // R = (-.5, .5, .5), S = (.5, -.5, .5), T = (.5, .5, -.5): a symmetric matrix with the eigenvalues 1/2, -1, -1, so
    // the rotation closest to it turns by half a turn about (1, 1, 1): 2 n n^T - I.
    const ElementValues values = geometryOf(model, 10);
    const double a = -1.0 / 3.0;
    const double b = 2.0 / 3.0;
    requireFrame(values.frame, {a, b, b}, {b, a, b}, {b, b, a}, 1.0e-9);
}

TEST_CASE("the element system of a pentahedron and of a pyramid are the format reference formulas",
          "[analysis][frames]")
{
    const Model model = readModelText(
        std::string(SOLIDS) + grid(1, 0, 0, 0) + grid(2, 1, 0, 0) + grid(3, 0, 1, 0) + grid(4, 0, 0, 1) +
        grid(5, 1, 0, 1) + grid(6, 0, 1, 1) + grid(11, 0, 0, 0) + grid(12, 1, 0, 0) + grid(13, 1, 1, 0) +
        grid(14, 0, 1, 0) + grid(15, 0.5, 0.5, 1) + "SolidPenta;10;1;1;2;3;4;5;6\nSolidPyramid;11;1;11;12;13;14;15\n");
    requireFrame(geometryOf(model, 10).frame, {ROOT_HALF, ROOT_HALF, 0}, {-ROOT_HALF, ROOT_HALF, 0}, {0, 0, 1});
    const ElementValues pyramid = geometryOf(model, 11);
    requireFrame(pyramid.frame, {0, -1, 0}, {1, 0, 0}, {0, 0, 1});
    REQUIRE(pyramid.volume == Approx(1.0 / 3.0));
}

TEST_CASE("pyramid, wedge and tetrahedron with straight mid-side nodes have the volume of the corner element",
          "[analysis][frames]")
{
    // The unit tetrahedron with its six edge nodes at the middles of the edges.
    const Model model =
        readModelText(std::string(SOLIDS) + grid(1, 0, 0, 0) + grid(2, 1, 0, 0) + grid(3, 0, 1, 0) + grid(4, 0, 0, 1) +
                      grid(5, 0.5, 0, 0) + grid(6, 0.5, 0.5, 0) + grid(7, 0, 0.5, 0) + grid(8, 0, 0, 0.5) +
                      grid(9, 0.5, 0, 0.5) + grid(10, 0, 0.5, 0.5) + "SolidTetra;10;1;1;2;3;4;5;6;7;8;9;10\n");
    REQUIRE(geometryOf(model, 10).volume == Approx(1.0 / 6.0));
    requireFrame(geometryOf(model, 10).frame, {-1.0 / 3.0, 2.0 / 3.0, 2.0 / 3.0}, {2.0 / 3.0, -1.0 / 3.0, 2.0 / 3.0},
                 {2.0 / 3.0, 2.0 / 3.0, -1.0 / 3.0}, 1.0e-9);
}

TEST_CASE("the r-axis of a spherical material system is taken at the element centre", "[analysis][frames]")
{
    // A triangle in the plane z = 1 around the point (0, 3, 1); the spherical system is at the origin. At the element
    // centre the radial direction is (0, 3, 1) / sqrt 10; projected on the plane (normal z) it is +y.
    const Model model =
        readModelText("IsoMat;1;2.1E+5;;0.3\nShellProp;1;1;0.1\nSphericalFrame2;3;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n" +
                      grid(1, -1, 2, 1) + grid(2, 1, 2, 1) + grid(3, 0, 5, 1) + "ShellTria3;10;1;1;2;3;;3\n");
    requireFrame(geometryOf(model, 10).materialFrame, {0, 1, 0}, {-1, 0, 0}, {0, 0, 1});
}

TEST_CASE("the rigid triangular plate, the shear panel and the reduced-rotation shells have their own geometry",
          "[analysis][frames]")
{
    const Model model = readModelText("IsoMat;1;2.1E+5;;0.3\nShellProp;1;1;0.1\nShearPanelProp;2;1;0.3\n" +
                                      grid(1, 0, 0, 0) + grid(2, 2, 0, 0) + grid(3, 2, 2, 0) + grid(4, 0, 2, 0) +
                                      "RigidTriaPlate;10;1;2;3\nShearPanel4;11;2;1;2;3;4\nShellQuad4Drilling;12;1;1;2;"
                                      "3;4\nShellTria3Drilling;13;1;1;2;3\n");
    const ElementValues plate = geometryOf(model, 10);
    REQUIRE(plate.area == Approx(2.0));
    REQUIRE(plate.normal.z == Approx(1.0));
    const ElementValues shear = geometryOf(model, 11);
    REQUIRE(shear.area == Approx(4.0));
    REQUIRE(shear.thickness == Approx(0.3));
    REQUIRE(shear.volume == Approx(1.2));
    // the shear panel: x from G1 to G2 (format reference Figure 9-65), no material axes
    requireFrame(shear.frame, {1, 0, 0}, {0, 1, 0}, {0, 0, 1});
    REQUIRE(std::isnan(shear.materialFrame(0, 0)));
    REQUIRE(geometryOf(model, 12).area == Approx(4.0));
    REQUIRE(geometryOf(model, 13).area == Approx(2.0));
    requireFrame(geometryOf(model, 13).frame, {1, 0, 0}, {0, 1, 0}, {0, 0, 1});
}

TEST_CASE("the displacement rotations of many grid points are the same with and without threads", "[analysis][frames]")
{
    std::string deck =
        "CylindricalFrame2;2;;0.;0.;0.;0.;0.;1.;1.;0.;0.\nSphericalFrame2;3;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n";
    for (int index = 1; index <= 25000; ++index)
    {
        deck += "Node;" + std::to_string(index) + ";2;" + std::to_string(1.0 + index % 7) + ";" +
                std::to_string(index % 360) + ".;" + std::to_string(index % 11) + ".;" +
                std::to_string(index % 3 == 0 ? 3 : 2) + "\n";
    }
    Model model = readModelText(deck);
    Model copy = model.copy();
    std::vector<std::uint32_t> rows;
    for (std::uint32_t row = 0; row < 25000; ++row)
        rows.push_back(row);
    base::ThreadPool pool(3);
    const std::vector<base::Mat3> serial = NodeDisplacementFrames::rotations(model, rows);
    const std::vector<base::Mat3> threaded = NodeDisplacementFrames::rotations(copy, rows, &pool);
    REQUIRE(serial == threaded);
    // The positions of the copy were computed over the pool, those of the original serially: the same numbers.
    const BasicPositionCache& left = BasicPositionCache::of(model);
    const BasicPositionCache& right = BasicPositionCache::of(copy, &pool);
    REQUIRE(left.positions().size() == right.positions().size());
    for (std::size_t row = 0; row < left.positions().size(); row += 997)
        REQUIRE(left.positions()[row] == right.positions()[row]);
    for (const base::Mat3& rotation : threaded)
        REQUIRE(rotation.determinant() == Approx(1.0).margin(1.0e-12));
}
