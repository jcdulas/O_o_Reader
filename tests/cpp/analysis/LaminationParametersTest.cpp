#include "oor/analysis/mass/LaminationParameters.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

using namespace oor::analysis;
using Catch::Approx;

namespace
{

// The carbon ply of a companion package: the hand values of COMPOSITE-DESIGN-SPEC.md section 3.1.
const MaterialInvariants CARBON = materialInvariants(135000.0, 8500.0, 0.3, 4400.0);

StackLaminationParameters stackOf(const std::vector<double>& angles, bool gradient = false)
{
    const std::vector<double> thickness(angles.size(), 0.2);
    std::vector<std::uint32_t> plyOf(angles.size());
    for (std::size_t ply = 0; ply < plyOf.size(); ++ply)
        plyOf[ply] = static_cast<std::uint32_t>(ply);
    return stackLaminationParameters(angles, thickness, plyOf, angles.size(), gradient, false);
}

} // namespace

TEST_CASE("the invariants of the carbon ply", "[lamination]")
{
    CHECK(CARBON.u[0] == Approx(56960.308414).epsilon(1e-9));
    CHECK(CARBON.u[1] == Approx(63610.459269).epsilon(1e-9));
    CHECK(CARBON.u[2] == Approx(15198.592021).epsilon(1e-9));
    CHECK(CARBON.u[3] == Approx(17763.124371).epsilon(1e-9));
    CHECK(CARBON.u[4] == Approx(19598.592021).epsilon(1e-9));
    const std::array<double, 45> gamma = invariantMatrices(CARBON.u);
    // a ply at 0 degrees: G0 + G1 + G3 is the stiffness in its axes
    CHECK(gamma[0] + gamma[9] + gamma[27] == Approx(CARBON.q11).epsilon(1e-13));
    CHECK(gamma[4] + gamma[13] + gamma[31] == Approx(CARBON.q22).epsilon(1e-13));
    CHECK(gamma[8] + gamma[17] + gamma[35] == Approx(CARBON.q66).epsilon(1e-13));
}

TEST_CASE("the lamination parameters of the antisymmetric cross-ply", "[lamination]")
{
    const StackLaminationParameters found = stackOf({0.0, 0.0, 0.0, 0.0, 90.0, 90.0, 90.0, 90.0}, true);
    CHECK(found.thickness == Approx(1.6));
    CHECK(found.xi[0] == Approx(0.0).margin(1e-14)); // xi_A1
    CHECK(found.xi[2] == Approx(1.0));               // xi_A3
    CHECK(found.xi[4] == Approx(-1.0));              // xi_B1: the 0 degree plies at the bottom
    CHECK(found.xi[8] == Approx(0.0).margin(1e-14)); // xi_D1
    REQUIRE(found.dThickness.size() == 12 * 8);
    // the thickness of every ply moved together moves nothing
    double sum = 0.0;
    for (std::size_t ply = 0; ply < 8; ++ply)
        sum += 0.2 * found.dThickness[4 * 8 + ply];
    CHECK(sum == Approx(0.0).margin(1e-13));
    const LaminationAbd abd = abdFromLaminationParameters(invariantMatrices(CARBON.u), found.thickness, found.xi.data(),
                                                          found.xi.data() + 4, found.xi.data() + 8, 4, false, false);
    CHECK(abd.b[0] == Approx(-40710.69).margin(0.006));
    CHECK(abd.b[4] == Approx(40710.69).margin(0.006));
    CHECK(abd.d[0] == Approx(24630.24).margin(0.006));
}

TEST_CASE("the feasibility rows and the map of the four angles", "[lamination]")
{
    const std::vector<std::uint32_t> classes = feasibilityClasses(4);
    REQUIRE(classes.size() == 7);
    CHECK(feasibilityClasses(1).empty());
    CHECK_THROWS_AS(feasibilityClasses(MAX_FEASIBILITY_ANGLES + 1), std::invalid_argument);
    // the antisymmetric cross-ply: v = (0.5, 0, 0, 0.5), b = (-0.5, 0, 0, 0.5), w = (0.5, 0, 0, 0.5)
    const std::vector<double> v{0.5, 0.0, 0.0, 0.5};
    const std::vector<double> b{-0.5, 0.0, 0.0, 0.5};
    std::vector<double> rows(14);
    momentFeasibility(v, b, v, classes, rows.data(), nullptr, nullptr);
    CHECK(rows[0] == Approx(0.0).margin(1e-15)); // S = {0}, the lower row: on the boundary
    const std::vector<double> map = angleMomentMap(std::vector<double>{0.0, 45.0, -45.0, 90.0});
    REQUIRE(map.size() == 20);
    CHECK(map[0] == Approx(0.25)); // v0 = 1/4 + xi1 / 2 + xi3 / 4
    CHECK(map[1] == Approx(0.5));
    CHECK(map[3] == Approx(0.25));
    CHECK_THROWS_AS(angleMomentMap(std::vector<double>{0.0, 180.0}), std::invalid_argument);
    CHECK(angleIndex(std::vector<double>{0.0, 45.0, -45.0, 90.0}, 135.0) == 2);
    CHECK(angleIndex(std::vector<double>{0.0, 45.0}, 30.0) == -1);
}
