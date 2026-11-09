#include "oor/analysis/mass/SectionFormulas.hpp"
#include "oor/analysis/mass/SectionShearSolver.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

using namespace oor::analysis;
using Catch::Approx;

TEST_CASE("the shear factors of a rectangle are 5/6 in both planes, whatever its proportions", "[analysis][section]")
{
    // The shear problem of a rectangle depends on one coordinate only: its solution, and so K = 5/6, is exact; the mesh
    // (cells twice as long as thick along the longer side) leaves at most 3e-4.
    for (const double width : {0.1, 1.0, 7.0})
    {
        const std::array<SectionRectangle, 1> bar{SectionRectangle{0.0, 2.0, 0.0, width}};
        const SectionStiffness solved = SectionShearSolver::ofRectangles(bar);
        INFO(width);
        REQUIRE(solved.k1 == Approx(5.0 / 6.0).epsilon(3e-4));
        REQUIRE(solved.k2 == Approx(5.0 / 6.0).epsilon(3e-4));
    }
    // The torsional constant of a square: 0.140577 a^4 (Saint-Venant, the series of the exact solution).
    const std::array<SectionRectangle, 1> square{SectionRectangle{0.0, 1.0, 0.0, 1.0}};
    REQUIRE(SectionShearSolver::ofRectangles(square).j == Approx(0.140577).epsilon(1e-3));
}

TEST_CASE("a square box has the shear factors and torsional constant the solver records", "[analysis][section]")
{
    // A PBARL BOX 2 x 2, walls 0.1: the solver writes the derived PBAR with K1 0.4277577, K2 0.4277586, J 0.70349 (EPT
    // table).
    const double box[4] = {2.0, 2.0, 0.1, 0.1};
    const SectionProperties section = SectionFormulas::standard("Box", box, ShearFactorRule::Solver);
    REQUIRE(section.k1 == Approx(0.4277577).epsilon(1e-3));
    REQUIRE(section.k2 == Approx(0.4277586).epsilon(1e-3));
    REQUIRE(section.j == Approx(0.70349).epsilon(1e-3));
    // The section is square: both planes alike.
    REQUIRE(section.k1 == Approx(section.k2).epsilon(1e-9));
}

TEST_CASE("the beam library equations give the legacy records of two older solvers", "[analysis][section]")
{
    // A PBEAML in the first older solver: BOX 0.6, 0.2, 0.0025, 0.0025 gives K1 0.24528, K2 0.74843.
    const double narrow[4] = {0.6, 0.2, 0.0025, 0.0025};
    const SectionProperties first = SectionFormulas::standard("Box", narrow, ShearFactorRule::BeamLibrary);
    REQUIRE(first.k1 == Approx(0.24528).epsilon(1e-4));
    REQUIRE(first.k2 == Approx(0.74843).epsilon(1e-4));
    // A PBEAML in the second older solver: BOX 6, 1, 0.1, 0.1 gives K1 0.11765, K2 0.85294.
    const double wide[4] = {6.0, 1.0, 0.1, 0.1};
    const SectionProperties second = SectionFormulas::standard("Box", wide, ShearFactorRule::BeamLibrary);
    REQUIRE(second.k1 == Approx(0.11765).epsilon(1e-4));
    REQUIRE(second.k2 == Approx(0.85294).epsilon(1e-4));
}

TEST_CASE("every section of the library has finite element shear factors and a torsional constant",
          "[analysis][section]")
{
    struct Case
    {
        std::string_view type;
        std::vector<double> dimensions;
    };
    const std::vector<Case> cases{{"LSection", {6.0, 8.0, 1.0, 0.5}},
                                  {"ISection", {10.0, 5.0, 4.0, 0.5, 0.6, 0.7}},
                                  {"Channel", {4.0, 10.0, 0.5, 1.0}},
                                  {"TSection", {6.0, 9.0, 1.0, 0.5}},
                                  {"Cross", {6.0, 2.0, 10.0, 1.0}},
                                  {"HSection", {4.0, 2.0, 10.0, 1.0}},
                                  {"TSection1", {10.0, 6.0, 1.0, 2.0}},
                                  {"ISection1", {4.0, 1.0, 6.0, 10.0}},
                                  {"Channel1", {3.0, 1.0, 6.0, 10.0}},
                                  {"ZSection", {3.0, 1.0, 6.0, 10.0}},
                                  {"Channel2", {1.0, 2.0, 8.0, 10.0}},
                                  {"TSection2", {8.0, 10.0, 2.0, 1.0}},
                                  {"Box1", {10.0, 8.0, 1.0, 2.0, 3.0, 0.5}},
                                  {"Hexagon", {1.0, 6.0, 4.0}},
                                  {"Hat", {4.0, 0.5, 6.0, 2.0}},
                                  {"Hat1", {8.0, 5.0, 4.0, 0.5, 1.0}},
                                  {"DoubleBoxSection", {10.0, 6.0, 4.0, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5}}};
    for (const Case& item : cases)
    {
        INFO(item.type);
        const SectionProperties solverSection =
            SectionFormulas::standard(item.type, item.dimensions, ShearFactorRule::Solver);
        const SectionProperties legacy =
            SectionFormulas::standard(item.type, item.dimensions, ShearFactorRule::BeamLibrary);
        REQUIRE(solverSection.k1 > 0.0);
        REQUIRE(solverSection.k1 < 1.0);
        REQUIRE(solverSection.k2 > 0.0);
        REQUIRE(solverSection.k2 < 1.0);
        // The Saint-Venant constant is below the polar moment and of the size of the thin-walled formulas.
        REQUIRE(solverSection.j > 0.0);
        REQUIRE(solverSection.j < solverSection.i1 + solverSection.i2);
        REQUIRE(solverSection.j == Approx(legacy.j).epsilon(0.35));
        // The rule changes K and J only.
        REQUIRE(solverSection.area == legacy.area);
        REQUIRE(solverSection.i1 == legacy.i1);
    }
}

TEST_CASE("on the meshes of a reference the properties are smooth and their derivatives are exact",
          "[analysis][section]")
{
    // a companion package: an I whose flanges have one width (their edges meet in the reference); the derivatives of
    // the finite element properties against central differences of the properties on the reference meshes
    const std::array<double, 6> reference{10.0, 6.0, 6.0, 0.6, 0.8, 0.8};
    SectionPropertiesGradient gradient;
    const SectionProperties section =
        SectionFormulas::standard("ISection", reference, ShearFactorRule::Solver, reference, &gradient);
    REQUIRE(gradient.count == 6);
    REQUIRE(section.j == SectionFormulas::standard("ISection", reference, ShearFactorRule::Solver).j);
    for (std::size_t index = 0; index < reference.size(); ++index)
    {
        const double step = 1.0e-4 * reference[index];
        std::array<double, 6> plus = reference;
        std::array<double, 6> minus = reference;
        plus[index] += step;
        minus[index] -= step;
        const SectionProperties up =
            SectionFormulas::standard("ISection", plus, ShearFactorRule::Solver, reference, nullptr);
        const SectionProperties down =
            SectionFormulas::standard("ISection", minus, ShearFactorRule::Solver, reference, nullptr);
        INFO(index);
        REQUIRE(gradient.j[index] == Approx((up.j - down.j) / (2.0 * step)).epsilon(1e-6).margin(1e-9));
        REQUIRE(gradient.k1[index] == Approx((up.k1 - down.k1) / (2.0 * step)).epsilon(1e-6).margin(1e-9));
        REQUIRE(gradient.cw[index] == Approx((up.cw - down.cw) / (2.0 * step)).epsilon(1e-6).margin(1e-6));
        REQUIRE(gradient.area[index] == Approx((up.area - down.area) / (2.0 * step)).epsilon(1e-9));
    }
}

TEST_CASE("on the meshes of a reference the second derivatives are those of the derivatives", "[analysis][section]")
{
    // a companion package: the second derivatives of a T2 (the finite element properties and the closed forms) against
    // central differences of the first derivatives on the reference meshes, and their symmetry
    const std::array<double, 4> reference{8.0, 10.0, 0.8, 0.6};
    SectionPropertiesGradient gradient;
    SectionPropertiesHessian hessian;
    const SectionProperties section =
        SectionFormulas::standard("TSection2", reference, ShearFactorRule::Solver, reference, &gradient, &hessian);
    REQUIRE(hessian.count == 4);
    REQUIRE(section.cw == SectionFormulas::standard("TSection2", reference, ShearFactorRule::Solver).cw);
    constexpr std::size_t WIDTH = MAX_SECTION_DIRECTIONS;
    for (std::size_t index = 0; index < reference.size(); ++index)
    {
        const double step = 1.0e-4 * reference[index];
        std::array<double, 4> plus = reference;
        std::array<double, 4> minus = reference;
        plus[index] += step;
        minus[index] -= step;
        SectionPropertiesGradient up;
        SectionPropertiesGradient down;
        (void)SectionFormulas::standard("TSection2", plus, ShearFactorRule::Solver, reference, &up);
        (void)SectionFormulas::standard("TSection2", minus, ShearFactorRule::Solver, reference, &down);
        for (std::size_t other = 0; other < reference.size(); ++other)
        {
            INFO(index << " " << other);
            const std::size_t entry = other * WIDTH + index;
            REQUIRE(hessian.j[entry] == hessian.j[index * WIDTH + other]);
            REQUIRE(hessian.cw[entry] ==
                    Approx((up.cw[other] - down.cw[other]) / (2.0 * step)).epsilon(1e-5).margin(1e-6));
            REQUIRE(hessian.j[entry] ==
                    Approx((up.j[other] - down.j[other]) / (2.0 * step)).epsilon(1e-5).margin(1e-6));
            REQUIRE(hessian.drawingShearCentre[0][entry] ==
                    Approx((up.drawingShearCentre[0][other] - down.drawingShearCentre[0][other]) / (2.0 * step))
                        .epsilon(1e-5)
                        .margin(1e-7));
            REQUIRE(hessian.i1[entry] ==
                    Approx((up.i1[other] - down.i1[other]) / (2.0 * step)).epsilon(1e-5).margin(1e-6));
        }
    }
}

TEST_CASE("a web much thinner than its flanges is meshed with graded cells, an ordinary I with cells of one length",
          "[analysis][section]")
{
    // a companion package D-577: an I 30 high, flanges 20 wide and 2 thick; a web of 2 keeps the uniform plan, a web of
    // 0.105 (whose uniform coarse mesh has 15 000 cells) takes a graded one of under a thousand
    const auto pieces = [](double web)
    {
        return std::vector<SectionRectangle>{
            {-10.0, 10.0, 0.0, 2.0}, {-web / 2.0, web / 2.0, 2.0, 28.0}, {-10.0, 10.0, 28.0, 30.0}};
    };
    const auto cells = [](const SectionMeshPlan& plan)
    {
        std::size_t total = 0;
        for (const auto& edges : plan.edges)
        {
            std::size_t across = 0;
            std::size_t along = 0;
            for (std::uint32_t band = edges[0]; band < edges[1]; ++band)
                across += plan.coarseY[band];
            for (std::uint32_t band = edges[2]; band < edges[3]; ++band)
                along += plan.coarseZ[band];
            total += across * along;
        }
        return total;
    };
    const SectionMeshPlan ordinary = SectionShearSolver::planOf(pieces(2.0));
    REQUIRE(ordinary.cutsY.empty());
    REQUIRE(ordinary.cutsZ.empty());
    const SectionMeshPlan thin = SectionShearSolver::planOf(pieces(0.105));
    REQUIRE(!thin.cutsZ.empty());
    REQUIRE(cells(thin) < 1000);
    // the cuts of a graded band increase strictly inside it, one fewer than its coarse cells
    for (std::size_t band = 0; band < thin.cutsZ.size(); ++band)
    {
        const std::vector<double>& cuts = thin.cutsZ[band];
        if (cuts.empty())
            continue;
        REQUIRE(cuts.size() + 1 == thin.coarseZ[band]);
        REQUIRE(thin.fineZ[band] == 2 * thin.coarseZ[band]);
        for (std::size_t index = 0; index < cuts.size(); ++index)
        {
            REQUIRE(cuts[index] > (index == 0 ? 0.0 : cuts[index - 1]));
            REQUIRE(cuts[index] < 1.0);
        }
    }
    // the torsional constant: the flanges' 2 b t^3 / 3 less their ends' share, the web's adds little; within 3e-4 of
    // the value on meshes refined at the corners until converged
    REQUIRE(SectionShearSolver::ofRectangles(pieces(0.105)).j == Approx(99.96899).epsilon(3e-4));
}
