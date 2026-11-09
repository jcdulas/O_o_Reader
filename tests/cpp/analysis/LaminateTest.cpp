#include "oor/analysis/mass/Laminate.hpp"
#include "oor/analysis/mass/MassErrors.hpp"
#include "oor/analysis/mass/MaterialProperties.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>

using namespace oor;
using namespace oor::analysis;
using Catch::Approx;
using oor::testing::readModelText;

namespace
{

// MAT8: E1 = 100, E2 = 10, NU12 = 0.25, G12 = 5, G1Z = 3, G2Z = 2, RHO = 0.001. Plane stress, in the ply axes:
//   NU21 = NU12 E2 / E1 = 0.025, the divisor 1 - NU12 NU21 = 0.99375,
//   Q11 = E1 / d, Q22 = E2 / d, Q12 = NU12 E2 / d, Q66 = G12.
constexpr double DIVISOR = 1.0 - 0.25 * 0.025;
constexpr double Q11 = 100.0 / DIVISOR;
constexpr double Q22 = 10.0 / DIVISOR;
constexpr double Q12 = 0.25 * 10.0 / DIVISOR;
constexpr double Q66 = 5.0;
constexpr double KAPPA = 5.0 / 6.0;

const char* const MATERIALS = "OrthoMat2D;1;100.;10.;0.25;5.;3.;2.;0.001\n";

// The index of the term (row, column) in a 3 x 3 matrix row by row.
constexpr std::size_t at(std::size_t row, std::size_t column)
{
    return row * 3 + column;
}

} // namespace

TEST_CASE("a symmetric laminate has the A, B, D and G of its hand computation", "[analysis][laminate]")
{
    // The plies 0 and 90 degrees, 1 thick each, SYM: the stack is 0, 90, 90, 0 from z = -2 to 2.
    const model::Model model = readModelText(std::string(MATERIALS) + "CompProp;10;;;;;;;Symmetry;1;1.;0.;;1;1.;90.\n");
    const auto laminate = Laminate::of(model, 10);
    REQUIRE(laminate);
    REQUIRE(laminate->plies.size() == 4);
    REQUIRE(laminate->totalThickness == Approx(4.0));
    const double angles[4] = {0.0, 90.0, 90.0, 0.0};
    for (std::size_t index = 0; index < 4; ++index)
    {
        REQUIRE(laminate->plies[index].angleInDegrees == angles[index]);
        REQUIRE(laminate->plies[index].zBottom == Approx(-2.0 + static_cast<double>(index)));
        REQUIRE(laminate->plies[index].zTop == Approx(-1.0 + static_cast<double>(index)));
    }
    // A: two plies of each kind, each a unit thick.
    REQUIRE(laminate->a[at(0, 0)] == Approx(2.0 * (Q11 + Q22)));
    REQUIRE(laminate->a[at(1, 1)] == Approx(2.0 * (Q11 + Q22)));
    REQUIRE(laminate->a[at(0, 1)] == Approx(4.0 * Q12));
    REQUIRE(laminate->a[at(2, 2)] == Approx(4.0 * Q66));
    REQUIRE(laminate->a[at(0, 2)] == Approx(0.0).margin(1e-9));
    // B vanishes by symmetry.
    for (const double value : laminate->b)
        REQUIRE(value == Approx(0.0).margin(1e-9));
    // D: the outer plies (0 degrees) are the integral of z^2 from 1 to 2 and from -2 to -1, 7/3 each; the inner ones
    // 1/3.
    REQUIRE(laminate->d[at(0, 0)] == Approx((14.0 * Q11 + 2.0 * Q22) / 3.0));
    REQUIRE(laminate->d[at(1, 1)] == Approx((14.0 * Q22 + 2.0 * Q11) / 3.0));
    REQUIRE(laminate->d[at(0, 1)] == Approx(16.0 * Q12 / 3.0));
    REQUIRE(laminate->d[at(2, 2)] == Approx(16.0 * Q66 / 3.0));
    // G: the 0 degree plies have (G1Z, G2Z) = (3, 2) along (x, y), the 90 degree plies (2, 3); 5/6 of the sum.
    REQUIRE(laminate->g[0] == Approx(KAPPA * 10.0));
    REQUIRE(laminate->g[3] == Approx(KAPPA * 10.0));
    REQUIRE(laminate->g[1] == Approx(0.0).margin(1e-12));
    REQUIRE(laminate->massPerArea == Approx(0.001 * 4.0));
}

TEST_CASE("an unsymmetric laminate has a coupling matrix", "[analysis][laminate]")
{
    // 0 degrees from z = -1 to 0 and 90 degrees from 0 to 1.
    const model::Model model = readModelText(std::string(MATERIALS) + "CompProp;11;;;;;;;;1;1.;0.;;1;1.;90.\n");
    const auto laminate = Laminate::of(model, 11);
    REQUIRE(laminate);
    REQUIRE(laminate->plies[0].zBottom == Approx(-1.0));
    REQUIRE(laminate->plies[1].zTop == Approx(1.0));
    REQUIRE(laminate->a[at(0, 0)] == Approx(Q11 + Q22));
    REQUIRE(laminate->a[at(0, 1)] == Approx(2.0 * Q12));
    // B = 1/2 sum Q (z_top^2 - z_bottom^2): -1/2 for the lower ply and +1/2 for the upper one.
    REQUIRE(laminate->b[at(0, 0)] == Approx(0.5 * (Q22 - Q11)));
    REQUIRE(laminate->b[at(1, 1)] == Approx(0.5 * (Q11 - Q22)));
    REQUIRE(laminate->b[at(0, 1)] == Approx(0.0).margin(1e-9));
    REQUIRE(laminate->b[at(2, 2)] == Approx(0.0).margin(1e-9));
    // D = 1/3 sum Q (z_top^3 - z_bottom^3): one third of each ply's Q.
    REQUIRE(laminate->d[at(0, 0)] == Approx((Q11 + Q22) / 3.0));
    REQUIRE(laminate->d[at(0, 1)] == Approx(2.0 * Q12 / 3.0));
    REQUIRE(laminate->d[at(2, 2)] == Approx(2.0 * Q66 / 3.0));
}

TEST_CASE("a ply at 45 degrees has the transformed stiffness of the textbook", "[analysis][laminate]")
{
    const model::Model model = readModelText(std::string(MATERIALS) + "CompProp;12;;;;;;;;1;2.;45.\n");
    const auto laminate = Laminate::of(model, 12);
    REQUIRE(laminate);
    // Q_bar at 45 degrees; the ply is 2 thick, so A is twice it.
    REQUIRE(laminate->a[at(0, 0)] == Approx(2.0 * 0.25 * (Q11 + Q22 + 2.0 * Q12 + 4.0 * Q66)));
    REQUIRE(laminate->a[at(1, 1)] == Approx(2.0 * 0.25 * (Q11 + Q22 + 2.0 * Q12 + 4.0 * Q66)));
    REQUIRE(laminate->a[at(0, 1)] == Approx(2.0 * 0.25 * (Q11 + Q22 + 2.0 * Q12 - 4.0 * Q66)));
    REQUIRE(laminate->a[at(2, 2)] == Approx(2.0 * 0.25 * (Q11 + Q22 - 2.0 * Q12)));
    REQUIRE(laminate->a[at(0, 2)] == Approx(2.0 * 0.25 * (Q11 - Q22)));
    REQUIRE(laminate->a[at(1, 2)] == Approx(2.0 * 0.25 * (Q11 - Q22)));
    // The shear moduli (3, 2) turned by 45 degrees: the mean on the diagonal, half the difference off it.
    REQUIRE(laminate->g[0] == Approx(KAPPA * 2.0 * 2.5));
    REQUIRE(laminate->g[1] == Approx(KAPPA * 2.0 * 0.5));
}

TEST_CASE("an isotropic ply is the same at every angle, a MAT2 ply is its matrix turned", "[analysis][laminate]")
{
    const model::Model model = readModelText(
        "IsoMat;1;100.;;0.25;0.002\n"
        "AnisoMat2D;2;100.;20.;0.;50.;0.;10.\n"
        "CompProp;20;;;;;;;;1;1.;0.;;1;1.;30.\n"
        "CompProp;21;;;;;;;;2;1.;90.\n");
    const auto isotropic = Laminate::of(model, 20);
    REQUIRE(isotropic);
    // E = 100, NU = 0.25, G = E / (2 (1 + NU)) = 40: Q11 = E / (1 - NU^2) = 106.667, Q12 = 26.667, Q66 = 40 whatever
    // the angle.
    const double q11 = 100.0 / (1.0 - 0.0625);
    REQUIRE(isotropic->a[at(0, 0)] == Approx(2.0 * q11));
    REQUIRE(isotropic->a[at(0, 1)] == Approx(2.0 * 0.25 * q11));
    REQUIRE(isotropic->a[at(2, 2)] == Approx(2.0 * 40.0));
    REQUIRE(isotropic->a[at(0, 2)] == Approx(0.0).margin(1e-9));
    REQUIRE(isotropic->g[0] == Approx(KAPPA * 2.0 * 40.0));
    REQUIRE(isotropic->massPerArea == Approx(0.004));

    // MAT2 at 90 degrees swaps the two normal terms.
    const auto turned = Laminate::of(model, 21);
    REQUIRE(turned);
    REQUIRE(turned->a[at(0, 0)] == Approx(50.0));
    REQUIRE(turned->a[at(1, 1)] == Approx(100.0));
    REQUIRE(turned->a[at(0, 1)] == Approx(20.0));
    REQUIRE(turned->a[at(2, 2)] == Approx(10.0));
}

TEST_CASE("a PCOMPG laminate has its plies in the order of the card and Z0 from the first", "[analysis][laminate]")
{
    const model::Model model =
        readModelText(std::string(MATERIALS) + "CompPropGlobal;30;0.5;;;;;;;101;1;2.;0.;;;102;1;1.;90.\n");
    const auto laminate = Laminate::of(model, 30);
    REQUIRE(laminate);
    REQUIRE(laminate->plies.size() == 2);
    REQUIRE(laminate->plies[0].zBottom == Approx(0.5));
    REQUIRE(laminate->plies[0].zTop == Approx(2.5));
    REQUIRE(laminate->plies[1].zTop == Approx(3.5));
    REQUIRE(laminate->totalThickness == Approx(3.0));
    // B about the reference plane z = 0, which the laminate does not straddle: 1/2 sum Q (zt^2 - zb^2).
    REQUIRE(laminate->b[at(0, 0)] == Approx(0.5 * Q11 * (6.25 - 0.25) + 0.5 * Q22 * (12.25 - 6.25)));
}

TEST_CASE("the MEM and BEND options keep one of the matrices, SMEAR gives none", "[analysis][laminate]")
{
    const model::Model model = readModelText(std::string(MATERIALS) +
                                             "CompProp;40;;;;;;;MembraneOnly;1;1.;0.\n"
                                             "CompProp;41;;;;;;;BendingOnly;1;1.;0.\n"
                                             "CompProp;42;;;;;;;SmearedLaminate;1;1.;0.\n");
    const auto membrane = Laminate::of(model, 40);
    REQUIRE(membrane->a[at(0, 0)] == Approx(Q11));
    REQUIRE(membrane->d[at(0, 0)] == 0.0);
    const auto bending = Laminate::of(model, 41);
    REQUIRE(bending->a[at(0, 0)] == 0.0);
    REQUIRE(bending->d[at(0, 0)] == Approx(Q11 / 12.0));
    REQUIRE(std::isnan(Laminate::of(model, 42)->a[at(0, 0)]));
}

TEST_CASE("a ply whose material is missing or has no plane stress law is an error", "[analysis][laminate]")
{
    const model::Model model = readModelText(std::string(MATERIALS) + "CompProp;50;;;;;;;;7;1.;0.\n" +
                                             "AxisymOrthoMat;3;1.;1.;1.;0.3;0.3;0.3;1.\nCompProp;51;;;;;;;;3;1.;0.\n");
    REQUIRE_THROWS_AS(Laminate::of(model, 50), MassError);
    REQUIRE_THROWS_AS(Laminate::of(model, 51), MassError);
    REQUIRE_FALSE(Laminate::of(model, 99).has_value());
}

TEST_CASE("the plane stress law is a MAT8 reduced to the ply axes, a MAT9 condensed to the plane",
          "[analysis][laminate]")
{
    const model::Model model = readModelText(
        "AnisoMat3D;9;100.;10.;0.;0.;0.;0.;100.;0.;0.;0.;0.;100.;0.;0.;0.;40.;0.;0.;30.;0.;50.;0.001;0.\n");
    const auto material = MaterialProperties::planeStress(model, 9);
    REQUIRE(material);
    // The stresses are (x, y, z, xy, yz, zx): G11 100, G12 10, G13 0, G22 100, G33 100, G44 40, G55 30, G66 50, so the
    // condensation of z (uncoupled from x and y) leaves Q11 = 100, Q12 = 10, Q22 = 100, Q66 = 40.
    REQUIRE(material->stiffness[at(0, 0)] == Approx(100.0));
    REQUIRE(material->stiffness[at(0, 1)] == Approx(10.0));
    REQUIRE(material->stiffness[at(2, 2)] == Approx(40.0));
    REQUIRE(material->shear[0] == Approx(50.0)); // the zx modulus along x
    REQUIRE(material->shear[1] == Approx(30.0)); // the yz modulus along y
    REQUIRE(material->density == Approx(0.001));
    REQUIRE(MaterialProperties::density(model, 9).value() == Approx(0.001));
    REQUIRE_FALSE(MaterialProperties::density(model, 77).has_value());
}
