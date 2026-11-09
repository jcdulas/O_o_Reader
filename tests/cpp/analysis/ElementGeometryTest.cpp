#include "oor/analysis/frames/FrameMath.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/model/cards/elements/solidHexa/SolidHexaTable.hpp"
#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/cards/elements/springScalarProperty/SpringScalarPropertyTable.hpp"
#include "oor/model/cards/rigid_elements/rigidBodyStar/RigidBodyStarTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceNodes/HeatSurfaceNodesTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceProperty/HeatSurfacePropertyTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceSide/HeatSurfaceSideTable.hpp"
#include "support/ModelRecord.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cmath>
#include <numbers>
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
    // The surface elements (CHBDYE, CHBDYG, CHBDYP) are an ID space of their own.
    auto location = model.locate(base::IdSpace::Element, eid);
    if (!location)
        location = model.locate(base::IdSpace::SurfaceElement, eid);
    REQUIRE(location.has_value());
    return ElementGeometry::of(model, *location);
}

void requireVec(const base::Vec3& actual, double x, double y, double z, double margin = EPS)
{
    REQUIRE(actual.x == Approx(x).margin(margin));
    REQUIRE(actual.y == Approx(y).margin(margin));
    REQUIRE(actual.z == Approx(z).margin(margin));
}

void requireFrame(const base::Mat3& frame, const base::Vec3& x, const base::Vec3& y, const base::Vec3& z,
                  double margin = EPS)
{
    requireVec(frame.column(0), x.x, x.y, x.z, margin);
    requireVec(frame.column(1), y.x, y.y, y.z, margin);
    requireVec(frame.column(2), z.x, z.y, z.z, margin);
}

// The record of a node: id, x, y, z (in the basic system).
std::string grid(int id, double x, double y, double z)
{
    return oor::testing::modelRecord("Node", id, "", x, y, z);
}

const char* const PROPERTIES = "IsoMat;1;2.1E+5;;0.3\nShellProp;1;1;0.1\n";

constexpr double ROOT_HALF = 0.70710678118654752440;

} // namespace

TEST_CASE("a triangle in the plane has its area, normal, centre, thickness, volume and frame", "[analysis][geometry]")
{
    const Model model = readModelText(std::string(PROPERTIES) + grid(1, 0, 0, 0) + grid(2, 1, 0, 0) + grid(3, 0, 1, 0) +
                                      "ShellTria3;10;1;1;2;3\n");
    const ElementValues values = geometryOf(model, 10);
    REQUIRE(values.area == Approx(0.5));
    requireVec(values.normal, 0.0, 0.0, 1.0);
    requireVec(values.center, 1.0 / 3.0, 1.0 / 3.0, 0.0);
    REQUIRE(values.thickness == Approx(0.1));
    REQUIRE(values.volume == Approx(0.05));
    requireFrame(values.frame, {1, 0, 0}, {0, 1, 0}, {0, 0, 1});
    requireFrame(values.materialFrame, {1, 0, 0}, {0, 1, 0}, {0, 0, 1});
    REQUIRE(std::isnan(values.length));
}

TEST_CASE("a tilted triangle has the normal of the right-hand rule on its connection order", "[analysis][geometry]")
{
    const Model model = readModelText(std::string(PROPERTIES) + grid(1, 0, 0, 0) + grid(2, 1, 0, 0) + grid(3, 0, 1, 1) +
                                      "ShellTria3;10;1;1;2;3\nShellTria3;11;1;1;3;2\n");
    const ElementValues values = geometryOf(model, 10);
    REQUIRE(values.area == Approx(0.5 * std::sqrt(2.0)));
    requireVec(values.normal, 0.0, -ROOT_HALF, ROOT_HALF);
    requireFrame(values.frame, {1, 0, 0}, {0, ROOT_HALF, ROOT_HALF}, {0, -ROOT_HALF, ROOT_HALF});
    requireVec(geometryOf(model, 11).normal, 0.0, ROOT_HALF, -ROOT_HALF);
}

TEST_CASE("a material angle turns the material direction about the normal", "[analysis][geometry]")
{
    const Model model = readModelText(std::string(PROPERTIES) + grid(1, 0, 0, 0) + grid(2, 1, 0, 0) + grid(3, 0, 1, 0) +
                                      "ShellTria3;10;1;1;2;3;90.\n");
    const ElementValues values = geometryOf(model, 10);
    requireFrame(values.materialFrame, {0, 1, 0}, {-1, 0, 0}, {0, 0, 1});
    requireFrame(values.frame, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}); // the element system does not turn
}

TEST_CASE("a unit square quadrilateral", "[analysis][geometry]")
{
    const Model model = readModelText(std::string(PROPERTIES) + grid(1, 0, 0, 0) + grid(2, 1, 0, 0) + grid(3, 1, 1, 0) +
                                      grid(4, 0, 1, 0) + "ShellQuad4;10;1;1;2;3;4\n");
    const ElementValues values = geometryOf(model, 10);
    REQUIRE(values.area == Approx(1.0));
    requireVec(values.center, 0.5, 0.5, 0.0);
    requireVec(values.normal, 0.0, 0.0, 1.0);
    REQUIRE(values.volume == Approx(0.1));
    requireFrame(values.frame, {1, 0, 0}, {0, 1, 0}, {0, 0, 1});
}

TEST_CASE("the element axis of a skewed quadrilateral bisects its diagonals, the material angle starts at G1-G2",
          "[analysis][geometry]")
{
    // A parallelogram with the diagonals (3, 1) and (-1, 1): the unit vectors differ, so the bisector is not the edge
    // G1-G2.
    const Model model = readModelText(std::string(PROPERTIES) + grid(1, 0, 0, 0) + grid(2, 2, 0, 0) + grid(3, 3, 1, 0) +
                                      grid(4, 1, 1, 0) + "ShellQuad4;10;1;1;2;3;4\nShellQuad4;11;1;1;2;3;4;30.\n");
    const ElementValues values = geometryOf(model, 10);
    REQUIRE(values.area == Approx(2.0));
    const base::Vec3 diagonal13{3.0, 1.0, 0.0};
    const base::Vec3 diagonal24{-1.0, 1.0, 0.0};
    const base::Vec3 expectedX = base::normalized(base::normalized(diagonal13) - base::normalized(diagonal24));
    requireVec(values.frame.column(0), expectedX.x, expectedX.y, 0.0);
    // The material x axis with THETA = 0 is G1-G2, with THETA = 30 degrees it is turned about z.
    requireVec(values.materialFrame.column(0), 1.0, 0.0, 0.0);
    const double angle = 30.0 * std::numbers::pi / 180.0;
    requireVec(geometryOf(model, 11).materialFrame.column(0), std::cos(angle), std::sin(angle), 0.0);
}

TEST_CASE("a warped quadrilateral has the area of the half cross product of its diagonals", "[analysis][geometry]")
{
    const Model model = readModelText(std::string(PROPERTIES) + grid(1, 0, 0, 0) + grid(2, 1, 0, 0) +
                                      grid(3, 1, 1, 0.2) + grid(4, 0, 1, 0) + "ShellQuad4;10;1;1;2;3;4\n");
    const ElementValues values = geometryOf(model, 10);
    REQUIRE(values.area == Approx(0.5 * std::sqrt(0.2 * 0.2 + 0.2 * 0.2 + 4.0)));
    requireVec(values.normal, -0.2 / (2.0 * values.area), -0.2 / (2.0 * values.area), 2.0 / (2.0 * values.area));
}

TEST_CASE("the corner thicknesses, the thickness flag and the laminates give the effective thickness",
          "[analysis][geometry]")
{
    const Model model =
        readModelText(std::string(PROPERTIES) + "CompProp;2;;;;;;;Symmetry;1;0.1;0.;Yes;1;0.2;45.\n" +
                      grid(1, 0, 0, 0) + grid(2, 1, 0, 0) + grid(3, 1, 1, 0) + grid(4, 0, 1, 0) +
                      "ShellQuad4;10;1;1;2;3;4;;;;0;0.2;0.2;0.4;0.4\n" + "ShellQuad4;11;1;1;2;3;4;;;;1;0.5;2.\n" +
                      "ShellQuad4;12;2;1;2;3;4;;;;0;0.9;0.9;0.9;0.9\n" + "ShellQuad4;13;1;1;2;3;4\n");
    REQUIRE(geometryOf(model, 10).thickness == Approx(0.3));
    // TFLAG 1: the values are fractions of 0.1 and a blank corner has the thickness of the property: (0.05 + 0.2 + 0.1
    // + 0.1) / 4.
    REQUIRE(geometryOf(model, 11).thickness == Approx(0.1125));
    // A symmetric laminate of two plies of 0.1 and 0.2 has the thickness 0.6 whatever the corners say.
    REQUIRE(geometryOf(model, 12).thickness == Approx(0.6));
    REQUIRE(geometryOf(model, 13).thickness == Approx(0.1));
}

TEST_CASE("mid-side nodes of a shell do not change its corner geometry, and a missing property gives no thickness",
          "[analysis][geometry]")
{
    const Model model =
        readModelText(std::string(PROPERTIES) + grid(1, 0, 0, 0) + grid(2, 2, 0, 0) + grid(3, 2, 2, 0) +
                      grid(4, 0, 2, 0) + grid(5, 1, 0, 0.3) + grid(6, 2, 1, 0) + grid(7, 1, 2, 0) + grid(8, 0, 1, 0) +
                      "ShellQuad8;10;1;1;2;3;4;5;6;7;8\nShellTria6;11;1;1;2;3;5;6;7\n" + "ShellQuad4;12;9;1;2;3;4\n");
    const ElementValues quad = geometryOf(model, 10);
    REQUIRE(quad.area == Approx(4.0));
    requireVec(quad.center, 1.0, 1.0, 0.0);
    REQUIRE(geometryOf(model, 11).area == Approx(2.0));
    REQUIRE(std::isnan(geometryOf(model, 12).thickness));
}

TEST_CASE("a material coordinate system projects its axis on the element, r through the element centre for a cylinder",
          "[analysis][geometry]")
{
    // A quadrilateral around the point (0, 5, 0) in the x-y plane; the cylindrical system has its axis along z.
    const Model model =
        readModelText(std::string(PROPERTIES) +
                      "CylindricalFrame2;3;;0.;0.;0.;0.;0.;1.;1.;0.;0.\nRectFrame2;4;;0.;0.;0.;1.;0.;1.;1.;1.;0.\n" +
                      grid(1, -1, 4, 0) + grid(2, 1, 4, 0) + grid(3, 1, 6, 0) + grid(4, -1, 6, 0) +
                      "ShellQuad4;10;1;1;2;3;4;;3\nShellQuad4;11;1;1;2;3;4;;4\n");
    // The r axis at the centre (0, 5, 0) is +y: the material x axis is +y, the material y axis is -x.
    requireFrame(geometryOf(model, 10).materialFrame, {0, 1, 0}, {-1, 0, 0}, {0, 0, 1});
    // The rectangular system 4 has its x axis along (1, 1, 0) / sqrt 2 in the basic system... its x axis projected: the
    // system is defined by the z axis through (1, 0, 1) and the x-z plane through (1, 1, 0): its x axis is +y.
    const base::Mat3 rectangular = geometryOf(model, 11).materialFrame;
    REQUIRE(std::isfinite(rectangular(0, 0)));
}

TEST_CASE("an axisymmetric ring has the volume its cross-section sweeps", "[analysis][geometry]")
{
    const Model model = readModelText(std::string(PROPERTIES) + grid(1, 1, 0, 0) + grid(2, 2, 0, 0) + grid(3, 2, 0, 1) +
                                      grid(4, 1, 0, 1) + "AxisymQuad;10;1;1;2;3;4\n");
    const ElementValues values = geometryOf(model, 10);
    REQUIRE(values.area == Approx(1.0));
    REQUIRE(values.volume == Approx(2.0 * std::numbers::pi * 1.5));
    REQUIRE(std::isnan(values.thickness));
}

TEST_CASE(
    "solid elements have the isoparametric volume, the centre of their corners and the format reference element system",
    "[analysis][geometry]")
{
    // A unit cube: nodes 1-8, the face G1-G4 at z = 0.
    std::string deck = "IsoMat;1;2.1E+5;;0.3\nSolidProp;1;1\n";
    const double cube[8][3] = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
    for (int node = 0; node < 8; ++node)
        deck += grid(node + 1, cube[node][0], cube[node][1], cube[node][2]);
    deck += "SolidHexa;10;1;1;2;3;4;5;6;7;8\n";
    deck += "SolidPenta;11;1;1;2;4;5;6;8\n"; // the half of the cube below the diagonal plane: a prism of volume 0.5
    deck += "SolidTetra;12;1;1;2;4;5\n";     // the corner tetrahedron: volume 1/6
    deck += "SolidPyramid;13;1;1;2;3;4;7\n"; // the base at z = 0 and the apex at (1, 1, 1): volume 1/3
    const Model model = readModelText(deck);
    const ElementValues hexa = geometryOf(model, 10);
    REQUIRE(hexa.volume == Approx(1.0));
    requireVec(hexa.center, 0.5, 0.5, 0.5);
    requireFrame(hexa.frame, {1, 0, 0}, {0, 1, 0}, {0, 0, 1});
    REQUIRE(std::isnan(hexa.area));
    const ElementValues prism = geometryOf(model, 11);
    REQUIRE(prism.volume == Approx(0.5));
    requireVec(prism.center, 1.0 / 3.0, 1.0 / 3.0, 0.5);
    REQUIRE(geometryOf(model, 12).volume == Approx(1.0 / 6.0));
    const ElementValues pyramid = geometryOf(model, 13);
    REQUIRE(pyramid.volume == Approx(1.0 / 3.0));
    requireVec(pyramid.center, 0.6, 0.6, 0.2);
}

TEST_CASE("solid volumes with curved edges follow the isoparametric mapping", "[analysis][geometry]")
{
    // The unit cube mapped by x -> x (1 + b x): its Jacobian determinant is 1 + 2 b x, the volume 1 + b. The twenty
    // nodes are the mapped nodes, so the quadratic mapping is exact.
    const double b = 0.3;
    const auto map = [&](double x, double y, double z) { return grid(0, x * (1.0 + b * x), y, z); };
    std::string deck = "IsoMat;1;2.1E+5;;0.3\nSolidProp;1;1\n";
    const double reference[20][3] = {{0, 0, 0},   {1, 0, 0},   {1, 1, 0},   {0, 1, 0},   {0, 0, 1},
                                     {1, 0, 1},   {1, 1, 1},   {0, 1, 1},   {0.5, 0, 0}, {1, 0.5, 0},
                                     {0.5, 1, 0}, {0, 0.5, 0}, {0, 0, 0.5}, {1, 0, 0.5}, {1, 1, 0.5},
                                     {0, 1, 0.5}, {0.5, 0, 1}, {1, 0.5, 1}, {0.5, 1, 1}, {0, 0.5, 1}};
    for (int node = 0; node < 20; ++node)
    {
        std::string line = map(reference[node][0], reference[node][1], reference[node][2]);
        line.replace(5, 1, std::to_string(node + 1)); // "Node;0" -> the node number
        deck += line;
    }
    deck += "SolidHexa;10;1;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16;17;18;19;20\n";
    const Model model = readModelText(deck);
    REQUIRE(geometryOf(model, 10).volume == Approx(1.0 + b));

    // The same unit prism and tetrahedron, with their edge nodes on the curve x -> x (1 + b x).
    const auto onCurve = [&](int id, double x, double y, double z) { return grid(id, x * (1.0 + b * x), y, z); };
    std::string wedge = "IsoMat;1;2.1E+5;;0.3\nSolidProp;1;1\n";
    const double prism[15][3] = {{0, 0, 0},   {1, 0, 0},   {0, 1, 0},     {0, 0, 1},     {1, 0, 1},
                                 {0, 1, 1},   {0.5, 0, 0}, {0.5, 0.5, 0}, {0, 0.5, 0},   {0, 0, 0.5},
                                 {1, 0, 0.5}, {0, 1, 0.5}, {0.5, 0, 1},   {0.5, 0.5, 1}, {0, 0.5, 1}};
    for (int node = 0; node < 15; ++node)
        wedge += onCurve(node + 1, prism[node][0], prism[node][1], prism[node][2]);
    wedge += "SolidPenta;10;1;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15\n";
    // Its volume: the integral of 1 + 2 b x over the prism, 1/2 + b/3.
    REQUIRE(geometryOf(readModelText(wedge), 10).volume == Approx(0.5 + b / 3.0));

    std::string tetra = "IsoMat;1;2.1E+5;;0.3\nSolidProp;1;1\n";
    const double tet[10][3] = {{0, 0, 0},     {1, 0, 0},   {0, 1, 0},   {0, 0, 1},     {0.5, 0, 0},
                               {0.5, 0.5, 0}, {0, 0.5, 0}, {0, 0, 0.5}, {0.5, 0, 0.5}, {0, 0.5, 0.5}};
    for (int node = 0; node < 10; ++node)
        tetra += onCurve(node + 1, tet[node][0], tet[node][1], tet[node][2]);
    tetra += "SolidTetra;10;1;1;2;3;4;5;6;7;8;9;10\n";
    // 1/6 + b/12.
    REQUIRE(geometryOf(readModelText(tetra), 10).volume == Approx(1.0 / 6.0 + b / 12.0));
}

TEST_CASE("a rod has its length, centre, area and volume; a spring has its length and no area", "[analysis][geometry]")
{
    const Model model = readModelText("IsoMat;1;2.1E+5;;0.3\nRodProp;1;1;0.5\nTubeProp;2;1;2.;0.5\n" +
                                      grid(1, 0, 0, 0) + grid(2, 3, 4, 0) +
                                      "Rod2;10;1;1;2\n"
                                      "Tube2;11;2;1;2\n"
                                      "Rod2InlineSection;12;1;2;1;0.25\n"
                                      "SpringDofInline;13;100.;1;1;2;1\n"
                                      "ScalarPoint;50\n"
                                      "SpringDofProperty;14;5;1;1;50\n"
                                      "SpringProp;5;100.\n");
    const ElementValues rod = geometryOf(model, 10);
    REQUIRE(rod.length == Approx(5.0));
    requireVec(rod.center, 1.5, 2.0, 0.0);
    REQUIRE(rod.area == Approx(0.5));
    REQUIRE(rod.volume == Approx(2.5));
    REQUIRE(std::isnan(rod.thickness));
    REQUIRE(std::isnan(rod.normal.x));
    // A tube of outer diameter 2 and wall 0.5: the ring between the diameters 2 and 1.
    REQUIRE(geometryOf(model, 11).area == Approx(std::numbers::pi / 4.0 * (4.0 - 1.0)));
    REQUIRE(geometryOf(model, 12).area == Approx(0.25));
    const ElementValues spring = geometryOf(model, 13);
    REQUIRE(spring.length == Approx(5.0));
    REQUIRE(std::isnan(spring.area));
    // A spring to a scalar point: the centre is its grid point, no length.
    const ElementValues grounded = geometryOf(model, 14);
    requireVec(grounded.center, 0.0, 0.0, 0.0);
    REQUIRE(std::isnan(grounded.length));
}

TEST_CASE("a bar has its element system from the orientation vector and its offsets move its ends",
          "[analysis][geometry]")
{
    const Model model = readModelText(
        "IsoMat;1;2.1E+5;;0.3\nBarProp;1;1;0.02;1.E-5;1.E-5;1.E-5\n" + grid(1, 0, 0, 0) + grid(2, 2, 0, 0) +
        grid(3, 0, 5, 0) +
        "Bar2;10;1;1;2;0.;;1.;0.\n"
        "Bar2;11;1;1;2;;3\n"
        "Bar2;12;1;1;2;0.;;1.;0.;OffsetsGlobalGlobalGlobal;;;0.;0.;1.\n"
        "Bar2;13;1;1;2;0.;;0.;1.;OffsetsBasicOrientOrient;;;0.;1.;0.;0.;1.;0.\n"); // offsets in the offset system at
                                                                                      // both ends
    const ElementValues plain = geometryOf(model, 10);
    REQUIRE(plain.length == Approx(2.0));
    requireFrame(plain.frame, {1, 0, 0}, {0, 1, 0}, {0, 0, 1});
    REQUIRE(plain.area == Approx(0.02));
    REQUIRE(plain.volume == Approx(0.04));
    requireFrame(geometryOf(model, 11).frame, {1, 0, 0}, {0, 1, 0}, {0, 0, 1});
    const ElementValues shifted = geometryOf(model, 12);
    REQUIRE(shifted.length == Approx(std::sqrt(5.0)));
    requireVec(shifted.center, 1.0, 0.0, 0.5);
    requireVec(shifted.frame.column(0), 2.0 / std::sqrt(5.0), 0.0, -1.0 / std::sqrt(5.0));
    // The offset system has x from GA to GB, z = x cross v (v along +z gives -y)... the offsets are along its y axis.
    const ElementValues offsetSystem = geometryOf(model, 13);
    REQUIRE(offsetSystem.length == Approx(2.0));
    requireVec(offsetSystem.center, 1.0, 0.0, 1.0);
}

TEST_CASE("a concentrated mass is where its offset puts it", "[analysis][geometry]")
{
    const Model model = readModelText("CylindricalFrame2;2;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n" + grid(1, 0, 3, 0) +
                                      "PointMassInertia;10;1;;5.;1.;2.;3.\n"
                                      "PointMassInertia;11;1;0;5.;1.;2.;3.\n"
                                      "PointMassInertia;12;1;2;5.;1.;2.;3.\n"
                                      "PointMassInertia;13;1;-1;5.;1.;2.;3.\n"
                                      "PointMassMatrix;14;1\n");
    requireVec(geometryOf(model, 10).center, 1.0, 5.0, 3.0);
    requireVec(geometryOf(model, 11).center, 1.0, 5.0, 3.0);
    // CID 2 is cylindrical: at the point (0, 3, 0) the radial direction is y, the tangential one -x: (1, 2, 3) is
    // 1 along r (y), 2 along theta (-x), 3 along z.
    requireVec(geometryOf(model, 12).center, -2.0, 4.0, 3.0);
    requireVec(geometryOf(model, 13).center, 1.0, 2.0, 3.0);
    requireVec(geometryOf(model, 14).center, 0.0, 3.0, 0.0);
}

TEST_CASE("a bush is at S of the way from GA to GB, in the system CID, or at the point of OCID", "[analysis][geometry]")
{
    const Model model =
        readModelText("BushProp;1;Stiffness;100.\n" + grid(1, 0, 0, 0) + grid(2, 4, 0, 0) + grid(3, 0, 1, 0) +
                      "Bushing;10;1;1;2;;0.;1.;0.\n"
                      "Bushing;11;1;1;2;3;;;;;0.25\n"
                      "Bushing;12;1;1;2;;;;;0\n"
                      "Bushing;13;1;1;2;;;;;;;0;1.;2.;3.\n"); // OCID 0: the point (1, 2, 3)
    const ElementValues first = geometryOf(model, 10);
    requireVec(first.center, 2.0, 0.0, 0.0);
    REQUIRE(first.length == Approx(4.0));
    requireFrame(first.frame, {1, 0, 0}, {0, 1, 0}, {0, 0, 1});
    requireVec(geometryOf(model, 11).center, 1.0, 0.0, 0.0);
    requireFrame(geometryOf(model, 12).frame, {1, 0, 0}, {0, 1, 0}, {0, 0, 1});
    requireVec(geometryOf(model, 13).center, 1.0, 2.0, 3.0);
}

TEST_CASE("rigid elements have the centre of their grid points, and the length between two", "[analysis][geometry]")
{
    const Model model = readModelText(
        grid(1, 0, 0, 0) + grid(2, 2, 0, 0) + grid(3, 2, 2, 0) + grid(4, 0, 2, 0) +
        "RigidBodyStar;10;1;123456;2;3;4\nRigidBar;11;1;2\nRigidInterpolation;12;1;123;1;1.;123;3;2;3;4\n");
    requireVec(geometryOf(model, 10).center, 1.0, 1.0, 0.0);
    const ElementValues bar = geometryOf(model, 11);
    REQUIRE(bar.length == Approx(2.0));
    requireVec(bar.center, 1.0, 0.0, 0.0);
    requireVec(geometryOf(model, 12).center, 1.0, 1.0, 0.0);
}

TEST_CASE("a node that does not exist is reported with the element", "[analysis][geometry]")
{
    const Model model =
        readModelText(std::string(PROPERTIES) + grid(1, 0, 0, 0) + grid(2, 1, 0, 0) + "ShellTria3;10;1;1;2;77\n");
    const auto location = model.locate(base::IdSpace::Element, 10);
    try
    {
        (void)ElementGeometry::of(model, *location);
        FAIL("no error was raised");
    }
    catch (const GeometryError& error)
    {
        REQUIRE_THAT(std::string(error.what()),
                     Catch::Matchers::ContainsSubstring("ShellTria3 10") && Catch::Matchers::ContainsSubstring("77"));
    }
}

TEST_CASE("the geometry of many elements equals the geometry one by one, with and without threads",
          "[analysis][geometry]")
{
    std::string deck = std::string(PROPERTIES);
    const int side = 100;
    for (int j = 0; j <= side; ++j)
    {
        for (int i = 0; i <= side; ++i)
            deck += grid(1 + i + j * (side + 1), i * 0.5, j * 0.4 + 0.01 * i * j, 0.02 * i);
    }
    int eid = 1;
    for (int j = 0; j < side; ++j)
    {
        for (int i = 0; i < side; ++i)
        {
            const int n1 = 1 + i + j * (side + 1);
            deck += oor::testing::modelRecord("ShellQuad4", eid++, 1, n1, n1 + 1, n1 + side + 2, n1 + side + 1);
        }
    }
    const Model model = readModelText(deck);
    const std::uint16_t type = typeIndexOf<ShellQuad4Table>();
    const CardTable& table = *model.findTable(type);
    std::vector<std::uint32_t> rows;
    for (std::uint32_t row = 0; row < table.rows().rowCount(); ++row)
        rows.push_back(row);
    const ElementArrays all = ElementGeometry::compute(model, type, rows, ALL_QUANTITIES);
    base::ThreadPool pool(3);
    const ElementArrays threaded = ElementGeometry::compute(model, type, rows, ALL_QUANTITIES, &pool);
    for (std::size_t index = 0; index < rows.size(); ++index)
    {
        const ElementValues one = ElementGeometry::of(model, CardLocation{type, rows[index]});
        REQUIRE(all.areas[index] == one.area);
        REQUIRE(all.centers[index] == one.center);
        REQUIRE(all.normals[index] == one.normal);
        REQUIRE(all.frames[index] == one.frame);
        REQUIRE(all.materialFrames[index] == one.materialFrame);
        REQUIRE(all.thicknesses[index] == one.thickness);
        REQUIRE(threaded.areas[index] == one.area);
        REQUIRE(threaded.frames[index] == one.frame);
    }
    // Asking for one quantity fills one array.
    const ElementArrays areas = ElementGeometry::compute(model, type, rows, bit(Quantity::Area));
    REQUIRE(areas.areas == all.areas);
    REQUIRE(areas.centers.empty());
}

TEST_CASE("a card type with no geometry gives NaN, and the card types with geometry are the elements",
          "[analysis][geometry]")
{
    const Model model = readModelText("ScalarPoint;1;2\nSpringScalarProperty;10;5;1;2\nSpringProp;5;100.\n");
    const std::uint16_t type = typeIndexOf<SpringScalarPropertyTable>();
    REQUIRE_FALSE(ElementGeometry::supports(type));
    const std::uint32_t rows[1] = {0};
    const ElementArrays arrays = ElementGeometry::compute(model, type, rows, ALL_QUANTITIES);
    REQUIRE(std::isnan(arrays.areas[0]));
    REQUIRE(ElementGeometry::supports(typeIndexOf<ShellQuad4Table>()));
    REQUIRE(ElementGeometry::supports(typeIndexOf<SolidHexaTable>()));
    REQUIRE(ElementGeometry::supports(typeIndexOf<RigidBodyStarTable>()));
}

TEST_CASE("the polar rotation is the rotation closest to a matrix", "[analysis][geometry]")
{
    // A rotation about z by 30 degrees times a symmetric stretch gives back the rotation.
    const double angle = 30.0 * std::numbers::pi / 180.0;
    const base::Mat3 rotation = base::rotationAboutAxis({0, 0, 1}, angle);
    base::Mat3 stretch;
    stretch(0, 0) = 2.0;
    stretch(1, 1) = 0.5;
    stretch(0, 1) = stretch(1, 0) = 0.1;
    const base::Mat3 recovered = polarRotation(rotation * stretch);
    for (std::size_t index = 0; index < 9; ++index)
        REQUIRE(recovered.values[index] == Approx(rotation.values[index]).margin(1.0e-12));
}

namespace
{

// Nodes and conduction elements the surface elements below are sides of: a unit cube (SolidHexa 100), a tetrahedron
// (SolidTetra 300), a prism (SolidPenta 320), a unit square shell (ShellQuad4 400, 0.1 thick), two rods along z
// (Rod2 500 and Rod2InlineSection 501).
std::string surfaceBase()
{
    std::string deck =
        "IsoMat;1;2.1E+5;;0.3\nShellProp;1;1;0.1\nSolidProp;2;1\nRodProp;3;1;0.5\n"
        "HeatSurfaceProp;1;0.5\nHeatSurfaceProp;2;;1.;0.5\n";
    deck += grid(1, 0, 0, 0) + grid(2, 1, 0, 0) + grid(3, 1, 1, 0) + grid(4, 0, 1, 0);
    deck += grid(5, 0, 0, 1) + grid(6, 1, 0, 1) + grid(7, 1, 1, 1) + grid(8, 0, 1, 1);
    deck += "SolidHexa;100;2;1;2;3;4;5;6;7;8\n";
    deck += grid(11, 0, 0, 0) + grid(12, 2, 0, 0) + grid(13, 0, 2, 0) + grid(14, 0, 0, 2);
    deck += "SolidTetra;300;2;11;12;13;14\n";
    deck += grid(21, 0, 0, 0) + grid(22, 1, 0, 0) + grid(23, 0, 1, 0) + grid(24, 0, 0, 2) + grid(25, 1, 0, 2) +
            grid(26, 0, 1, 2);
    deck += "SolidPenta;320;2;21;22;23;24;25;26\n";
    deck += grid(31, 0, 0, 0) + grid(32, 1, 0, 0) + grid(33, 1, 1, 0) + grid(34, 0, 1, 0);
    deck += "ShellQuad4;400;1;31;32;33;34\n";
    deck += grid(41, 0, 0, 0) + grid(42, 0, 0, 3);
    deck += "Rod2;500;3;41;42\nRod2InlineSection;501;41;42;1;0.75\n";
    deck += grid(51, 0, 0, 0) + grid(52, 0, 0, 1) + grid(53, 2, 0, 0);
    return deck;
}

} // namespace

TEST_CASE("the six sides of a CHBDYE on a cube are its faces, the normals pointing out",
          "[analysis][geometry][thermal]")
{
    std::string deck = surfaceBase();
    for (int side = 1; side <= 6; ++side)
        deck += oor::testing::modelRecord("HeatSurfaceSide", 200 + side, 100, side);
    const Model model = readModelText(deck);
    struct Expected
    {
        base::Vec3 normal;
        base::Vec3 center;
    };
    const Expected expected[6] = {{{0, 0, -1}, {0.5, 0.5, 0.0}}, {{0, -1, 0}, {0.5, 0.0, 0.5}},
                                  {{1, 0, 0}, {1.0, 0.5, 0.5}},  {{0, 1, 0}, {0.5, 1.0, 0.5}},
                                  {{-1, 0, 0}, {0.0, 0.5, 0.5}}, {{0, 0, 1}, {0.5, 0.5, 1.0}}};
    for (int side = 1; side <= 6; ++side)
    {
        const ElementValues values = geometryOf(model, 200 + side);
        REQUIRE(values.area == Approx(1.0));
        requireVec(values.normal, expected[side - 1].normal.x, expected[side - 1].normal.y,
                   expected[side - 1].normal.z);
        requireVec(values.center, expected[side - 1].center.x, expected[side - 1].center.y,
                   expected[side - 1].center.z);
        REQUIRE(std::isnan(values.length));
        REQUIRE(std::isnan(values.volume));
    }
}

TEST_CASE("the sides of a tetrahedron and of a prism have their own corners, three or four",
          "[analysis][geometry][thermal]")
{
    const Model model = readModelText(surfaceBase() +
                                      "HeatSurfaceSide;301;300;1\n"
                                      "HeatSurfaceSide;303;300;3\n"
                                      "HeatSurfaceSide;321;320;1\n"
                                      "HeatSurfaceSide;322;320;2\n"
                                      "HeatSurfaceSide;323;320;3\n"
                                      "HeatSurfaceSide;325;320;5\n"
                                      "HeatSurfaceSide;326;320;6\n");
    ElementValues values = geometryOf(model, 301);
    REQUIRE(values.area == Approx(2.0));
    requireVec(values.normal, 0, 0, -1);
    values = geometryOf(model, 303);
    REQUIRE(values.area == Approx(0.5 * std::sqrt(48.0)));
    requireVec(values.normal, 1.0 / std::sqrt(3.0), 1.0 / std::sqrt(3.0), 1.0 / std::sqrt(3.0));
    values = geometryOf(model, 321); // the triangle at the bottom of the prism
    REQUIRE(values.area == Approx(0.5));
    requireVec(values.normal, 0, 0, -1);
    values = geometryOf(model, 322); // the face on y = 0
    REQUIRE(values.area == Approx(2.0));
    requireVec(values.normal, 0, -1, 0);
    values = geometryOf(model, 323); // the face on the hypotenuse
    REQUIRE(values.area == Approx(2.0 * std::sqrt(2.0)));
    requireVec(values.normal, ROOT_HALF, ROOT_HALF, 0);
    values = geometryOf(model, 325); // the triangle at the top
    REQUIRE(values.area == Approx(0.5));
    requireVec(values.normal, 0, 0, 1);
    REQUIRE(std::isnan(geometryOf(model, 326).area)); // a prism has five sides
}

TEST_CASE("a CHBDYE on a shell has the shell as top and bottom, and its edges as lines with the thickness as width",
          "[analysis][geometry][thermal]")
{
    const Model model = readModelText(surfaceBase() +
                                      "HeatSurfaceSide;401;400;1\nHeatSurfaceSide;402;400;"
                                      "2\nHeatSurfaceSide;403;400;3\nHeatSurfaceSide;406;400;6\n");
    ElementValues values = geometryOf(model, 401);
    REQUIRE(values.area == Approx(1.0));
    requireVec(values.normal, 0, 0, 1);
    values = geometryOf(model, 406);
    REQUIRE(values.area == Approx(1.0));
    requireVec(values.normal, 0, 0, -1);
    values = geometryOf(model, 402); // the edge G1-G2, on y = 0
    REQUIRE(values.length == Approx(1.0));
    REQUIRE(values.area == Approx(0.1));
    requireVec(values.center, 0.5, 0.0, 0.0);
    requireVec(values.normal, 0, -1, 0);
    values = geometryOf(model, 403); // the edge G2-G3, on x = 1
    requireVec(values.normal, 1, 0, 0);
}

TEST_CASE("a CHBDYE on a rod has a linear side and two end points with the cross-section",
          "[analysis][geometry][thermal]")
{
    const Model model = readModelText(surfaceBase() +
                                      "HeatSurfaceSide;551;500;1\n"
                                      "HeatSurfaceSide;552;500;2\n"
                                      "HeatSurfaceSide;553;500;3\n"
                                      "HeatSurfaceSide;562;501;2\n"
                                      "HeatSurfaceSide;563;500;4\n");
    ElementValues values = geometryOf(model, 551);
    REQUIRE(values.length == Approx(3.0));
    requireVec(values.center, 0, 0, 1.5);
    REQUIRE(std::isnan(values.area));
    values = geometryOf(model, 552);
    requireVec(values.center, 0, 0, 0);
    requireVec(values.normal, 0, 0, -1);
    REQUIRE(values.area == Approx(0.5)); // the A of PROD 3
    values = geometryOf(model, 553);
    requireVec(values.center, 0, 0, 3);
    requireVec(values.normal, 0, 0, 1);
    REQUIRE(geometryOf(model, 562).area == Approx(0.75)); // the area of the CONROD itself
    REQUIRE(std::isnan(geometryOf(model, 563).area));     // a rod has three sides
}

TEST_CASE("a CHBDYE with no conduction element, or on one with no side numbering, has no geometry",
          "[analysis][geometry][thermal]")
{
    const Model model = readModelText(
        surfaceBase() + "PlotLine2;800;41;42\nHeatSurfaceSide;601;999;1\nHeatSurfaceSide;602;800;1\n");
    for (const base::EntityId eid : {601, 602})
    {
        const ElementValues values = geometryOf(model, eid);
        REQUIRE(std::isnan(values.area));
        REQUIRE(std::isnan(values.center.x));
        REQUIRE(std::isnan(values.normal.x));
    }
}

TEST_CASE("a CHBDYG has the geometry of its type: triangle, quadrilateral, cone and the right-hand normal",
          "[analysis][geometry][thermal]")
{
    std::string deck = grid(1, 0, 0, 0) + grid(2, 2, 0, 0) + grid(3, 0, 2, 0);
    deck += grid(11, 0, 0, 0) + grid(12, 0, 1, 0) + grid(13, 0, 1, 1) + grid(14, 0, 0, 1);
    deck += grid(21, 1, 0, 0) + grid(22, 2, 0, 1) + grid(23, 1, 1, 0) + grid(24, 2, 1, 0);
    deck += "HeatSurfaceNodes;10;AreaTria3;;;;;1;2;3\n";
    deck += "HeatSurfaceNodes;11;AreaQuad4;;;;;11;12;13;14\n";
    deck += "HeatSurfaceNodes;12;ReversedNormal;;;;;21;22\n";
    deck += "HeatSurfaceNodes;13;ReversedNormal1;;;;;21;24\n";
    const Model model = readModelText(deck);
    ElementValues values = geometryOf(model, 10);
    REQUIRE(values.area == Approx(2.0));
    requireVec(values.normal, 0, 0, 1);
    requireVec(values.center, 2.0 / 3.0, 2.0 / 3.0, 0.0);
    values = geometryOf(model, 11);
    REQUIRE(values.area == Approx(1.0));
    requireVec(values.normal, 1, 0, 0);
    requireVec(values.center, 0.0, 0.5, 0.5);
    values = geometryOf(model, 12); // a cone from r = 1 to r = 2 over a height of 1
    REQUIRE(values.area == Approx(std::numbers::pi * 3.0 * std::sqrt(2.0)));
    REQUIRE(values.length == Approx(std::sqrt(2.0)));
    requireVec(values.normal, ROOT_HALF, 0, -ROOT_HALF);
    values = geometryOf(model, 13); // the same cone about the y axis, from y = 0 to y = 1
    REQUIRE(values.area == Approx(std::numbers::pi * 3.0 * std::sqrt(2.0)));
    requireVec(values.normal, ROOT_HALF, -ROOT_HALF, 0);
}

TEST_CASE("a CHBDYP has the area its PHBDY gives: AF for a point, AF by length for a line, the cone for a tube",
          "[analysis][geometry][thermal]")
{
    const Model model = readModelText(surfaceBase() +
                                      "HeatSurfaceProperty;701;1;Point;;;51;;52\n"
                                      "HeatSurfaceProperty;702;1;Line;;;51;53;52\n"
                                      "HeatSurfaceProperty;703;1;Line;;;51;53;;;;;0;1.;0.;1.\n"
                                      "HeatSurfaceProperty;704;2;ForcedConvectionTube;;;51;53\n"
                                      "HeatSurfaceProperty;705;1;EllipticCylinder;;;51;53;52\n"
                                      "HeatSurfaceProperty;706;9;Line;;;51;53;52\n");
    ElementValues values = geometryOf(model, 701);
    REQUIRE(values.area == Approx(0.5));
    requireVec(values.normal, 0, 0, 1);
    requireVec(values.center, 0, 0, 0);
    values = geometryOf(model, 702);
    REQUIRE(values.length == Approx(2.0));
    REQUIRE(values.area == Approx(1.0));
    requireVec(values.center, 1, 0, 0);
    requireVec(values.normal, 0, 0, 1);
    values = geometryOf(model, 703); // the orientation vector (1, 0, 1): its part across the line
    requireVec(values.normal, 0, 0, 1);
    values = geometryOf(model, 704);
    REQUIRE(values.area == Approx(std::numbers::pi * 0.75 * std::sqrt(4.0 + 0.0625)));
    REQUIRE(std::isnan(values.normal.x));
    values = geometryOf(model, 705);
    REQUIRE(values.area == Approx(1.0));
    requireVec(values.normal, 0, 0, 1);
    values = geometryOf(model, 706); // no PHBDY 9: the length and the centre only
    REQUIRE(values.length == Approx(2.0));
    REQUIRE(std::isnan(values.area));
}

TEST_CASE("the surface element card types have geometry", "[analysis][geometry][thermal]")
{
    REQUIRE(ElementGeometry::supports(typeIndexOf<HeatSurfaceSideTable>()));
    REQUIRE(ElementGeometry::supports(typeIndexOf<HeatSurfaceNodesTable>()));
    REQUIRE(ElementGeometry::supports(typeIndexOf<HeatSurfacePropertyTable>()));
}

TEST_CASE("the acoustic absorber is a surface with an area and the cavity elements are solids with a volume",
          "[analysis][geometry][acoustic]")
{
    std::string deck = grid(1, 0, 0, 0) + grid(2, 1, 0, 0) + grid(3, 1, 1, 0) + grid(4, 0, 1, 0);
    deck += grid(5, 0, 0, 1) + grid(6, 1, 0, 1) + grid(7, 1, 1, 1) + grid(8, 0, 1, 1);
    deck += "AcousticAbsorber;700;13;1;2;3;4\nAcousticAbsorber;701;13;1;2;3\n";
    deck += "AcousticAbsorberHexa;710;11;1;2;3;4;5;6;7;8\nAcousticBarrierHexa;711;12;1;2;3;4;5;6;7;8\n";
    const Model model = readModelText(deck);
    ElementValues values = geometryOf(model, 700);
    REQUIRE(values.area == Approx(1.0));
    requireVec(values.normal, 0, 0, 1);
    requireVec(values.center, 0.5, 0.5, 0.0);
    REQUIRE(std::isnan(values.thickness));
    REQUIRE(std::isnan(values.volume));
    values = geometryOf(model, 701);
    REQUIRE(values.area == Approx(0.5));
    requireVec(values.center, 2.0 / 3.0, 1.0 / 3.0, 0.0);
    for (const base::EntityId eid : {710, 711})
    {
        values = geometryOf(model, eid);
        REQUIRE(values.volume == Approx(1.0));
        requireVec(values.center, 0.5, 0.5, 0.5);
        REQUIRE(std::isnan(values.area));
    }
}

TEST_CASE("the marker elements of the Euler domain are a segment and a point", "[analysis][geometry]")
{
    const Model model = readModelText(grid(1, 0, 0, 0) + grid(2, 3, 4, 0) +
                                      "MarkerProp;7\nMarkerBody;100;7;1;2\nMarkerNode;101;7;2\n");
    const ElementValues segment = geometryOf(model, 100);
    REQUIRE(segment.length == Approx(5.0));
    requireVec(segment.center, 1.5, 2.0, 0.0);
    const ElementValues point = geometryOf(model, 101);
    requireVec(point.center, 3.0, 4.0, 0.0);
    REQUIRE(std::isnan(point.length));
}
