#include "oor/analysis/coordinates/CoordinateErrors.hpp"
#include "oor/analysis/coordinates/SystemTransforms.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

using namespace oor;
using namespace oor::analysis;
using Catch::Approx;
using oor::testing::readModelText;

namespace
{

constexpr double EPS = 1.0e-12;

void requireNear(const base::Vec3& actual, const base::Vec3& expected, double margin = EPS)
{
    REQUIRE(actual.x == Approx(expected.x).margin(margin));
    REQUIRE(actual.y == Approx(expected.y).margin(margin));
    REQUIRE(actual.z == Approx(expected.z).margin(margin));
}

// Every kind of system, nested: a rotated CORD2R, a CORD2C in it, a CORD2S in that, a CORD1R, CORD1C and CORD1S on
// grid points that sit in the curvilinear systems.
model::Model nestedSystems()
{
    return readModelText(
        "RectFrame2;1;;1.;2.;3.;1.;2.;4.;2.;2.;3.\n"
        "CylindricalFrame2;2;1;0.;0.;1.;0.;1.;2.;1.;0.;1.\n"
        "SphericalFrame2;3;2;0.5;30.;0.5;0.5;30.;1.5;1.5;30.;0.5\n"
        "Node;11;2;1.;10.;0.\n"
        "Node;12;3;2.;45.;20.\n"
        "Node;13;;0.;3.;1.\n"
        "RectFrame1;4;11;12;13\n"
        "CylindricalFrame1;5;12;13;11\n"
        "SphericalFrame1;6;13;11;12\n");
}

} // namespace

TEST_CASE("batch transforms give the hand values of each kind of system", "[analysis][coordinates][transforms]")
{
    const model::Model model = readModelText(
        "RectFrame2;1;;1.;0.;0.;1.;0.;1.;1.;1.;0.\n"
        "CylindricalFrame2;2;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "SphericalFrame2;3;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n");
    const std::vector<base::EntityId> cids = {1, 2, 3};
    const std::vector<base::Vec3> local = {{1.0, 2.0, 3.0}, {2.0, 90.0, 3.0}, {2.0, 90.0, 90.0}};
    const std::vector<base::Vec3> basic =
        SystemTransforms::points(model, cids, local, TransformDirection::ToBasic, nullptr);
    requireNear(basic[0], {1.0 - 2.0, 1.0, 3.0});
    requireNear(basic[1], {0.0, 2.0, 3.0});
    requireNear(basic[2], {0.0, 2.0, 0.0});
    const std::vector<base::Vec3> back =
        SystemTransforms::points(model, cids, basic, TransformDirection::FromBasic, nullptr);
    for (std::size_t index = 0; index < local.size(); ++index)
        requireNear(back[index], local[index]);

    // The directions at (0, 2, 0): R along +y, theta along -x; spherical R along +y, theta along -z, phi along -x.
    const std::vector<base::Vec3> at = {{0.0, 2.0, 0.0}};
    const std::vector<base::Mat3> axes = SystemTransforms::axesAt(model, std::vector<base::EntityId>{2}, at, nullptr);
    requireNear(axes[0].column(0), {0.0, 1.0, 0.0});
    requireNear(axes[0].column(1), {-1.0, 0.0, 0.0});
    requireNear(axes[0].column(2), {0.0, 0.0, 1.0});
    const std::vector<base::Mat3> spherical =
        SystemTransforms::axesAt(model, std::vector<base::EntityId>{3}, at, nullptr);
    requireNear(spherical[0].column(0), {0.0, 1.0, 0.0});
    requireNear(spherical[0].column(1), {0.0, 0.0, -1.0});
    requireNear(spherical[0].column(2), {-1.0, 0.0, 0.0});

    // A vector (1, 2, 3) along R, theta, Z at (0, 2, 0) is (-2, 1, 3) in the basic system; back again.
    const std::vector<base::Vec3> vector = {{1.0, 2.0, 3.0}};
    const std::vector<base::Vec3> turned = SystemTransforms::vectors(model, std::vector<base::EntityId>{2}, vector, at,
                                                                     TransformDirection::ToBasic, nullptr);
    requireNear(turned[0], {-2.0, 1.0, 3.0});
    requireNear(SystemTransforms::vectors(model, std::vector<base::EntityId>{2}, turned, at,
                                          TransformDirection::FromBasic, nullptr)[0],
                vector[0]);
    // A rectangular system needs no point: its directions are its axes.
    requireNear(SystemTransforms::vectors(model, std::vector<base::EntityId>{1}, vector, {},
                                          TransformDirection::ToBasic, nullptr)[0],
                {-2.0, 1.0, 3.0});
}

TEST_CASE("batch transforms go there and back to 1e-12 in nested systems of the six card types",
          "[analysis][coordinates][transforms]")
{
    const model::Model model = nestedSystems();
    base::ThreadPool pool(4);
    // Enough points to run on the threads, every system in turn.
    constexpr std::size_t COUNT = 40000;
    std::vector<base::EntityId> cids(COUNT);
    std::vector<base::Vec3> basic(COUNT);
    for (std::size_t index = 0; index < COUNT; ++index)
    {
        cids[index] = static_cast<base::EntityId>(index % 7); // 0 to 6
        const double t = static_cast<double>(index);
        basic[index] = {7.0 * std::sin(0.37 * t), 5.0 * std::cos(1.13 * t), 3.0 * std::sin(0.71 * t + 0.3)};
    }
    const std::vector<base::Vec3> local =
        SystemTransforms::points(model, cids, basic, TransformDirection::FromBasic, &pool);
    const std::vector<base::Vec3> again =
        SystemTransforms::points(model, cids, local, TransformDirection::ToBasic, &pool);
    double worst = 0.0;
    for (std::size_t index = 0; index < COUNT; ++index)
        worst = std::max(worst, base::norm(again[index] - basic[index]) / std::max(1.0, base::norm(basic[index])));
    CHECK(worst < EPS);

    // Vectors at the points: there and back, and the same as the axes applied by hand.
    std::vector<base::Vec3> vectors(COUNT);
    for (std::size_t index = 0; index < COUNT; ++index)
        vectors[index] = {1.0, -2.0 + static_cast<double>(index % 5), 0.5};
    const std::vector<base::Vec3> inBasic =
        SystemTransforms::vectors(model, cids, vectors, basic, TransformDirection::ToBasic, &pool);
    const std::vector<base::Vec3> back =
        SystemTransforms::vectors(model, cids, inBasic, basic, TransformDirection::FromBasic, &pool);
    const std::vector<base::Mat3> axes = SystemTransforms::axesAt(model, cids, basic, &pool);
    worst = 0.0;
    double byHand = 0.0;
    for (std::size_t index = 0; index < COUNT; ++index)
    {
        worst = std::max(worst, base::norm(back[index] - vectors[index]));
        byHand = std::max(byHand, base::norm(axes[index] * vectors[index] - inBasic[index]));
    }
    CHECK(worst < EPS);
    CHECK(byHand == 0.0);
}

TEST_CASE("the spherical polar angle comes back near the poles", "[analysis][coordinates][transforms]")
{
    const model::Model model = readModelText("SphericalFrame2;3;;1.;1.;1.;1.;1.;2.;2.;1.;1.\n");
    const std::vector<base::Vec3> local = {{10.0, 1.0e-7, 30.0}, {10.0, 180.0 - 1.0e-7, -60.0}, {3.0, 1.0e-3, 0.0}};
    const std::vector<base::EntityId> cid = {3};
    const std::vector<base::Vec3> basic =
        SystemTransforms::points(model, cid, local, TransformDirection::ToBasic, nullptr);
    const std::vector<base::Vec3> back =
        SystemTransforms::points(model, cid, basic, TransformDirection::FromBasic, nullptr);
    for (std::size_t index = 0; index < local.size(); ++index)
        CHECK(back[index].y == Approx(local[index].y).margin(1.0e-12)); // acos(z / R) gave 0 for the first
}

TEST_CASE("batch transforms refuse what they cannot do", "[analysis][coordinates][transforms]")
{
    const model::Model model = readModelText("CylindricalFrame2;2;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n");
    const std::vector<base::Vec3> two = {{1.0, 0.0, 0.0}, {2.0, 0.0, 0.0}};
    CHECK_THROWS_AS(
        SystemTransforms::points(model, std::vector<base::EntityId>{9}, two, TransformDirection::ToBasic, nullptr),
        CoordinateSystemError);
    CHECK_THROWS_AS(SystemTransforms::points(model, std::vector<base::EntityId>{0, 2, 2}, two,
                                             TransformDirection::ToBasic, nullptr),
                    base::UsageError);
    CHECK_THROWS_AS(
        SystemTransforms::vectors(model, std::vector<base::EntityId>{2}, two, {}, TransformDirection::ToBasic, nullptr),
        base::UsageError);
    const std::vector<base::Vec3> three = {{1.0, 0.0, 0.0}, {2.0, 0.0, 0.0}, {3.0, 0.0, 0.0}};
    CHECK_THROWS_AS(SystemTransforms::vectors(model, std::vector<base::EntityId>{2}, two, three,
                                              TransformDirection::ToBasic, nullptr),
                    base::UsageError);
    // The basic system needs no card, and a model without systems has it.
    const model::Model empty = readModelText("Node;1;;0.;0.;0.\n");
    CHECK(SystemTransforms::points(empty, std::vector<base::EntityId>{0}, two, TransformDirection::ToBasic, nullptr)[1]
              .x == 2.0);
}
