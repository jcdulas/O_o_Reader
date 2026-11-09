#include "oor/analysis/mass/SectionFormulas.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <numbers>
#include <vector>

using namespace oor;
using namespace oor::analysis;
using Catch::Approx;
using oor::testing::readModelText;

namespace
{

constexpr double PI = std::numbers::pi;

// A rectangle of a section in the drawing coordinates of the format reference figures: y vertical, z horizontal.
struct Rect
{
    double y0;
    double y1;
    double z0;
    double z1;
};

// What the parallel axis theorem gives for a section cut into rectangles: written out here, apart from the library, so
// that the library's integration is checked against it.
struct Composite
{
    double area = 0.0;
    double yc = 0.0;
    double zc = 0.0;
    double i1 = 0.0; // about the centroid: the integral of y^2
    double i2 = 0.0;
    double i12 = 0.0;
};

Composite composite(const std::vector<Rect>& rects)
{
    Composite result;
    double sy = 0.0;
    double sz = 0.0;
    for (const Rect& rect : rects)
    {
        const double area = (rect.y1 - rect.y0) * (rect.z1 - rect.z0);
        result.area += area;
        sy += area * (rect.y0 + rect.y1) / 2.0;
        sz += area * (rect.z0 + rect.z1) / 2.0;
    }
    result.yc = sy / result.area;
    result.zc = sz / result.area;
    for (const Rect& rect : rects)
    {
        const double w = rect.z1 - rect.z0;
        const double h = rect.y1 - rect.y0;
        const double dy = (rect.y0 + rect.y1) / 2.0 - result.yc;
        const double dz = (rect.z0 + rect.z1) / 2.0 - result.zc;
        result.i1 += w * h * h * h / 12.0 + w * h * dy * dy;
        result.i2 += h * w * w * w / 12.0 + w * h * dz * dz;
        result.i12 += w * h * dy * dz;
    }
    return result;
}

void requirePoint(const SectionProperties& section, std::size_t index, const Composite& about, double y, double z)
{
    REQUIRE(section.recovery[index][0] == Approx(y - about.yc).margin(1e-12));
    REQUIRE(section.recovery[index][1] == Approx(z - about.zc).margin(1e-12));
}

// The section from `rects` and the format reference figure: area, I1, I2, I12, and the four recovery points C, D, E, F
// given in the drawing coordinates (the library measures them from the neutral axis).
void requireSection(const char* type, const std::vector<double>& dimensions, const std::vector<Rect>& rects,
                    double handArea, const std::array<std::array<double, 2>, 4>& points)
{
    const SectionProperties section = SectionFormulas::standard(type, dimensions);
    const Composite expected = composite(rects);
    INFO(type);
    REQUIRE(handArea == Approx(expected.area));
    REQUIRE(section.area == Approx(handArea));
    REQUIRE(section.i1 == Approx(expected.i1));
    REQUIRE(section.i2 == Approx(expected.i2));
    REQUIRE(section.i12 == Approx(expected.i12).margin(1e-9));
    for (std::size_t index = 0; index < 4; ++index)
        requirePoint(section, index, expected, points[index][0], points[index][1]);
    REQUIRE(section.j > 0.0);
    REQUIRE(section.k1 > 0.0);
    REQUIRE(section.k2 > 0.0);
    REQUIRE(section.k1 <= 1.0 + 1e-12);
    REQUIRE(section.k2 <= 1.0 + 1e-12);
}

} // namespace

TEST_CASE("a rod, a tube and a bar have the closed-form properties", "[analysis][section]")
{
    const double rod[1] = {2.0};
    const SectionProperties a = SectionFormulas::standard("Rod", rod);
    REQUIRE(a.area == Approx(4.0 * PI));
    REQUIRE(a.i1 == Approx(PI * 16.0 / 4.0));
    REQUIRE(a.i2 == Approx(PI * 16.0 / 4.0));
    REQUIRE(a.i12 == Approx(0.0).margin(1e-12));
    REQUIRE(a.j == Approx(PI * 16.0 / 2.0));
    // the solver's finite element rule: 6/7 (Cowper with no Poisson effect); the beam library equations: 0.9.
    REQUIRE(a.k1 == Approx(6.0 / 7.0));
    REQUIRE(SectionFormulas::standard("Rod", rod, ShearFactorRule::BeamLibrary).k1 == Approx(0.9));
    REQUIRE(std::isnan(SectionFormulas::standard("Rod", rod, ShearFactorRule::None).k1));
    REQUIRE(a.recovery[0][0] == Approx(2.0)); // C is the top, D the right, E the bottom, F the left
    REQUIRE(a.recovery[1][1] == Approx(2.0));
    REQUIRE(a.recovery[2][0] == Approx(-2.0));
    REQUIRE(a.recovery[3][1] == Approx(-2.0));

    const double tube[2] = {3.0, 2.0};
    const SectionProperties b = SectionFormulas::standard("Tube", tube);
    REQUIRE(b.area == Approx(PI * 5.0));
    REQUIRE(b.i1 == Approx(PI * (81.0 - 16.0) / 4.0));
    REQUIRE(b.j == Approx(PI * (81.0 - 16.0) / 2.0));
    REQUIRE(b.recovery[0][0] == Approx(3.0));
    // m = 2/3: 6 (1 + m^2)^2 / (7 (1 + m^2)^2 + 20 m^2); 0.5 under the beam library equations.
    const double m2 = 4.0 / 9.0;
    REQUIRE(b.k1 == Approx(6.0 * (1.0 + m2) * (1.0 + m2) / (7.0 * (1.0 + m2) * (1.0 + m2) + 20.0 * m2)));
    REQUIRE(SectionFormulas::standard("Tube", tube, ShearFactorRule::BeamLibrary).k2 == Approx(0.5));

    // TUBE2 gives the thickness instead of the inner radius: the same tube.
    const double tube2[2] = {3.0, 1.0};
    const SectionProperties c = SectionFormulas::standard("Tube2", tube2);
    REQUIRE(c.area == Approx(b.area));
    REQUIRE(c.i1 == Approx(b.i1));
    REQUIRE(c.j == Approx(b.j));

    // BAR: DIM1 the width (z), DIM2 the height (y).
    const double bar[2] = {4.0, 6.0};
    const SectionProperties d = SectionFormulas::standard("Bar", bar);
    REQUIRE(d.area == Approx(24.0));
    REQUIRE(d.i1 == Approx(4.0 * 216.0 / 12.0)); // b h^3 / 12
    REQUIRE(d.i2 == Approx(6.0 * 64.0 / 12.0));  // h b^3 / 12
    REQUIRE(d.i12 == Approx(0.0).margin(1e-12));
    // The series for the torsion of a rectangle, 6 x 4: b t^3 (1/3 - 0.21 t/b (1 - t^4 / 12 b^4)).
    REQUIRE(d.j == Approx(6.0 * 64.0 * (1.0 / 3.0 - 0.21 * (4.0 / 6.0) * (1.0 - 256.0 / (12.0 * 1296.0)))));
    REQUIRE(d.k1 == Approx(5.0 / 6.0));
    REQUIRE(d.k2 == Approx(5.0 / 6.0));
    // C top right, D bottom right, E bottom left, F top left, as (y, z).
    REQUIRE(d.recovery[0][0] == Approx(3.0));
    REQUIRE(d.recovery[0][1] == Approx(2.0));
    REQUIRE(d.recovery[1][0] == Approx(-3.0));
    REQUIRE(d.recovery[3][1] == Approx(-2.0));
}

TEST_CASE("a box has its closed-form properties and, under the beam library equations, Bredt's torsional constant",
          "[analysis][section]")
{
    // 10 wide, 8 high, top and bottom walls 1 thick, side walls 2 thick.
    const double dimensions[4] = {10.0, 8.0, 1.0, 2.0};
    const SectionProperties box = SectionFormulas::standard("Box", dimensions, ShearFactorRule::BeamLibrary);
    REQUIRE(box.area == Approx(80.0 - 6.0 * 6.0));
    REQUIRE(box.i1 == Approx((10.0 * 512.0 - 6.0 * 216.0) / 12.0));
    REQUIRE(box.i2 == Approx((8.0 * 1000.0 - 6.0 * 216.0) / 12.0));
    REQUIRE(box.j == Approx(2.0 * 1.0 * 2.0 * 64.0 * 49.0 / (20.0 + 8.0 - 4.0 - 1.0)));
    // The net walls over the area, the corner squares counting for neither direction (the legacy records of two
    // older solvers): the sides 2 x 2 x (8 - 2) and the top and bottom 2 x 1 x (10 - 4), of 44.
    REQUIRE(box.k1 == Approx(24.0 / 44.0));
    REQUIRE(box.k2 == Approx(12.0 / 44.0));

    // The same box with the walls of BOX1: the top 1, the bottom 1, the right 2, the left 2.
    const double box1[6] = {10.0, 8.0, 1.0, 1.0, 2.0, 2.0};
    const SectionProperties general = SectionFormulas::standard("Box1", box1, ShearFactorRule::BeamLibrary);
    REQUIRE(general.area == Approx(box.area));
    REQUIRE(general.i1 == Approx(box.i1));
    REQUIRE(general.i2 == Approx(box.i2));
    // Mid-line: 8 wide, 7 high; 4 A^2 / (8/1 + 8/1 + 7/2 + 7/2) = 4 * 3136 / 23.
    REQUIRE(general.j == Approx(4.0 * 56.0 * 56.0 / 23.0));
    REQUIRE(general.k1 == Approx(box.k1));
    REQUIRE(general.k2 == Approx(box.k2));
}

TEST_CASE("a symmetric I section has the textbook moments", "[analysis][section]")
{
    // H 10, flanges 5 wide and 1 thick, web 1 thick.
    const double dimensions[6] = {10.0, 5.0, 5.0, 1.0, 1.0, 1.0};
    const SectionProperties section = SectionFormulas::standard("ISection", dimensions, ShearFactorRule::BeamLibrary);
    REQUIRE(section.area == Approx(18.0));
    REQUIRE(section.i1 == Approx((5.0 * 1000.0 - 4.0 * 512.0) / 12.0));
    REQUIRE(section.i2 == Approx((2.0 * 1.0 * 125.0 + 8.0 * 1.0) / 12.0));
    REQUIRE(section.i12 == Approx(0.0).margin(1e-12));
    REQUIRE(section.j == Approx((5.0 + 5.0 + 8.0) / 3.0));
    // The web carries the shear in y: 8 of 18; the flanges outside the web in z: 2 x 4 of 18.
    REQUIRE(section.k1 == Approx(8.0 / 18.0));
    REQUIRE(section.k2 == Approx(8.0 / 18.0));
    REQUIRE(section.recovery[0][0] == Approx(5.0));
    REQUIRE(section.recovery[0][1] == Approx(2.5));
}

TEST_CASE("every section of the library has the area, moments and recovery points of its figure", "[analysis][section]")
{
    // L: DIM1 the horizontal leg 6, DIM2 the vertical leg 8, DIM3 and DIM4 their thicknesses 1 and 0.5.
    requireSection("LSection", {6.0, 8.0, 1.0, 0.5}, {{0, 1, 0, 6}, {1, 8, 0, 0.5}}, 6.0 * 1.0 + 7.0 * 0.5,
                   {{{8, 0.5}, {0, 6}, {0, 0}, {8, 0}}});
    // I: the bottom flange 5 wide and 0.6 thick, the top 4 and 0.7, the web 0.5, 10 high.
    requireSection("ISection", {10.0, 5.0, 4.0, 0.5, 0.6, 0.7},
                   {{0, 0.6, -2.5, 2.5}, {0.6, 9.3, -0.25, 0.25}, {9.3, 10, -2, 2}}, 10.15,
                   {{{10, 2}, {0, 2.5}, {0, -2.5}, {10, -2}}});
    // CHAN: 4 across, 10 high, web 0.5, flanges 1.
    requireSection("Channel", {4.0, 10.0, 0.5, 1.0}, {{0, 10, 0, 0.5}, {9, 10, 0.5, 4}, {0, 1, 0.5, 4}},
                   10.0 * 0.5 + 2.0 * 3.5 * 1.0, {{{10, 4}, {0, 4}, {0, 0}, {10, 0}}});
    // T: the flange 6 wide and 1 thick on top, the web 0.5 wide, 9 high in all.
    requireSection("TSection", {6.0, 9.0, 1.0, 0.5}, {{8, 9, -3, 3}, {0, 8, -0.25, 0.25}}, 6.0 * 1.0 + 8.0 * 0.5,
                   {{{9, 0}, {9, 3}, {0, 0}, {9, -3}}});
    // CROSS: arms 6 in all and 1 thick, the stem 2 wide and 10 high.
    requireSection("Cross", {6.0, 2.0, 10.0, 1.0}, {{-5, 5, -1, 1}, {-0.5, 0.5, 1, 4}, {-0.5, 0.5, -4, -1}},
                   2.0 * 10.0 + 6.0 * 1.0, {{{5, 0}, {0, 4}, {-5, 0}, {0, -4}}});
    // H: a gap of 4, the flanges 1 thick each (2 in all) and 10 high, the crossbar 1 thick.
    requireSection("HSection", {4.0, 2.0, 10.0, 1.0}, {{-5, 5, -3, -2}, {-5, 5, 2, 3}, {-0.5, 0.5, -2, 2}},
                   2.0 * 10.0 + 4.0 * 1.0, {{{5, 3}, {-5, 3}, {-5, -3}, {5, -3}}});
    // T1: the flange 10 high and 1 thick, the stem 6 long and 2 thick.
    requireSection("TSection1", {10.0, 6.0, 1.0, 2.0}, {{-5, 5, 0, 1}, {-1, 1, -6, 0}}, 10.0 * 1.0 + 6.0 * 2.0,
                   {{{0, 1}, {-5, 1}, {0, -6}, {5, 1}}});
    // I1: flanges 5 wide and 2 thick (4 + 1 wide, 10 - 6 over two), the web 1 thick and 6 high.
    requireSection("ISection1", {4.0, 1.0, 6.0, 10.0}, {{0, 2, -2.5, 2.5}, {2, 8, -0.5, 0.5}, {8, 10, -2.5, 2.5}},
                   2.0 * 2.0 * 5.0 + 6.0 * 1.0, {{{10, 2.5}, {0, 2.5}, {0, -2.5}, {10, -2.5}}});
    // CHAN1: the web 1 thick and 10 high, the flanges 3 long beyond it and 2 thick.
    requireSection("Channel1", {3.0, 1.0, 6.0, 10.0}, {{0, 10, 0, 1}, {8, 10, 1, 4}, {0, 2, 1, 4}},
                   10.0 * 1.0 + 2.0 * 2.0 * 3.0, {{{10, 4}, {0, 4}, {0, 0}, {10, 0}}});
    // Z: the web 1 thick and 10 high, the flanges 3 long (left on top, right at the bottom) and 2 thick.
    requireSection("ZSection", {3.0, 1.0, 6.0, 10.0}, {{0, 10, 0, 1}, {8, 10, -3, 0}, {0, 2, 1, 4}},
                   10.0 * 1.0 + 2.0 * 2.0 * 3.0, {{{10, 1}, {0, 4}, {0, 0}, {10, -3}}});
    // CHAN2: the legs 1 thick, the base 2 thick, 8 high, 10 wide.
    requireSection("Channel2", {1.0, 2.0, 8.0, 10.0}, {{0, 2, 0, 10}, {2, 8, 0, 1}, {2, 8, 9, 10}},
                   2.0 * 1.0 * 8.0 + 8.0 * 2.0, {{{8, 10}, {0, 10}, {0, 0}, {8, 0}}});
    // T2: the flange 8 wide and 2 thick at the bottom, the web 1 wide, 10 high in all.
    requireSection("TSection2", {8.0, 10.0, 2.0, 1.0}, {{0, 2, -4, 4}, {2, 10, -0.5, 0.5}}, 1.0 * 10.0 + 2.0 * 7.0,
                   {{{10, 0.5}, {0, 4}, {0, -4}, {10, -0.5}}});
    // BOX1: 10 by 8, walls 1 (top), 2 (bottom), 3 (right), 0.5 (left).
    requireSection("Box1", {10.0, 8.0, 1.0, 2.0, 3.0, 0.5},
                   {{7, 8, 0, 10}, {0, 2, 0, 10}, {2, 7, 7, 10}, {2, 7, 0, 0.5}}, 3.0 * 10.0 + 5.0 * 3.5,
                   {{{8, 10}, {0, 10}, {0, 0}, {8, 0}}});
}

TEST_CASE("the hat sections and the double box have the area of their figure", "[analysis][section]")
{
    // HAT: 4 high, 0.5 thick, the crown 6 wide over the legs, brims 2 long outside them.
    requireSection("Hat", {4.0, 0.5, 6.0, 2.0},
                   {{3.5, 4, -3, 3}, {0.5, 3.5, -3, -2.5}, {0.5, 3.5, 2.5, 3}, {0, 0.5, -5, -2.5}, {0, 0.5, 2.5, 5}},
                   6.0 * 0.5 + 2.0 * 2.5 * 0.5 + 2.0 * 3.0 * 0.5, {{{4, 3}, {0, 5}, {0, -5}, {4, -3}}});
    // HAT1: a base plate 8 wide and 1 thick, a hat 5 high (all in), 4 wide over the legs, 0.5 thick.
    requireSection(
        "Hat1", {8.0, 5.0, 4.0, 0.5, 1.0},
        {{0, 1, -4, 4}, {1, 1.5, -4, -2}, {1, 1.5, 2, 4}, {1, 4.5, -2, -1.5}, {1, 4.5, 1.5, 2}, {4.5, 5, -2, 2}},
        8.0 * 1.0 + 0.5 * 4.0 + 2.0 * 0.5 * 4.0 + 0.5 * (4.0 - 1.0), {{{0, -4}, {0, 4}, {5, -2}, {5, 2}}});
    // DBOX: 10 wide, 6 high, the web at 4 from the left, all the walls 0.5 thick.
    requireSection("DoubleBoxSection", {10.0, 6.0, 4.0, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5},
                   {{0, 6, 0, 0.5},
                    {0, 6, 3.75, 4.25},
                    {0, 6, 9.5, 10},
                    {5.5, 6, 0.5, 3.75},
                    {0, 0.5, 0.5, 3.75},
                    {5.5, 6, 4.25, 9.5},
                    {0, 0.5, 4.25, 9.5}},
                   3.0 + 3.0 + 3.0 + 2.0 * 0.5 * 3.25 + 2.0 * 0.5 * 5.25, {{{0, 0}, {0, 10}, {6, 10}, {6, 0}}});
}

TEST_CASE("a hexagon is a rectangle and two triangles", "[analysis][section]")
{
    // 6 wide, the sloping sides inset by 1, 4 high: a 4 x 4 square and two isosceles triangles (base 4, height 1).
    const double dimensions[3] = {1.0, 6.0, 4.0};
    const SectionProperties section = SectionFormulas::standard("Hexagon", dimensions, ShearFactorRule::BeamLibrary);
    REQUIRE(section.area == Approx(4.0 * 5.0));
    // About the vertical axis: the square 4 x 4^3 / 12, each triangle b h^3 / 36 about its own axis and its area 2 at
    // 7/3.
    REQUIRE(section.i1 == Approx(4.0 * 64.0 / 12.0 + 2.0 * 64.0 / 48.0)); // the integral of y^2: base^3 height / 48
    REQUIRE(section.i2 == Approx(4.0 * 64.0 / 12.0 + 2.0 * (4.0 / 36.0 + 2.0 * (7.0 / 3.0) * (7.0 / 3.0))));
    REQUIRE(section.i12 == Approx(0.0).margin(1e-12));
    REQUIRE(section.j == Approx(std::pow(20.0, 4) / (40.0 * (section.i1 + section.i2))));
    REQUIRE(section.k1 == Approx(5.0 / 6.0));
    // C and D are the middle of the top and the bottom, E and F the points on the right and the left.
    REQUIRE(section.recovery[0][0] == Approx(2.0));
    REQUIRE(section.recovery[1][0] == Approx(-2.0));
    REQUIRE(section.recovery[2][1] == Approx(3.0));
    REQUIRE(section.recovery[3][1] == Approx(-3.0));
}

TEST_CASE("sections that are not symmetric have a product of inertia and a centroid off their drawing origin",
          "[analysis][section]")
{
    // L 6 x 8, thicknesses 1 and 0.5: the flange (A 6, centre y 0.5, z 3) and the leg (A 3.5, centre y 4.5, z 0.25).
    const double dimensions[4] = {6.0, 8.0, 1.0, 0.5};
    const SectionProperties section = SectionFormulas::standard("LSection", dimensions);
    const double yc = (6.0 * 0.5 + 3.5 * 4.5) / 9.5;
    const double zc = (6.0 * 3.0 + 3.5 * 0.25) / 9.5;
    const double i12 = 6.0 * (0.5 - yc) * (3.0 - zc) + 3.5 * (4.5 - yc) * (0.25 - zc);
    REQUIRE(section.i12 == Approx(i12));
    REQUIRE(section.i12 < 0.0);
    // The recovery point E is the outer corner: from the centroid it is (-yc, -zc).
    REQUIRE(section.recovery[2][0] == Approx(-yc));
    REQUIRE(section.recovery[2][1] == Approx(-zc));

    // Z, 3 + 1 + 3 wide, 10 high, the flanges 2 thick: centrally symmetric, so the centroid is its centre and the
    // product is the two flanges' terms: 2 x (6 x (+4) x (-2)) with the top flange to the left.
    const double z[4] = {3.0, 1.0, 6.0, 10.0};
    const SectionProperties zee = SectionFormulas::standard("ZSection", z);
    REQUIRE(zee.i12 == Approx(-96.0));
    REQUIRE(zee.recovery[0][0] == Approx(5.0)); // C: the top right corner of the web
    REQUIRE(zee.recovery[0][1] == Approx(0.5));
    REQUIRE(zee.recovery[3][1] == Approx(-3.5)); // F: the end of the top flange
}

TEST_CASE("impossible sections give NaN everywhere", "[analysis][section]")
{
    const auto allNan = [](const SectionProperties& section)
    { return std::isnan(section.area) && std::isnan(section.i1) && std::isnan(section.j); };
    const double thickWalls[4] = {10.0, 8.0, 5.0, 1.0}; // the top and bottom walls fill the box
    REQUIRE(allNan(SectionFormulas::standard("Box", thickWalls)));
    const double tube[2] = {2.0, 3.0}; // the hole is bigger than the tube
    REQUIRE(allNan(SectionFormulas::standard("Tube", tube)));
    const double wrongCount[3] = {1.0, 2.0, 3.0};
    REQUIRE(allNan(SectionFormulas::standard("Bar", wrongCount)));
    const double zero[2] = {0.0, 2.0};
    REQUIRE(allNan(SectionFormulas::standard("Bar", zero)));
    const double any[1] = {1.0};
    REQUIRE(allNan(SectionFormulas::standard("NOT_A_SECTION", any)));
}

TEST_CASE("the section of a property card is the one the card gives, or the formula of its type", "[analysis][section]")
{
    const model::Model model = readModelText(
        "IsoMat;1;2.1E+5;;0.3;7.8E-9\n"
        "BarProp;1;1;4.;5.;6.;7.;0.25;1.;2.;3.;4.;-1.;-2.;-3.;-4.;0.8;0.7;0.1\n"
        "BarShapeProp;2;1;;Tube;3.;2.;0.5\n"
        "BeamProp;3;1;10.;20.;30.;2.;40.;0.1;1.;1.;1.;1.;1.;1.;1.;1.;Yes;0.5;12.;22.;32.;3.;42.;0.2;2.;2.;2.;2.;2.;2.;"
        "2.;2.;Yes;1.;14.;24.;34.;4.;44.;0.3;3.;3.;3.;3.;3.;3.;3.;3.\n"
        "BeamShapeProp;4;1;;Rod;1.;0.1;Yes;1.;2.;0.3\n"
        "RodProp;5;1;3.;4.;5.;0.5\n"
        "TubeProp;6;1;4.;0.5;0.25\n");
    const auto pbar = SectionFormulas::of(model, 1);
    REQUIRE(pbar);
    REQUIRE(pbar->propertyType == "BarProp");
    REQUIRE(pbar->materialId == 1);
    const SectionProperties& card = pbar->stations.front();
    REQUIRE(card.area == 4.0);
    REQUIRE(card.i1 == 5.0);
    REQUIRE(card.i2 == 6.0);
    REQUIRE(card.j == 7.0);
    REQUIRE(card.nsm == 0.25);
    REQUIRE(card.i12 == 0.1);
    REQUIRE(card.k1 == 0.8);
    REQUIRE(card.recovery[0][0] == 1.0); // C1 C2 D1 D2 E1 E2 F1 F2
    REQUIRE(card.recovery[3][1] == -4.0);

    const auto pbarl = SectionFormulas::of(model, 2);
    REQUIRE(pbarl);
    REQUIRE(pbarl->sectionType == "Tube");
    REQUIRE(pbarl->stations.front().area == Approx(PI * 5.0));
    REQUIRE(pbarl->stations.front().nsm == 0.5);

    // A PBEAM keeps one set of properties per station, in order of X/XB, and the mean area is the trapezoid.
    const auto pbeam = SectionFormulas::of(model, 3);
    REQUIRE(pbeam);
    REQUIRE(pbeam->stations.size() == 3);
    REQUIRE(pbeam->stations[0].xxb == 0.0);
    REQUIRE(pbeam->stations[1].xxb == 0.5);
    REQUIRE(pbeam->stations[2].area == 14.0);
    REQUIRE(pbeam->meanArea() == Approx(0.5 * (10.0 + 12.0) / 2.0 + 0.5 * (12.0 + 14.0) / 2.0));

    const auto pbeaml = SectionFormulas::of(model, 4);
    REQUIRE(pbeaml);
    REQUIRE(pbeaml->stations.size() == 2);
    REQUIRE(pbeaml->stations[0].area == Approx(PI * 1.0 * 1.0 - 0.0).epsilon(1e-12));
    REQUIRE(pbeaml->stations[1].area == Approx(PI * 4.0));
    REQUIRE(pbeaml->stations[1].nsm == 0.3);
    REQUIRE(pbeaml->meanArea() == Approx(PI * 2.5));

    REQUIRE(SectionFormulas::of(model, 5)->stations.front().area == 3.0);
    REQUIRE(SectionFormulas::of(model, 5)->stations.front().j == 4.0);
    // The tube of a PTUBE: OD 4, wall 0.5.
    const SectionProperties ring = SectionFormulas::of(model, 6)->stations.front();
    REQUIRE(ring.area == Approx(PI / 4.0 * (16.0 - 9.0)));
    REQUIRE(ring.i1 == Approx(PI / 64.0 * (256.0 - 81.0)));
    REQUIRE_FALSE(SectionFormulas::of(model, 99).has_value());
}

TEST_CASE("the sections of PBCOMP, PBEAM3 and PBEND are the values of their cards", "[analysis][section]")
{
    const model::Model model = readModelText(
        "IsoMat;1;2.1E+5;;0.3;7.8E-9\n"
        "BeamLumpedAreaProp;5;1;2.;3.;4.;0.5;6.;0.1\n"
        "BeamProp3;6;1;0.1;0.01;0.02;0.;0.03;0.05;0.5;0.;0.;0.5;-0.5;0.;0.;-0.5\n"
        "CurvedBeamProp;7;1;1;5.;0.5\n");
    const SectionProperties bcomp = SectionFormulas::of(model, 5)->stations.front();
    REQUIRE(bcomp.area == 2.0);
    REQUIRE(bcomp.i1 == 3.0);
    REQUIRE(bcomp.i2 == 4.0);
    REQUIRE(bcomp.i12 == 0.5);
    REQUIRE(bcomp.j == 6.0);
    REQUIRE(bcomp.nsm == 0.1);

    // PBEAM3: IZ is the moment of plane 1, IY of plane 2, three stations (A, the middle node C, B) at X/XB 0, 1/2 and 1
    // which take the values of end A where they are blank, and the stress points are (y, z) pairs.
    const auto beam3 = SectionFormulas::of(model, 6);
    REQUIRE(beam3);
    REQUIRE(beam3->stations.size() == 3);
    REQUIRE(beam3->stations[1].xxb == 0.5);
    REQUIRE(beam3->stations[2].xxb == 1.0);
    const SectionProperties& end = beam3->stations.front();
    REQUIRE(end.area == 0.1);
    REQUIRE(end.i1 == 0.01);
    REQUIRE(end.i2 == 0.02);
    REQUIRE(end.j == 0.03);
    REQUIRE(end.nsm == 0.05);
    REQUIRE(end.recovery[0][0] == 0.5);
    REQUIRE(end.recovery[2][0] == -0.5);
    REQUIRE(beam3->stations[2].area == 0.1);
    REQUIRE(beam3->meanArea() == Approx(0.1));

    // PBEND in its pipe form: the mean radius 5 and the wall 0.5 make a thin tube.
    const SectionProperties pipe = SectionFormulas::of(model, 7)->stations.front();
    REQUIRE(pipe.area == Approx(2.0 * PI * 5.0 * 0.5));
    REQUIRE(pipe.i1 == Approx(PI * 125.0 * 0.5));
    REQUIRE(pipe.j == Approx(2.0 * PI * 125.0 * 0.5));
}

TEST_CASE("the integral along a beam is the trapezoid of the stations, with the centre of the distribution",
          "[analysis][section]")
{
    const double positions[2] = {0.0, 1.0};
    const double values[2] = {1.0, 3.0};
    const LinearIntegral tapered = integrateAlongBeam(positions, values);
    REQUIRE(tapered.integral == Approx(2.0));
    REQUIRE(tapered.centroid == Approx((0.5 + 2.0 / 3.0) / 2.0)); // the integral of x (1 + 2x) over 2

    const double uniform[2] = {5.0, 5.0};
    REQUIRE(integrateAlongBeam(positions, uniform).centroid == Approx(0.5));

    // One station at the start: the value holds along the whole beam.
    const double first[1] = {0.0};
    const double only[1] = {4.0};
    REQUIRE(integrateAlongBeam(first, only).integral == Approx(4.0));
}
