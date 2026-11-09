#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/analysis/coordinates/CoordinateErrors.hpp"
#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/model/cards/coordinate_systems/rectFrame2/RectFrame2Table.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

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

void requireNear(const base::Vec3& actual, double x, double y, double z, double margin = EPS)
{
    REQUIRE(actual.x == Approx(x).margin(margin));
    REQUIRE(actual.y == Approx(y).margin(margin));
    REQUIRE(actual.z == Approx(z).margin(margin));
}

// The message of the coordinate system error that computing the positions of a model raises.
std::string errorMessage(const Model& model)
{
    try
    {
        (void)BasicPositionCache::of(model);
    }
    catch (const CoordinateSystemError& error)
    {
        return error.what();
    }
    FAIL("no error was raised");
    return {};
}

base::Vec3 basicPosition(const Model& model, base::EntityId nid)
{
    const auto location = model.locate(base::IdSpace::Node, nid);
    REQUIRE(location.has_value());
    return BasicPositionCache::of(model).position(location->row);
}

} // namespace

TEST_CASE("a rectangular system rotated and moved places a grid point in the basic system", "[analysis][coordinates]")
{
    // The system 1 has its origin at (1, 0, 0), its z axis along z and its x axis along the basic y axis.
    const Model model = readModelText(
        "RectFrame2;1;;1.;0.;0.;1.;0.;1.;1.;1.;0.\n"
        "Node;10;1;1.;0.;0.\n"
        "Node;11;1;0.;2.;3.\n"
        "Node;12;;5.;6.;7.\n");
    requireNear(basicPosition(model, 10), 1.0, 1.0, 0.0);
    requireNear(basicPosition(model, 11), 1.0 - 2.0, 0.0, 3.0);
    requireNear(basicPosition(model, 12), 5.0, 6.0, 7.0);
}

TEST_CASE("a cylindrical system reads radius, angle in degrees and axial coordinate", "[analysis][coordinates]")
{
    const Model model = readModelText(
        "CylindricalFrame2;2;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "Node;20;2;2.;90.;3.\n"
        "Node;21;2;2.;180.;-1.\n"
        "Node;22;2;0.;45.;4.\n");
    requireNear(basicPosition(model, 20), 0.0, 2.0, 3.0);
    requireNear(basicPosition(model, 21), -2.0, 0.0, -1.0);
    requireNear(basicPosition(model, 22), 0.0, 0.0, 4.0);
}

TEST_CASE("a spherical system reads radius, polar angle and azimuth", "[analysis][coordinates]")
{
    const Model model = readModelText(
        "SphericalFrame2;3;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "Node;30;3;2.;90.;0.\n"
        "Node;31;3;2.;0.;0.\n"
        "Node;32;3;1.;90.;90.\n"
        "Node;33;3;2.;60.;30.\n");
    requireNear(basicPosition(model, 30), 2.0, 0.0, 0.0);
    requireNear(basicPosition(model, 31), 0.0, 0.0, 2.0);
    requireNear(basicPosition(model, 32), 0.0, 1.0, 0.0);
    const double root3 = std::sqrt(3.0);
    requireNear(basicPosition(model, 33), 2.0 * root3 / 2.0 * root3 / 2.0, 2.0 * root3 / 2.0 * 0.5, 1.0);
}

TEST_CASE("systems chain through RID, and CORD1x systems through their grid points", "[analysis][coordinates]")
{
    // System 1: translated by (10, 0, 0). System 2 (cylindrical) is given in system 1. System 5 is made by grid points
    // that sit in system 1.
    const Model model = readModelText(
        "RectFrame2;1;;10.;0.;0.;10.;0.;1.;11.;0.;0.\n"
        "CylindricalFrame2;2;1;1.;0.;0.;1.;0.;1.;2.;0.;0.\n"
        "Node;40;2;1.;90.;5.\n"
        "Node;41;1;0.;0.;0.\n"
        "Node;42;1;0.;0.;1.\n"
        "Node;43;1;0.;1.;0.\n"
        "RectFrame1;5;41;42;43\n"
        "Node;44;5;1.;2.;3.\n");
    // 40: system 2 has its origin at (11, 0, 0) in the basic system; (R=1, theta=90, Z=5) is (0, 1, 5) from it.
    requireNear(basicPosition(model, 40), 11.0, 1.0, 5.0);
    // 44: system 5 has its origin at (10, 0, 0), z along z, x along the basic x axis (G3 is at +y?). G3 - G1 = (0, 1,
    // 0): the x axis is along +y and y along -x.
    requireNear(basicPosition(model, 44), 10.0 - 2.0, 1.0, 3.0);
}

TEST_CASE("a loop of reference systems is refused and the cycle is named", "[analysis][coordinates]")
{
    const Model model = readModelText(
        "RectFrame2;1;2;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "CylindricalFrame2;2;3;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "SphericalFrame2;3;1;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "Node;10;1;1.;0.;0.\n");
    REQUIRE_THAT(errorMessage(model),
                 Catch::Matchers::ContainsSubstring("loop") &&
                     Catch::Matchers::ContainsSubstring(
                         "RectFrame2 1 -> CylindricalFrame2 2 -> SphericalFrame2 3 -> RectFrame2 1"));
}

TEST_CASE("a loop through the grid points that define a system is refused", "[analysis][coordinates]")
{
    // Grid 7 is in the system 5, which is defined by the grid points 7, 8 and 9.
    const Model model = readModelText(
        "Node;7;5;1.;0.;0.\n"
        "Node;8;;0.;0.;1.\n"
        "Node;9;;1.;1.;0.\n"
        "RectFrame1;5;7;8;9\n");
    REQUIRE_THAT(errorMessage(model), Catch::Matchers::ContainsSubstring("RectFrame1 5 -> grid 7 -> RectFrame1 5"));
}

TEST_CASE("a missing coordinate system and degenerate points are refused with a message", "[analysis][coordinates]")
{
    const Model missing = readModelText("Node;7;9;1.;0.;0.\n");
    REQUIRE_THAT(errorMessage(missing), Catch::Matchers::ContainsSubstring("coordinate system 9"));
    const Model flat = readModelText(
        "RectFrame2;1;;0.;0.;0.;0.;0.;1.;0.;0.;2.\n"
        "Node;7;1;1.;0.;0.\n");
    REQUIRE_THAT(errorMessage(flat), Catch::Matchers::ContainsSubstring("RectFrame2 1"));
}

TEST_CASE("the rotation of a displacement system is evaluated at the node", "[analysis][coordinates]")
{
    const Model model = readModelText(
        "CylindricalFrame2;2;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "SphericalFrame2;3;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "Node;20;;0.;2.;3.;2\n"
        "Node;21;;2.;0.;0.;2\n"
        "Node;22;;0.;0.;5.;3\n"
        "Node;23;;1.;1.;0.;3\n");
    BasicPositionCache& cache = BasicPositionCache::of(model);
    CoordinateTransform& transform = cache.transform();
    const auto rotationOf = [&](base::EntityId nid, base::EntityId cd)
    {
        const auto location = model.locate(base::IdSpace::Node, nid);
        return transform.displacementRotation(cd, cache.position(location->row));
    };
    // Cylindrical, at the angle 90 degrees: R along +y, theta along -x, Z along z.
    const base::Mat3 at90 = rotationOf(20, 2);
    requireNear(at90.column(0), 0.0, 1.0, 0.0);
    requireNear(at90.column(1), -1.0, 0.0, 0.0);
    requireNear(at90.column(2), 0.0, 0.0, 1.0);
    // Cylindrical, at the angle 0: the axes themselves.
    const base::Mat3 at0 = rotationOf(21, 2);
    requireNear(at0.column(0), 1.0, 0.0, 0.0);
    requireNear(at0.column(1), 0.0, 1.0, 0.0);
    // Spherical on the polar axis: R along +z, theta along +x, phi along +y.
    const base::Mat3 onAxis = rotationOf(22, 3);
    requireNear(onAxis.column(0), 0.0, 0.0, 1.0);
    requireNear(onAxis.column(1), 1.0, 0.0, 0.0);
    requireNear(onAxis.column(2), 0.0, 1.0, 0.0);
    // Spherical at (1, 1, 0): R along (1, 1, 0)/sqrt 2, theta along -z, phi along (-1, 1, 0)/sqrt 2.
    const base::Mat3 inPlane = rotationOf(23, 3);
    const double root = std::sqrt(0.5);
    requireNear(inPlane.column(0), root, root, 0.0);
    requireNear(inPlane.column(1), 0.0, 0.0, -1.0);
    requireNear(inPlane.column(2), -root, root, 0.0);
    // The rotation of a system is orthonormal and right-handed.
    REQUIRE(inPlane.determinant() == Approx(1.0).margin(EPS));
}

TEST_CASE("the cache tells whether its rows are the grid points in ID order", "[analysis][coordinates][cache]")
{
    Model model = readModelText("Node;1;;0.;0.;0.\nNode;2;;1.;0.;0.\nNode;3;;2.;0.;0.\n");
    REQUIRE(BasicPositionCache::of(model).inIdOrder());
    const auto gridAt = [](base::EntityId id)
    {
        NodeCard card;
        card.id = id;
        card.xyz = base::Vec3{static_cast<double>(id), 0.0, 0.0};
        return card;
    };
    model.add<NodeTable>(gridAt(10)); // added after the last ID: still in order
    REQUIRE(BasicPositionCache::of(model).inIdOrder());
    const CardLocation five = model.add<NodeTable>(gridAt(5)); // added below the last ID
    REQUIRE_FALSE(BasicPositionCache::of(model).inIdOrder());
    model.setId(five, 20); // renumbered: in order again
    REQUIRE(BasicPositionCache::of(model).inIdOrder());
    model.remove(*model.locate(base::IdSpace::Node, 2)); // a dead row: the buffer is not the table
    REQUIRE_FALSE(BasicPositionCache::of(model).inIdOrder());
    model.compact();
    REQUIRE(BasicPositionCache::of(model).inIdOrder());
    requireNear(basicPosition(model, 20), 5.0, 0.0, 0.0);
}

TEST_CASE("editing one grid point recomputes that point only", "[analysis][coordinates][cache]")
{
    std::string deck = "CylindricalFrame2;2;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n";
    for (int index = 1; index <= 100; ++index)
        deck += "Node;" + std::to_string(index) + ";2;1.;" + std::to_string(index) + ".;0.\n";
    Model model = readModelText(deck);
    BasicPositionCache& cache = BasicPositionCache::of(model);
    REQUIRE(cache.statistics().lastNodesRecomputed == 100);
    BasicPositionCache::of(model);
    REQUIRE(cache.statistics().lastNodesRecomputed == 0);

    NodeTable& grids = model.table<NodeTable>();
    const auto location = model.locate(base::IdSpace::Node, 50);
    grids.setXyz(location->row, base::Vec3{2.0, 90.0, 4.0});
    BasicPositionCache::of(model);
    REQUIRE(cache.statistics().lastNodesRecomputed == 1);
    requireNear(basicPosition(model, 50), 0.0, 2.0, 4.0);
    requireNear(basicPosition(model, 51), std::cos(51.0 * std::numbers::pi / 180.0),
                std::sin(51.0 * std::numbers::pi / 180.0), 0.0);
}

TEST_CASE("editing a coordinate system recomputes the grid points that depend on it, directly or through a chain",
          "[analysis][coordinates][cache]")
{
    Model model = readModelText(
        "RectFrame2;1;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "RectFrame2;2;1;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "RectFrame2;3;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "Node;10;1;1.;0.;0.\n"
        "Node;11;2;1.;0.;0.\n"
        "Node;12;3;1.;0.;0.\n"
        "Node;13;;1.;0.;0.\n");
    BasicPositionCache& cache = BasicPositionCache::of(model);
    REQUIRE(cache.statistics().lastNodesRecomputed == 4);

    // Move the system 1: the grid points in 1 and in 2 (which is defined in 1) change; those in 3 and the basic one do
    // not.
    RectFrame2Table& systems = model.table<RectFrame2Table>();
    const auto location = model.locate(base::IdSpace::Coordinate, 1);
    RectFrame2Card card = systems.card(location->row);
    card.pointA = {5.0, 0.0, 0.0};
    card.pointB = {5.0, 0.0, 1.0};
    card.pointC = {6.0, 0.0, 0.0};
    systems.store(location->row, card);
    BasicPositionCache::of(model);
    REQUIRE(cache.statistics().lastNodesRecomputed == 2);
    requireNear(basicPosition(model, 10), 6.0, 0.0, 0.0);
    requireNear(basicPosition(model, 11), 6.0, 0.0, 0.0);
    requireNear(basicPosition(model, 12), 1.0, 0.0, 0.0);
    requireNear(basicPosition(model, 13), 1.0, 0.0, 0.0);

    // An edit of the system 3 touches the grid points in 3 only.
    const auto third = model.locate(base::IdSpace::Coordinate, 3);
    RectFrame2Card other = systems.card(third->row);
    other.pointA = {0.0, 7.0, 0.0};
    other.pointB = {0.0, 7.0, 1.0};
    other.pointC = {1.0, 7.0, 0.0};
    systems.store(third->row, other);
    BasicPositionCache::of(model);
    REQUIRE(cache.statistics().lastNodesRecomputed == 1);
    requireNear(basicPosition(model, 12), 1.0, 7.0, 0.0);
}

TEST_CASE("a grid point that defines a system moves the system and the points in it", "[analysis][coordinates][cache]")
{
    Model model = readModelText(
        "Node;41;;0.;0.;0.\n"
        "Node;42;;0.;0.;1.\n"
        "Node;43;;1.;0.;0.\n"
        "RectFrame1;5;41;42;43\n"
        "Node;44;5;1.;2.;3.\n"
        "Node;45;;9.;9.;9.\n");
    BasicPositionCache& cache = BasicPositionCache::of(model);
    requireNear(basicPosition(model, 44), 1.0, 2.0, 3.0);
    NodeTable& grids = model.table<NodeTable>();
    grids.setXyz(model.locate(base::IdSpace::Node, 41)->row, base::Vec3{10.0, 0.0, 0.0});
    BasicPositionCache::of(model);
    // The grid point 41 (edited) and 44 (in the system 5) are recomputed; 42, 43 and 45 are not.
    REQUIRE(cache.statistics().lastNodesRecomputed == 2);
    // The system moved and turned (its z axis now goes from 41 to 42): the position equals a computation from scratch.
    const base::Vec3 expected = CoordinateTransform(model).basicPositionOfNode(44);
    requireNear(basicPosition(model, 44), expected.x, expected.y, expected.z, 1.0e-12);
    REQUIRE(norm(expected - base::Vec3{1.0, 2.0, 3.0}) > 1.0);
}

TEST_CASE("added grid points are computed, deleted ones are left, and a compaction keeps the rest",
          "[analysis][coordinates][cache]")
{
    Model model = readModelText(
        "RectFrame2;1;;100.;0.;0.;100.;0.;1.;101.;0.;0.\n"
        "Node;1;1;1.;0.;0.\n"
        "Node;2;1;2.;0.;0.\n"
        "Node;3;1;3.;0.;0.\n"
        "Node;4;;4.;0.;0.\n");
    BasicPositionCache& cache = BasicPositionCache::of(model);
    NodeCard added;
    added.id = 5;
    added.positionFrameId = 1;
    added.xyz = base::Vec3{5.0, 0.0, 0.0};
    model.add<NodeTable>(added);
    BasicPositionCache::of(model);
    REQUIRE(cache.statistics().lastNodesRecomputed == 1);
    requireNear(basicPosition(model, 5), 105.0, 0.0, 0.0);

    model.remove(*model.locate(base::IdSpace::Node, 2));
    BasicPositionCache::of(model);
    REQUIRE(cache.statistics().lastNodesRecomputed == 0);
    model.compact();
    BasicPositionCache::of(model);
    REQUIRE(cache.statistics().lastNodesRecomputed == 0);
    REQUIRE(cache.positions().size() == 4);
    requireNear(basicPosition(model, 1), 101.0, 0.0, 0.0);
    requireNear(basicPosition(model, 3), 103.0, 0.0, 0.0);
    requireNear(basicPosition(model, 4), 4.0, 0.0, 0.0);
    requireNear(basicPosition(model, 5), 105.0, 0.0, 0.0);
}

TEST_CASE("a copy of a model has a cache of its own, and edits stay on their side", "[analysis][coordinates][cache]")
{
    Model model = readModelText(
        "RectFrame2;1;;100.;0.;0.;100.;0.;1.;101.;0.;0.\n"
        "Node;1;1;1.;0.;0.\n"
        "Node;2;1;2.;0.;0.\n");
    BasicPositionCache::of(model);
    Model copy = model.copy();
    BasicPositionCache& copyCache = BasicPositionCache::of(copy);
    REQUIRE(copyCache.statistics().lastNodesRecomputed == 0); // the copy starts from the state of the original

    copy.table<NodeTable>().setXyz(copy.locate(base::IdSpace::Node, 1)->row, base::Vec3{50.0, 0.0, 0.0});
    requireNear(basicPosition(copy, 1), 150.0, 0.0, 0.0);
    requireNear(basicPosition(model, 1), 101.0, 0.0, 0.0);
    model.table<NodeTable>().setXyz(model.locate(base::IdSpace::Node, 2)->row, base::Vec3{7.0, 0.0, 0.0});
    requireNear(basicPosition(model, 2), 107.0, 0.0, 0.0);
    requireNear(basicPosition(copy, 2), 102.0, 0.0, 0.0);
}

TEST_CASE("a refresh that failed starts again once the cause is gone", "[analysis][coordinates][cache]")
{
    Model model = readModelText("Node;1;;1.;0.;0.\n");
    BasicPositionCache::of(model);
    NodeTable& grids = model.table<NodeTable>();
    grids.setPositionFrameId(0, 8);
    REQUIRE_FALSE(errorMessage(model).empty());
    grids.setPositionFrameId(0, 0);
    requireNear(basicPosition(model, 1), 1.0, 0.0, 0.0);
}

TEST_CASE("a spherical system given in a cylindrical one is nested like the format reference says",
          "[analysis][coordinates]")
{
    // The system 2 has its origin at (R, theta, Z) = (2, 90, 0) of the cylindrical system 1, that is (0, 2, 0); its z
    // axis goes to (2, 90, 1) = (0, 2, 1) and its x-z plane holds (3, 90, 0) = (0, 3, 0): x along +y, z along +z, y = z
    // x x = -x.
    const Model model = readModelText(
        "CylindricalFrame2;1;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "SphericalFrame2;2;1;2.;90.;0.;2.;90.;1.;3.;90.;0.\n"
        "Node;10;2;1.;90.;0.\n"
        "Node;11;2;1.;0.;0.\n"
        "Node;12;2;1.;90.;90.\n");
    requireNear(basicPosition(model, 10), 0.0, 3.0, 0.0);
    requireNear(basicPosition(model, 11), 0.0, 2.0, 1.0);
    requireNear(basicPosition(model, 12), -1.0, 2.0, 0.0);
}

TEST_CASE("fromBasic is the inverse of toBasic in every kind of system, nested ones included",
          "[analysis][coordinates]")
{
    // A rotated rectangular system, a cylindrical one in it and a spherical one in that: a query edit that gives a
    // position in the basic system writes it back in the system of the node with this.
    const Model model = readModelText(
        "RectFrame2;1;;1.;2.;3.;1.;2.;4.;2.;2.;3.\n"
        "CylindricalFrame2;2;1;0.;0.;1.;0.;1.;2.;1.;0.;1.\n"
        "SphericalFrame2;3;2;0.5;0.5;0.5;0.5;0.5;1.5;1.5;0.5;0.5\n"
        "Node;1;;0.;0.;0.\n");
    CoordinateTransform transform(model);
    CHECK(transform.fromBasic(0, base::Vec3{1.0, 2.0, 3.0}).x == 1.0); // the basic system is the identity
    for (const base::EntityId cid : {1, 2, 3})
    {
        for (const base::Vec3 local :
             {base::Vec3{1.5, 40.0, 0.25}, base::Vec3{0.3, 130.0, -2.0}, base::Vec3{2.0, 200.0, 90.0}})
        {
            const base::Vec3 basic = transform.toBasic(cid, local);
            const base::Vec3 back = transform.fromBasic(cid, basic);
            // the same point again, whichever angles the inverse chose for it
            requireNear(transform.toBasic(cid, back), basic.x, basic.y, basic.z, 1.0e-12);
        }
    }
    // rectangular: the coordinates themselves come back
    const base::Vec3 rect = transform.fromBasic(1, transform.toBasic(1, base::Vec3{0.1, 0.2, 0.3}));
    requireNear(rect, 0.1, 0.2, 0.3, 1.0e-12);
    // a cylindrical point on the axis has the angle 0
    const base::Vec3 onAxis = transform.fromBasic(2, transform.toBasic(2, base::Vec3{0.0, 77.0, 4.0}));
    CHECK(onAxis.x == Approx(0.0).margin(1e-12));
    CHECK(onAxis.z == Approx(4.0).margin(1e-12));
    CHECK_THROWS_AS(transform.fromBasic(9, base::Vec3{}), CoordinateSystemError);
}
