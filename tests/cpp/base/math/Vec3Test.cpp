#include "oor/base/math/Mat3.hpp"
#include "oor/base/math/Vec3.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <numbers>

using namespace oor::base;
using Catch::Approx;

TEST_CASE("vector arithmetic", "[base][math]")
{
    const Vec3 a{1.0, 2.0, 3.0};
    const Vec3 b{4.0, 5.0, 6.0};
    REQUIRE(a + b == Vec3{5.0, 7.0, 9.0});
    REQUIRE(b - a == Vec3{3.0, 3.0, 3.0});
    REQUIRE(a * 2.0 == Vec3{2.0, 4.0, 6.0});
    REQUIRE(-a == Vec3{-1.0, -2.0, -3.0});
    REQUIRE(dot(a, b) == 32.0);
    REQUIRE(cross(Vec3{1, 0, 0}, Vec3{0, 1, 0}) == Vec3{0, 0, 1});
    REQUIRE(norm(Vec3{3, 4, 0}) == 5.0);
    REQUIRE(norm(normalized(Vec3{0, 0, 7})) == 1.0);
    REQUIRE(a[0] == 1.0);
    REQUIRE(a[2] == 3.0);
}

TEST_CASE("a Vec3 is three contiguous doubles", "[base][math]")
{
    static_assert(sizeof(Vec3) == 3 * sizeof(double));
    static_assert(sizeof(Mat3) == 9 * sizeof(double));
}

TEST_CASE("matrix products and rotations", "[base][math]")
{
    const Mat3 turn = rotationAboutAxis(Vec3{0, 0, 1}, std::numbers::pi / 2.0);
    const Vec3 turned = turn * Vec3{1, 0, 0};
    REQUIRE(turned.x == Approx(0.0).margin(1e-15));
    REQUIRE(turned.y == Approx(1.0));
    REQUIRE(turn.determinant() == Approx(1.0));
    const Mat3 back = turn.transposed() * turn;
    for (std::size_t row = 0; row < 3; ++row)
    {
        for (std::size_t column = 0; column < 3; ++column)
            REQUIRE(back(row, column) == Approx(row == column ? 1.0 : 0.0).margin(1e-15));
    }
    const Mat3 frame = Mat3::fromColumns({1, 0, 0}, {0, 0, 1}, {0, -1, 0});
    REQUIRE(frame.column(1) == Vec3{0, 0, 1});
    REQUIRE(frame * Vec3{0, 1, 0} == Vec3{0, 0, 1});
}
