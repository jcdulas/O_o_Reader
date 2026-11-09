#include "oor/analysis/quality/GeometryCheckThresholds.hpp"
#include "oor/analysis/quality/MeshQuality.hpp"
#include "oor/model/cards/elements/solidHexa/SolidHexaTable.hpp"
#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "support/ModelRecord.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <array>
#include <cmath>
#include <numbers>
#include <string>
#include <vector>

using oor::testing::modelRecord;

using namespace oor;
using namespace oor::analysis;
using namespace oor::model;
using Catch::Approx;
using oor::testing::readModelText;

namespace
{

constexpr double DEGREES = 180.0 / std::numbers::pi;

using Point = std::array<double, 3>;

// The records of the nodes at the points, numbered from `first`.
std::string gridsOf(const std::vector<Point>& points, int first = 1)
{
    std::string text;
    for (std::size_t index = 0; index < points.size(); ++index)
        text += modelRecord("Node", first + static_cast<int>(index), "", points[index][0], points[index][1],
                            points[index][2]);
    return text;
}

const char* const PROPERTIES = "IsoMat;1;2.1E+5;;0.3\nShellProp;1;1;0.1\nSolidProp;2;1\n";

QualityValues qualityOf(const Model& model, base::EntityId eid)
{
    const auto location = model.locate(base::IdSpace::Element, eid);
    REQUIRE(location.has_value());
    return MeshQuality::of(model, *location);
}

// One quadrilateral (element 10, nodes 1 to 4) or triangle (nodes 1 to 3) on the points given.
Model shellOn(const std::vector<Point>& points, const std::string& card)
{
    return readModelText(std::string(PROPERTIES) + gridsOf(points) + card);
}

Model quad(const std::vector<Point>& points)
{
    return shellOn(points, "ShellQuad4;10;1;1;2;3;4\n");
}

Model triangle(const std::vector<Point>& points)
{
    return shellOn(points, "ShellTria3;10;1;1;2;3\n");
}

const std::vector<Point> UNIT_CUBE = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                                      {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};

Model hexa(const std::vector<Point>& points)
{
    return readModelText(std::string(PROPERTIES) + gridsOf(points) + "SolidHexa;10;2;1;2;3;4;5;6;7;8\n");
}

constexpr double ROOT3 = 1.7320508075688772935;

} // namespace

TEST_CASE("a unit square has every shell metric at its ideal value", "[analysis][quality]")
{
    const QualityValues q = qualityOf(quad({{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}}), 10);
    REQUIRE(q.aspectRatio == Approx(1.0));
    REQUIRE(q.skew == Approx(0.0).margin(1e-9));
    REQUIRE(q.warp == Approx(0.0).margin(1e-12));
    REQUIRE(q.taper == Approx(0.0).margin(1e-12));
    REQUIRE(q.minAngle == Approx(90.0));
    REQUIRE(q.maxAngle == Approx(90.0));
    REQUIRE(q.jacobianRatio == Approx(1.0));
    REQUIRE(q.jacobianMin == Approx(0.25)); // a quarter of the area: the reference square has the area 4
    REQUIRE(q.minEdge == Approx(1.0));
    REQUIRE(q.maxEdge == Approx(1.0));
    // What does not apply to a shell without mid-side nodes is not a number.
    REQUIRE(std::isnan(q.collapse));
    REQUIRE(std::isnan(q.edgePointLengthRatio));
    REQUIRE(std::isnan(q.edgePointAngle));
    REQUIRE(std::isnan(q.faceWarp));
}

TEST_CASE("a 4 to 1 rectangle has the aspect ratio 4 and is otherwise perfect", "[analysis][quality]")
{
    const QualityValues q = qualityOf(quad({{0, 0, 0}, {4, 0, 0}, {4, 1, 0}, {0, 1, 0}}), 10);
    REQUIRE(q.aspectRatio == Approx(4.0));
    REQUIRE(q.minEdge == Approx(1.0));
    REQUIRE(q.maxEdge == Approx(4.0));
    REQUIRE(q.skew == Approx(0.0).margin(1e-9));
    REQUIRE(q.taper == Approx(0.0).margin(1e-12));
    REQUIRE(q.minAngle == Approx(90.0));
    REQUIRE(q.jacobianRatio == Approx(1.0));
    REQUIRE(q.jacobianMin == Approx(1.0)); // the area 4 over the reference area 4
}

TEST_CASE("a rhombus has the skew the angle between the lines through the middles of opposite sides leaves",
          "[analysis][quality]")
{
    // Sides of length 1 at 60 degrees: the middles of opposite sides are joined by lines that cross at 60 degrees, so
    // the skew is 90 - 60 = 30 degrees; the interior angles are 60 and 120.
    const QualityValues q = qualityOf(quad({{0, 0, 0}, {1, 0, 0}, {1.5, ROOT3 / 2, 0}, {0.5, ROOT3 / 2, 0}}), 10);
    REQUIRE(q.skew == Approx(30.0));
    REQUIRE(q.minAngle == Approx(60.0));
    REQUIRE(q.maxAngle == Approx(120.0));
    REQUIRE(q.aspectRatio == Approx(1.0));
    REQUIRE(q.taper == Approx(0.0).margin(1e-12)); // a parallelogram
    REQUIRE(q.warp == Approx(0.0).margin(1e-12));
    REQUIRE(q.jacobianRatio == Approx(1.0));
}

TEST_CASE("a trapezoid has the taper |a - b| / (a + b) and the Jacobian of its corners", "[analysis][quality]")
{
    // Parallel sides 3 and 1, height 1: the triangles at the corners have the areas 3/2 and 1/2 against half the area
    // (1), so the taper is 1/2. The determinants at the corners are twice the corner triangles over four: 3/4 and 1/4.
    const QualityValues q = qualityOf(quad({{0, 0, 0}, {3, 0, 0}, {2, 1, 0}, {1, 1, 0}}), 10);
    REQUIRE(q.taper == Approx(0.5));
    REQUIRE(q.minAngle == Approx(45.0));
    REQUIRE(q.maxAngle == Approx(135.0));
    REQUIRE(q.aspectRatio == Approx(3.0));
    REQUIRE(q.jacobianMin == Approx(0.25));
    REQUIRE(q.jacobianRatio == Approx(1.0 / 3.0));
    REQUIRE(q.skew == Approx(0.0).margin(1e-9)); // symmetric: the lines through the middles are perpendicular
}

TEST_CASE("a warped quadrilateral has the distance of its corners from the mean plane over the mean diagonal",
          "[analysis][quality]")
{
    // The corners alternate between z = h and z = -h: the mean plane is z = 0, every corner is at h from it, both
    // diagonals are sqrt(2) long.
    const double h = 0.1;
    const QualityValues q = qualityOf(quad({{0, 0, h}, {1, 0, -h}, {1, 1, h}, {0, 1, -h}}), 10);
    REQUIRE(q.warp == Approx(h / std::sqrt(2.0)));
    // A flat element is not warped, and this one is above the default tolerance of 0.05.
    REQUIRE(GeometryCheckThresholds().failedChecks(ElementShape::Quadrilateral, q) == bit(QualityCheck::Warp));
}

TEST_CASE("a quadrilateral with a reflex corner has an interior angle over 180 and a negative Jacobian",
          "[analysis][quality]")
{
    const QualityValues q = qualityOf(quad({{0, 0, 0}, {2, 0, 0}, {0.5, 0.5, 0}, {0, 2, 0}}), 10);
    REQUIRE(q.maxAngle == Approx(360.0 - std::acos(-0.6) * DEGREES));
    REQUIRE(q.jacobianMin == Approx(-0.5));
    REQUIRE(q.jacobianRatio == Approx(-0.5));
    const std::uint16_t failed = GeometryCheckThresholds().failedChecks(ElementShape::Quadrilateral, q);
    REQUIRE((failed & bit(QualityCheck::MaxAngle)) != 0);
}

TEST_CASE("an equilateral triangle is perfect, a right isosceles one has the skew of its medians",
          "[analysis][quality]")
{
    const QualityValues equilateral = qualityOf(triangle({{0, 0, 0}, {1, 0, 0}, {0.5, ROOT3 / 2, 0}}), 10);
    REQUIRE(equilateral.aspectRatio == Approx(1.0));
    REQUIRE(equilateral.minAngle == Approx(60.0));
    REQUIRE(equilateral.maxAngle == Approx(60.0));
    REQUIRE(equilateral.skew == Approx(0.0).margin(1e-9));
    REQUIRE(equilateral.jacobianRatio == Approx(1.0));
    REQUIRE(equilateral.jacobianMin == Approx(ROOT3 / 2.0)); // twice the area
    REQUIRE(std::isnan(equilateral.warp));
    REQUIRE(std::isnan(equilateral.taper));

    // The median to a leg makes an angle atan(2) with it: the skew is 90 - atan(2).
    const QualityValues right = qualityOf(triangle({{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}), 10);
    REQUIRE(right.skew == Approx(std::atan(0.5) * DEGREES));
    REQUIRE(right.minAngle == Approx(45.0));
    REQUIRE(right.maxAngle == Approx(90.0));
    REQUIRE(right.aspectRatio == Approx(std::sqrt(2.0)));
    REQUIRE(right.jacobianMin == Approx(1.0));
}

TEST_CASE("the GEOMCHECK tolerances flag the elements that fail, with the defaults of the format reference",
          "[analysis][quality]")
{
    const GeometryCheckThresholds defaults;
    REQUIRE(defaults.q4Skew == 30.0);
    REQUIRE(defaults.q4Taper == 0.5);
    REQUIRE(defaults.q4Warp == 0.05);
    REQUIRE(defaults.t3Skew == 10.0);
    REQUIRE(defaults.t3Iamax == 160.0);
    REQUIRE(defaults.tetAr == 100.0);
    REQUIRE(defaults.hexWarp == 0.707);
    REQUIRE(defaults.penEpia == 150.0);

    // A triangle with an angle of 5 degrees fails TriangleMinAngle (the format reference: the smallest vertex angle),
    // one with 170 fails TriangleMaxAngle.
    const QualityValues thin = qualityOf(triangle({{0, 0, 0}, {1, 0, 0}, {0.9, 0.0787, 0}}), 10);
    REQUIRE(thin.minAngle < 10.0);
    REQUIRE((defaults.failedChecks(ElementShape::Triangle, thin) & bit(QualityCheck::MinAngle)) != 0);
    const QualityValues flat = qualityOf(triangle({{0, 0, 0}, {1, 0, 0}, {0.5, 0.05, 0}}), 10);
    REQUIRE(flat.maxAngle > 160.0);
    REQUIRE((defaults.failedChecks(ElementShape::Triangle, flat) & bit(QualityCheck::MaxAngle)) != 0);

    // A good element fails nothing; a NaN metric fails nothing.
    const QualityValues good = qualityOf(quad({{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}}), 10);
    REQUIRE(defaults.failedChecks(ElementShape::Quadrilateral, good) == 0);
    REQUIRE(defaults.failedChecks(ElementShape::Quadrilateral, QualityValues{}) == 0);
    REQUIRE(defaults.failedChecks(ElementShape::Line, thin) == 0);

    // The trapezoid of taper 0.5 sits on the tolerance (not above it); the keyword changes it.
    const QualityValues trapezoid = qualityOf(quad({{0, 0, 0}, {3, 0, 0}, {2, 1, 0}, {1, 1, 0}}), 10);
    REQUIRE(defaults.failedChecks(ElementShape::Quadrilateral, trapezoid) == 0);
    GeometryCheckThresholds strict;
    REQUIRE(strict.set("QuadTaperLimit", 0.3));
    REQUIRE(strict.q4Taper == 0.3);
    REQUIRE(strict.failedChecks(ElementShape::Quadrilateral, trapezoid) == bit(QualityCheck::Taper));
    REQUIRE_FALSE(strict.set("NOT_A_KEYWORD", 1.0));
    REQUIRE_FALSE(strict.set("MSGLIMIT", 5.0));
}

TEST_CASE("the GEOMCHECK statements of the deck change the tolerances, the last one that names a keyword counts",
          "[analysis][quality]")
{
    const Model none = readModelText("[executive]\nSolutionSequence;101\n[model]\nNode;1\n");
    REQUIRE(GeometryCheckThresholds::fromModel(none).q4Skew == 30.0);

    const Model model = readModelText(
        "[executive]\n"
        "SolutionSequence;101\n"
        "GeometryCheck;QuadSkewLimit=15.0;MessageLimit=50\n"
        "GeometryCheck;TetrahedronAspectRatioLimit=50.0;QuadSkewLimit=20.0\n"
        "GeometryCheck;QuadTaperLimit;MessageKind=FATAL\n"
        "GeometryCheck;HexahedronJacobianLimit=0.001;NoMessages\n"
        "[model]\n"
        "Node;1\n");
    const GeometryCheckThresholds thresholds = GeometryCheckThresholds::fromModel(model);
    REQUIRE(thresholds.q4Skew == 20.0);
    REQUIRE(thresholds.tetAr == 50.0);
    REQUIRE(thresholds.q4Taper == 0.5); // named without a value: the default stays
    REQUIRE(thresholds.hexDetj == 0.001);
    REQUIRE(thresholds.q4Warp == 0.05);
}

TEST_CASE("a unit cube is perfect and a box of 4 to 1 to 1 has the aspect ratio 4", "[analysis][quality]")
{
    const QualityValues q = qualityOf(hexa(UNIT_CUBE), 10);
    REQUIRE(q.aspectRatio == Approx(1.0));
    REQUIRE(q.minAngle == Approx(90.0));
    REQUIRE(q.maxAngle == Approx(90.0));
    REQUIRE(q.faceWarp == Approx(1.0));
    REQUIRE(q.jacobianRatio == Approx(1.0));
    REQUIRE(q.jacobianMin == Approx(0.125)); // an eighth of the volume: the reference cube has the volume 8
    REQUIRE(q.minEdge == Approx(1.0));
    REQUIRE(std::isnan(q.skew));
    REQUIRE(std::isnan(q.collapse));

    std::vector<Point> box = UNIT_CUBE;
    for (Point& point : box)
        point[0] *= 4.0;
    const QualityValues elongated = qualityOf(hexa(box), 10);
    REQUIRE(elongated.aspectRatio == Approx(4.0));
    REQUIRE(elongated.maxEdge == Approx(4.0));
    REQUIRE(elongated.jacobianMin == Approx(0.5));
}

TEST_CASE("a sheared hexahedron keeps a uniform Jacobian and has the angles of its sheared faces",
          "[analysis][quality]")
{
    // The top face moved by 1 along x: the faces in the x-z planes are parallelograms of 45 and 135 degrees, the
    // vertical edges are sqrt(2) long, and the mapping is still affine, so the determinant is the same at every corner.
    std::vector<Point> points = UNIT_CUBE;
    for (std::size_t node = 4; node < 8; ++node)
        points[node][0] += 1.0;
    const QualityValues q = qualityOf(hexa(points), 10);
    REQUIRE(q.minAngle == Approx(45.0));
    REQUIRE(q.maxAngle == Approx(135.0));
    REQUIRE(q.aspectRatio == Approx(std::sqrt(2.0)));
    REQUIRE(q.jacobianRatio == Approx(1.0));
    REQUIRE(q.jacobianMin == Approx(0.125));
    REQUIRE(q.faceWarp == Approx(1.0));
}

TEST_CASE("a truncated pyramid has the Jacobian of its corners in proportion to the sides of their faces",
          "[analysis][quality]")
{
    // The top face is a square of half the side: the determinant at a top corner is (1/2)(1/2) of the one at a bottom
    // corner times the same height: 1/32 against 1/8.
    std::vector<Point> points = UNIT_CUBE;
    for (std::size_t node = 4; node < 8; ++node)
    {
        points[node][0] = 0.25 + 0.5 * points[node][0];
        points[node][1] = 0.25 + 0.5 * points[node][1];
    }
    const QualityValues q = qualityOf(hexa(points), 10);
    REQUIRE(q.jacobianMin == Approx(1.0 / 32.0));
    REQUIRE(q.jacobianRatio == Approx(0.25));
    REQUIRE(q.faceWarp == Approx(1.0)); // the side faces are flat trapezoids
}

TEST_CASE("a hexahedron connected the other way round has a negative Jacobian and fails the test",
          "[analysis][quality]")
{
    // The top and bottom faces exchanged: the element is turned inside out.
    std::vector<Point> points = {UNIT_CUBE[4], UNIT_CUBE[5], UNIT_CUBE[6], UNIT_CUBE[7],
                                 UNIT_CUBE[0], UNIT_CUBE[1], UNIT_CUBE[2], UNIT_CUBE[3]};
    const QualityValues q = qualityOf(hexa(points), 10);
    REQUIRE(q.jacobianMin == Approx(-0.125));
    REQUIRE(q.jacobianRatio == Approx(-1.0));
    REQUIRE((GeometryCheckThresholds().failedChecks(ElementShape::Hexahedron, q) & bit(QualityCheck::Jacobian)) != 0);
}

TEST_CASE("a warped face lowers the face warp coefficient below 1", "[analysis][quality]")
{
    // One corner of the top face lifted by 0.5: the normals at its diagonal corners tilt apart.
    std::vector<Point> points = UNIT_CUBE;
    points[6][2] += 0.5;
    const QualityValues q = qualityOf(hexa(points), 10);
    // The normals at the corners of the top face are (0, 0, 1) at G5, (-1/2, -1/2, 1) at G7, (0, -1/2, 1) at G6 and
    // (-1/2, 0, 1) at G8; the smallest cosine between diagonal corners is the one of G6 and G8: 1 / 1.25 = 0.8. The
    // side faces stay flat.
    REQUIRE(q.faceWarp == Approx(0.8));
}

TEST_CASE("a regular tetrahedron has the collapse 1 and a flat one has the collapse 0", "[analysis][quality]")
{
    // The tetrahedron of edge 2 sqrt(2) on four corners of a cube of side 2, connected for a positive volume.
    const std::vector<Point> regular = {{1, 1, 1}, {-1, 1, -1}, {1, -1, -1}, {-1, -1, 1}};
    const Model model = readModelText(std::string(PROPERTIES) + gridsOf(regular) + "SolidTetra;10;2;1;2;3;4\n");
    const QualityValues q = qualityOf(model, 10);
    REQUIRE(q.aspectRatio == Approx(1.0));
    REQUIRE(q.minAngle == Approx(60.0));
    REQUIRE(q.maxAngle == Approx(60.0));
    REQUIRE(q.collapse == Approx(1.0));
    REQUIRE(q.jacobianMin == Approx(16.0)); // six times the volume 8/3
    REQUIRE(q.jacobianRatio == Approx(1.0));
    REQUIRE(q.minEdge == Approx(2.0 * std::sqrt(2.0)));
    REQUIRE(std::isnan(q.faceWarp));

    const std::vector<Point> flat = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}};
    const QualityValues degenerate =
        qualityOf(readModelText(std::string(PROPERTIES) + gridsOf(flat) + "SolidTetra;10;2;1;2;3;4\n"), 10);
    REQUIRE(degenerate.collapse == Approx(0.0).margin(1e-12));
    REQUIRE(degenerate.jacobianMin == Approx(0.0).margin(1e-12));
    const std::uint16_t degenerateFailed =
        GeometryCheckThresholds().failedChecks(ElementShape::Tetrahedron, degenerate);
    REQUIRE((degenerateFailed & bit(QualityCheck::Jacobian)) != 0);

    // A sliver: one edge 200 times the shortest.
    const std::vector<Point> sliver = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 200}};
    const QualityValues needle =
        qualityOf(readModelText(std::string(PROPERTIES) + gridsOf(sliver) + "SolidTetra;10;2;1;2;3;4\n"), 10);
    REQUIRE(needle.aspectRatio == Approx(200.0).epsilon(1e-3));
    const std::uint16_t needleFailed = GeometryCheckThresholds().failedChecks(ElementShape::Tetrahedron, needle);
    REQUIRE((needleFailed & bit(QualityCheck::AspectRatio)) != 0);
}

TEST_CASE("a prism and a pyramid have the angles of their faces and a uniform Jacobian", "[analysis][quality]")
{
    // The prism on the right triangle of legs 1, height 1: faces of 45, 45, 90 degrees and squares.
    const std::vector<Point> prism = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {0, 1, 1}};
    const QualityValues p =
        qualityOf(readModelText(std::string(PROPERTIES) + gridsOf(prism) + "SolidPenta;10;2;1;2;3;4;5;6\n"), 10);
    REQUIRE(p.minAngle == Approx(45.0));
    REQUIRE(p.maxAngle == Approx(90.0));
    REQUIRE(p.aspectRatio == Approx(std::sqrt(2.0)));
    REQUIRE(p.faceWarp == Approx(1.0));
    REQUIRE(p.jacobianRatio == Approx(1.0));
    REQUIRE(p.jacobianMin == Approx(0.5)); // the volume over the volume of the reference wedge (1)

    // The pyramid on the unit square with its apex above the centre, connected for a positive Jacobian.
    const std::vector<Point> pyramid = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0.5, 0.5, 1}};
    const QualityValues y =
        qualityOf(readModelText(std::string(PROPERTIES) + gridsOf(pyramid) + "SolidPyramid;10;2;1;2;3;4;5\n"), 10);
    REQUIRE(y.jacobianRatio == Approx(1.0));
    REQUIRE(y.jacobianMin == Approx(0.125));
    REQUIRE(y.faceWarp == Approx(1.0));
    REQUIRE(y.maxAngle == Approx(90.0));
    REQUIRE(y.minEdge == Approx(1.0));
    // The side edges are sqrt(1.5) long; the apex angles of the triangular faces sum to 180: the smallest angle is at a
    // base corner of a side face.
    REQUIRE(y.maxEdge == Approx(std::sqrt(1.5)));
    REQUIRE(y.minAngle > 40.0);
    REQUIRE(y.minAngle < 70.0);

    // Turned inside out (the apex below the base): negative.
    const std::vector<Point> inverted = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0.5, 0.5, -1}};
    const QualityValues turned =
        qualityOf(readModelText(std::string(PROPERTIES) + gridsOf(inverted) + "SolidPyramid;10;2;1;2;3;4;5\n"), 10);
    REQUIRE(turned.jacobianMin < 0.0);
}

TEST_CASE("mid-side nodes on the curve of a mapping give the Jacobian of the mapping", "[analysis][quality]")
{
    // The elements are mapped by x -> x (1 + b x): the determinant is 1 + 2 b x times the one of the straight element,
    // so at the corners and the centre it ranges from 1 (x = 0) to 1 + 2 b (x = 1), and the Jacobian ratio is 1 / (1 +
    // 2 b) when x = 1 is a corner, whatever the shape. The quadratic shape functions reproduce the mapping exactly.
    const double b = 0.3;
    const double expected = 1.0 / (1.0 + 2.0 * b);
    const auto mapped = [&](const std::vector<Point>& reference)
    {
        std::vector<Point> points = reference;
        for (Point& point : points)
            point[0] = point[0] * (1.0 + b * point[0]);
        return points;
    };
    SECTION("quadrilateral with eight nodes")
    {
        const std::vector<Point> reference = {{0, 0, 0},   {1, 0, 0},   {1, 1, 0},   {0, 1, 0},
                                              {0.5, 0, 0}, {1, 0.5, 0}, {0.5, 1, 0}, {0, 0.5, 0}};
        const Model model =
            readModelText(std::string(PROPERTIES) + gridsOf(mapped(reference)) + "ShellQuad8;10;1;1;2;3;4;5;6;7;8\n");
        const QualityValues q = qualityOf(model, 10);
        REQUIRE(q.jacobianRatio == Approx(expected));
        REQUIRE(q.jacobianMin == Approx(0.25)); // the smallest is the one at x = 0: the straight value
        // The mid-side nodes of the edges along x sit on the curve, off the middle of their edges: at x = 0.5 (1 + 0.5
        // b) between the corners 0 and 1 + b, that is r = 0.5 (1 + 0.5 b) / (1 + b), and |2 r - 1| = 0.5 b / (1 + b).
        REQUIRE(q.edgePointLengthRatio == Approx(0.5 * b / (1.0 + b)));
    }
    SECTION("triangle with six nodes")
    {
        const std::vector<Point> reference = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0.5, 0, 0}, {0.5, 0.5, 0}, {0, 0.5, 0}};
        const Model model =
            readModelText(std::string(PROPERTIES) + gridsOf(mapped(reference)) + "ShellTria6;10;1;1;2;3;4;5;6\n");
        const QualityValues q = qualityOf(model, 10);
        REQUIRE(q.jacobianRatio == Approx(expected));
        REQUIRE(q.jacobianMin == Approx(1.0));
    }
    SECTION("tetrahedron with ten nodes")
    {
        const std::vector<Point> reference = {{0, 0, 0},     {1, 0, 0},   {0, 1, 0},   {0, 0, 1},     {0.5, 0, 0},
                                              {0.5, 0.5, 0}, {0, 0.5, 0}, {0, 0, 0.5}, {0.5, 0, 0.5}, {0, 0.5, 0.5}};
        const Model model = readModelText(std::string(PROPERTIES) + gridsOf(mapped(reference)) +
                                          "SolidTetra;10;2;1;2;3;4;5;6;7;8;9;10\n");
        const QualityValues q = qualityOf(model, 10);
        REQUIRE(q.jacobianRatio == Approx(expected));
        REQUIRE(q.jacobianMin == Approx(1.0));
    }
    SECTION("hexahedron with twenty nodes")
    {
        const std::vector<Point> reference = {{0, 0, 0},   {1, 0, 0},   {1, 1, 0},   {0, 1, 0},   {0, 0, 1},
                                              {1, 0, 1},   {1, 1, 1},   {0, 1, 1},   {0.5, 0, 0}, {1, 0.5, 0},
                                              {0.5, 1, 0}, {0, 0.5, 0}, {0, 0, 0.5}, {1, 0, 0.5}, {1, 1, 0.5},
                                              {0, 1, 0.5}, {0.5, 0, 1}, {1, 0.5, 1}, {0.5, 1, 1}, {0, 0.5, 1}};
        const Model model = readModelText(std::string(PROPERTIES) + gridsOf(mapped(reference)) +
                                          "SolidHexa;10;2;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16;17;18;19;20\n");
        const QualityValues q = qualityOf(model, 10);
        REQUIRE(q.jacobianRatio == Approx(expected));
        REQUIRE(q.jacobianMin == Approx(0.125));
    }
    SECTION("pentahedron with fifteen nodes")
    {
        const std::vector<Point> reference = {{0, 0, 0},   {1, 0, 0},   {0, 1, 0},     {0, 0, 1},     {1, 0, 1},
                                              {0, 1, 1},   {0.5, 0, 0}, {0.5, 0.5, 0}, {0, 0.5, 0},   {0, 0, 0.5},
                                              {1, 0, 0.5}, {0, 1, 0.5}, {0.5, 0, 1},   {0.5, 0.5, 1}, {0, 0.5, 1}};
        const Model model = readModelText(std::string(PROPERTIES) + gridsOf(mapped(reference)) +
                                          "SolidPenta;10;2;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15\n");
        const QualityValues q = qualityOf(model, 10);
        REQUIRE(q.jacobianRatio == Approx(expected));
        REQUIRE(q.jacobianMin == Approx(0.5));
    }
}

TEST_CASE("mid-side nodes at the middle of their edges change nothing, others move the edge point metrics",
          "[analysis][quality]")
{
    // A tetrahedron with its edge nodes at the middles: the Jacobian is the one of the corners, the edge points are
    // ideal.
    const std::vector<Point> straight = {{0, 0, 0},     {1, 0, 0},   {0, 1, 0},   {0, 0, 1},     {0.5, 0, 0},
                                         {0.5, 0.5, 0}, {0, 0.5, 0}, {0, 0, 0.5}, {0.5, 0, 0.5}, {0, 0.5, 0.5}};
    const Model model =
        readModelText(std::string(PROPERTIES) + gridsOf(straight) + "SolidTetra;10;2;1;2;3;4;5;6;7;8;9;10\n");
    const QualityValues q = qualityOf(model, 10);
    REQUIRE(q.jacobianRatio == Approx(1.0));
    REQUIRE(q.jacobianMin == Approx(1.0));
    REQUIRE(q.edgePointLengthRatio == Approx(0.0).margin(1e-12));
    REQUIRE(q.edgePointAngle == Approx(180.0));

    // The node of the edge G1-G2 at a tenth of the way, a little off the line: the length ratio is |2 r - 1| = 0.8 and
    // the angle at the node is under 150 degrees, so both edge point tests fail.
    std::vector<Point> moved = straight;
    moved[4] = {0.1, 0.1, 0};
    const QualityValues bent = qualityOf(
        readModelText(std::string(PROPERTIES) + gridsOf(moved) + "SolidTetra;10;2;1;2;3;4;5;6;7;8;9;10\n"), 10);
    REQUIRE(bent.edgePointLengthRatio == Approx(0.8));
    REQUIRE(bent.edgePointAngle < 150.0);
    const std::uint16_t failed = GeometryCheckThresholds().failedChecks(ElementShape::Tetrahedron, bent);
    REQUIRE((failed & bit(QualityCheck::EdgePointLength)) != 0);
    REQUIRE((failed & bit(QualityCheck::EdgePointAngle)) != 0);
}

TEST_CASE("blank mid-side nodes are the straight element, and a CTRIAX6 alternates corners and mid-side nodes",
          "[analysis][quality]")
{
    // A CQUAD8 with blank mid-side nodes is the quadrilateral on its corners.
    const std::vector<Point> square = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
    const QualityValues q =
        qualityOf(readModelText(std::string(PROPERTIES) + gridsOf(square) + "ShellQuad8;10;1;1;2;3;4\n"), 10);
    REQUIRE(q.jacobianRatio == Approx(1.0));
    REQUIRE(q.jacobianMin == Approx(0.25));
    REQUIRE(std::isnan(q.edgePointAngle));

    // CTRIAX6: G1, G3, G5 are the corners (x-z plane), G2, G4, G6 the middles of the sides between them.
    const std::vector<Point> axisymmetric = {{1, 0, 0}, {1.5, 0, 0}, {2, 0, 0}, {1.5, 0, 0.5}, {1, 0, 1}, {1, 0, 0.5}};
    const Model model =
        readModelText(std::string(PROPERTIES) + gridsOf(axisymmetric) + "AxisymTria6;10;1;1;2;3;4;5;6\n");
    const QualityValues ring = qualityOf(model, 10);
    REQUIRE(ring.minAngle == Approx(45.0));
    REQUIRE(ring.maxAngle == Approx(90.0));
    REQUIRE(ring.aspectRatio == Approx(std::sqrt(2.0)));
    REQUIRE(ring.edgePointLengthRatio == Approx(0.0).margin(1e-12));
}

TEST_CASE("a card type that is not a shell or a solid has no quality", "[analysis][quality]")
{
    const Model model = readModelText(std::string(PROPERTIES) + gridsOf({{0, 0, 0}, {1, 0, 0}}) + "Rod2;10;1;1;2\n");
    const QualityValues q = qualityOf(model, 10);
    REQUIRE(std::isnan(q.aspectRatio));
    REQUIRE(std::isnan(q.minEdge));
    REQUIRE_FALSE(MeshQuality::supports(model.locate(base::IdSpace::Element, 10)->table));
    REQUIRE(ShellQuality::supports(typeIndexOf<ShellQuad4Table>()));
    REQUIRE(SolidQuality::supports(typeIndexOf<SolidHexaTable>()));
    REQUIRE_FALSE(SolidQuality::supports(typeIndexOf<ShellQuad4Table>()));
}

TEST_CASE("the quality of a node in a coordinate system is the quality of its position in the basic system",
          "[analysis][quality]")
{
    const Model model = readModelText(std::string(PROPERTIES) +
                                      "CylindricalFrame2;1;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
                                      "Node;1;1;1.;0.;0.\n"
                                      "Node;2;1;1.;90.;0.\n"
                                      "Node;3;1;2.;90.;0.\n"
                                      "Node;4;1;2.;0.;0.\n"
                                      "ShellQuad4;10;1;1;2;3;4\n");
    // The corners of the element are (1, 0), (0, 1), (0, 2), (2, 0) in the basic system: sides sqrt(2), 1, 2
    // sqrt(2), 1.
    const QualityValues q = qualityOf(model, 10);
    REQUIRE(q.minEdge == Approx(1.0));
    REQUIRE(q.maxEdge == Approx(2.0 * std::sqrt(2.0)));
    REQUIRE(q.aspectRatio == Approx(2.0 * std::sqrt(2.0)));
}

TEST_CASE("many elements give what one element gives, with and without threads", "[analysis][quality]")
{
    std::string deck = std::string(PROPERTIES);
    const int side = 100;
    for (int j = 0; j <= side; ++j)
    {
        for (int i = 0; i <= side; ++i)
            deck += modelRecord("Node", 1 + i + j * (side + 1), "", i * 0.5 + 0.1 * (j % 3), j * 0.4 + 0.01 * i * j,
                                0.02 * i + 0.03 * (j % 2));
    }
    int eid = 1;
    for (int j = 0; j < side; ++j)
    {
        for (int i = 0; i < side; ++i)
        {
            const int n1 = 1 + i + j * (side + 1);
            deck += modelRecord("ShellQuad4", eid++, 1, n1, n1 + 1, n1 + side + 2, n1 + side + 1);
        }
    }
    const Model model = readModelText(deck);
    const std::uint16_t type = typeIndexOf<ShellQuad4Table>();
    const CardTable& table = *model.findTable(type);
    std::vector<std::uint32_t> rows;
    for (std::uint32_t row = 0; row < table.rows().rowCount(); ++row)
        rows.push_back(row);
    const QualityArrays serial = ShellQuality::compute(model, type, rows);
    base::ThreadPool pool(3);
    const QualityArrays threaded = ShellQuality::compute(model, type, rows, &pool);
    REQUIRE(serial.size() == rows.size());
    for (std::size_t index = 0; index < rows.size(); ++index)
    {
        const QualityValues one = ShellQuality::of(model, CardLocation{type, rows[index]});
        const QualityValues a = serial.at(index);
        const QualityValues b = threaded.at(index);
        REQUIRE(a.aspectRatio == one.aspectRatio);
        REQUIRE(a.skew == one.skew);
        REQUIRE(a.warp == one.warp);
        REQUIRE(a.taper == one.taper);
        REQUIRE(a.minAngle == one.minAngle);
        REQUIRE(a.maxAngle == one.maxAngle);
        REQUIRE(a.jacobianRatio == one.jacobianRatio);
        REQUIRE(b.aspectRatio == one.aspectRatio);
        REQUIRE(b.warp == one.warp);
        REQUIRE(b.jacobianMin == one.jacobianMin);
    }
    // The model has distorted elements: not all the same.
    REQUIRE(serial.warp[0] != serial.warp[side + 5]);
}

TEST_CASE("many solids give what one solid gives, with and without threads", "[analysis][quality]")
{
    std::string deck = std::string(PROPERTIES);
    const int side = 21;
    const int row = side + 1;
    for (int k = 0; k <= side; ++k)
    {
        for (int j = 0; j <= side; ++j)
        {
            for (int i = 0; i <= side; ++i)
                deck += modelRecord("Node", 1 + i + j * row + k * row * row, "", i + 0.1 * (j % 3),
                                    j + 0.05 * (k % 2) * i, k + 0.02 * i * j);
        }
    }
    int eid = 1;
    for (int k = 0; k < side; ++k)
    {
        for (int j = 0; j < side; ++j)
        {
            for (int i = 0; i < side; ++i)
            {
                const int n = 1 + i + j * row + k * row * row;
                deck += modelRecord("SolidHexa", eid++, 2, n, n + 1, n + 1 + row, n + row, n + row * row,
                                    n + 1 + row * row, n + 1 + row + row * row, n + row + row * row);
            }
        }
    }
    const Model model = readModelText(deck);
    const std::uint16_t type = typeIndexOf<SolidHexaTable>();
    const CardTable& table = *model.findTable(type);
    std::vector<std::uint32_t> rows;
    for (std::uint32_t at = 0; at < table.rows().rowCount(); ++at)
        rows.push_back(at);
    const QualityArrays serial = SolidQuality::compute(model, type, rows);
    base::ThreadPool pool(3);
    const QualityArrays threaded = SolidQuality::compute(model, type, rows, &pool);
    for (std::size_t index = 0; index < rows.size(); index += 7)
    {
        const QualityValues one = SolidQuality::of(model, CardLocation{type, rows[index]});
        REQUIRE(serial.aspectRatio[index] == one.aspectRatio);
        REQUIRE(serial.jacobianMin[index] == one.jacobianMin);
        REQUIRE(serial.faceWarp[index] == one.faceWarp);
        REQUIRE(threaded.aspectRatio[index] == one.aspectRatio);
        REQUIRE(threaded.jacobianRatio[index] == one.jacobianRatio);
        REQUIRE(threaded.minAngle[index] == one.minAngle);
    }
    REQUIRE(serial.jacobianRatio[0] < 1.0);
}

TEST_CASE("a node that does not exist is reported with the element", "[analysis][quality]")
{
    const Model model = readModelText(std::string(PROPERTIES) + gridsOf({{0, 0, 0}, {1, 0, 0}, {1, 1, 0}}) +
                                      "ShellQuad4;10;1;1;2;3;99\n");
    REQUIRE_THROWS_WITH(qualityOf(model, 10),
                        Catch::Matchers::ContainsSubstring("ShellQuad4 10") && Catch::Matchers::ContainsSubstring("99"));
}
