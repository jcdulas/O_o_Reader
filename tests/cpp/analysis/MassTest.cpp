#include "oor/analysis/mass/MassCalculator.hpp"
#include "oor/analysis/mass/MassErrors.hpp"
#include "oor/analysis/mass/NonStructuralMass.hpp"
#include "support/ModelRecord.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <numbers>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace oor;
using namespace oor::analysis;
using Catch::Approx;
using oor::testing::readModelText;

namespace
{

// The record of a node: id, x, y, z in the basic system.
std::string grid(int id, double x, double y, double z)
{
    return oor::testing::modelRecord("Node", id, "", x, y, z);
}

// The mass of the element with this ID, with a set of non-structural mass when `set` is not 0.
ElementMasses massOf(const model::Model& model, base::EntityId eid, base::EntityId set = 0, double factor = 1.0)
{
    const auto location = model.locate(base::IdSpace::Element, eid);
    REQUIRE(location.has_value());
    MassOptions options;
    options.weightFactor = factor;
    std::optional<NonStructuralMass> nonStructural;
    if (set != 0)
    {
        nonStructural = NonStructuralMass::resolve(model, set);
        options.nonStructural = &*nonStructural;
    }
    return MassCalculator::of(model, *location, options);
}

double mass(const model::Model& model, base::EntityId eid, base::EntityId set = 0)
{
    return massOf(model, eid, set).mass.front();
}

MassTotals totalOf(const model::Model& model, base::EntityId set = 0, double factor = 1.0)
{
    MassOptions options;
    options.weightFactor = factor;
    std::optional<NonStructuralMass> nonStructural;
    if (set != 0)
    {
        nonStructural = NonStructuralMass::resolve(model, set);
        options.nonStructural = &*nonStructural;
    }
    return MassCalculator::total(model, MassCalculator::allElements(model), options);
}

void requireVec(const base::Vec3& actual, double x, double y, double z, double margin = 1e-12)
{
    REQUIRE(actual.x == Approx(x).margin(margin));
    REQUIRE(actual.y == Approx(y).margin(margin));
    REQUIRE(actual.z == Approx(z).margin(margin));
}

constexpr double RHO = 0.001;
const char* const MATERIAL = "IsoMat;1;2.1E+5;;0.3;0.001\n";

} // namespace

TEST_CASE("a shell weighs its area times density times thickness plus the non-structural mass", "[analysis][mass]")
{
    // A 10 x 5 plate 2 thick with an NSM of 0.01 per unit area, and a triangle of a second property.
    const model::Model model = readModelText(std::string(MATERIAL) + "ShellProp;1;1;2.;;;;;0.01\nShellProp;2;1;1.\n" +
                                             grid(1, 0, 0, 0) + grid(2, 10, 0, 0) + grid(3, 10, 5, 0) +
                                             grid(4, 0, 5, 0) + "ShellQuad4;10;1;1;2;3;4\nShellTria3;11;2;1;2;3\n");
    const ElementMasses quad = massOf(model, 10);
    REQUIRE(quad.mass.front() == Approx(50.0 * (RHO * 2.0 + 0.01)));
    REQUIRE(quad.structural.front() == Approx(50.0 * RHO * 2.0));
    REQUIRE(quad.nonStructural.front() == Approx(50.0 * 0.01));
    requireVec(quad.center.front(), 5.0, 2.5, 0.0);
    REQUIRE(quad.property.front() == 1);
    REQUIRE(quad.material.front() == 1);
    REQUIRE(mass(model, 11) == Approx(25.0 * RHO));
    requireVec(massOf(model, 11).center.front(), 20.0 / 3.0, 5.0 / 3.0, 0.0);
}

TEST_CASE("the thickness of a shell is the mean of its corner thicknesses", "[analysis][mass]")
{
    const model::Model model =
        readModelText(std::string(MATERIAL) + "ShellProp;1;1;2.\n" + grid(1, 0, 0, 0) + grid(2, 1, 0, 0) +
                      grid(3, 1, 1, 0) + grid(4, 0, 1, 0) + "ShellQuad4;10;1;1;2;3;4;;;;0;1.;2.;3.;4.\n");
    REQUIRE(mass(model, 10) == Approx(RHO * 2.5));
}

TEST_CASE("a PSHELL1 weighs the mean of the thicknesses it states, or the thickness of the element when T1 is blank",
          "[analysis][mass]")
{
    // SOL 700 shells: T1 to T4 at the grid points (1, 2, 3, 4: mean 2.5), a property with none and an element that
    // has 3.
    const model::Model model =
        readModelText(std::string(MATERIAL) + "ShellPropNodal;1;1;;;;;;;1.;2.;3.;4.\nShellPropNodal;2;1\n" +
                      grid(1, 0, 0, 0) + grid(2, 1, 0, 0) + grid(3, 1, 1, 0) + grid(4, 0, 1, 0) +
                      "ShellQuad4;10;1;1;2;3;4\nShellQuad4;11;2;1;2;3;4;;;;0;3.;3.;3.;3.\n");
    REQUIRE(mass(model, 10) == Approx(RHO * 2.5));
    REQUIRE(mass(model, 11) == Approx(RHO * 3.0));
}

TEST_CASE("a laminate weighs the sum of density times thickness of its plies", "[analysis][mass]")
{
    // Two materials: 0.001 and 0.004; plies 1 and 2 thick; NSM 0.5; the area is 6.
    const model::Model model = readModelText(
        std::string(MATERIAL) + "IsoMat;2;1.E+5;;0.3;0.004\nCompProp;5;;0.5;;;;;;1;1.;0.;;2;2.;90.\n" +
        grid(1, 0, 0, 0) + grid(2, 3, 0, 0) + grid(3, 3, 2, 0) + grid(4, 0, 2, 0) + "ShellQuad4;10;5;1;2;3;4\n");
    const ElementMasses masses = massOf(model, 10);
    REQUIRE(masses.mass.front() == Approx(6.0 * (RHO * 1.0 + 0.004 * 2.0 + 0.5)));
    REQUIRE(masses.structural.front() == Approx(6.0 * (RHO + 0.008)));
    REQUIRE(masses.material.front() == 0); // several materials: the grouping splits it
    REQUIRE(masses.property.front() == 5);

    const MassGroups groups = MassCalculator::group(model, MassCalculator::allElements(model), MassGrouping::Material);
    REQUIRE(groups.byId.at(1).mass() == Approx(6.0 * RHO));
    REQUIRE(groups.byId.at(2).mass() == Approx(6.0 * 0.008));
    REQUIRE(groups.byId.at(0).mass() == Approx(6.0 * 0.5)); // the NSM belongs to no material
}

TEST_CASE("a symmetric laminate counts the mirrored plies", "[analysis][mass]")
{
    const model::Model model =
        readModelText(std::string(MATERIAL) + "CompProp;5;;;;;;;Symmetry;1;1.;0.;;1;2.;90.\n" + grid(1, 0, 0, 0) +
                      grid(2, 1, 0, 0) + grid(3, 1, 1, 0) + grid(4, 0, 1, 0) + "ShellQuad4;10;5;1;2;3;4\n");
    REQUIRE(mass(model, 10) == Approx(RHO * 2.0 * (1.0 + 2.0)));
}

TEST_CASE("a shear panel and a plane strain element weigh like their properties", "[analysis][mass]")
{
    const model::Model model =
        readModelText(std::string(MATERIAL) + "ShearPanelProp;3;1;0.5;0.2\nPlanePropNonlinear;4;1\n" +
                      grid(1, 0, 0, 0) + grid(2, 2, 0, 0) + grid(3, 2, 3, 0) + grid(4, 0, 3, 0) +
                      "ShearPanel4;10;3;1;2;3;4\nPlaneStrainTria3;11;4;1;2;3\n");
    REQUIRE(mass(model, 10) == Approx(6.0 * (RHO * 0.5 + 0.2)));
    // A plane strain element is one unit deep: the density times the area.
    REQUIRE(mass(model, 11) == Approx(RHO * 3.0));
}

TEST_CASE("solids weigh their density times their volume", "[analysis][mass]")
{
    const model::Model model = readModelText(std::string(MATERIAL) + "SolidProp;1;1\n" + grid(1, 0, 0, 0) +
                                             grid(2, 2, 0, 0) + grid(3, 2, 3, 0) + grid(4, 0, 3, 0) + grid(5, 0, 0, 4) +
                                             grid(6, 2, 0, 4) + grid(7, 2, 3, 4) + grid(8, 0, 3, 4) +
                                             "SolidHexa;10;1;1;2;3;4;5;6;7;8\nSolidTetra;11;1;1;2;4;5\nSolidPenta;12;1;1;"
                                             "2;4;5;6;8\nSolidPyramid;13;1;1;2;3;4;5\n");
    REQUIRE(mass(model, 10) == Approx(RHO * 24.0));
    REQUIRE(mass(model, 11) == Approx(RHO * (1.0 / 6.0) * 2.0 * 3.0 * 4.0));
    REQUIRE(mass(model, 12) == Approx(RHO * 0.5 * 2.0 * 3.0 * 4.0));
    REQUIRE(mass(model, 13) == Approx(RHO * 2.0 * 3.0 * 4.0 / 3.0)); // a pyramid: a third of the box
    requireVec(massOf(model, 10).center.front(), 1.0, 1.5, 2.0);
}

TEST_CASE("a fluid solid is not structural mass", "[analysis][mass]")
{
    const model::Model model =
        readModelText("FluidMat;2;1.E+5;0.001;1.4\nSolidProp;1;2;;;;;PressureFluid\n" + grid(1, 0, 0, 0) +
                      grid(2, 1, 0, 0) + grid(3, 0, 1, 0) + grid(4, 0, 0, 1) + "SolidTetra;10;1;1;2;3;4\n");
    REQUIRE(mass(model, 10) == 0.0);
}

TEST_CASE("rods, tubes and bars weigh density times area plus NSM, times length", "[analysis][mass]")
{
    const model::Model model = readModelText(std::string(MATERIAL) +
                                             "RodProp;1;1;2.;;;0.1\n"
                                             "TubeProp;2;1;4.;0.5;0.25\n"
                                             "BarProp;3;1;3.;1.;1.;1.;0.2\n"
                                             "BarShapeProp;4;1;;Rod;1.;0.3\n" +
                                             grid(1, 0, 0, 0) + grid(2, 3, 0, 0) + grid(3, 0, 4, 0) +
                                             "Rod2;10;1;1;2\nTube2;11;2;1;3\nRod2InlineSection;12;1;2;1;0.5;;;0."
                                             "25\nBar2;13;3;1;2;0.;;0.;1.\nBar2;14;4;1;3;0.;;0.;1.\n");
    REQUIRE(mass(model, 10) == Approx((RHO * 2.0 + 0.1) * 3.0));
    // The tube: OD 4, wall 0.5, so the ring area pi/4 (16 - 9); length 4.
    REQUIRE(mass(model, 11) == Approx((RHO * std::numbers::pi / 4.0 * 7.0 + 0.25) * 4.0));
    REQUIRE(mass(model, 12) == Approx((RHO * 0.5 + 0.25) * 3.0));
    REQUIRE(mass(model, 13) == Approx((RHO * 3.0 + 0.2) * 3.0));
    // A PBARL ROD of radius 1: the area from the section formula.
    REQUIRE(mass(model, 14) == Approx((RHO * std::numbers::pi + 0.3) * 4.0));
    requireVec(massOf(model, 11).center.front(), 0.0, 2.0, 0.0);
}

TEST_CASE("a tapered beam weighs the trapezoid of its stations and sits at their centre", "[analysis][mass]")
{
    // The area goes from 2 to 4 over a beam 6 long, and the NSM from 0 to 0.5.
    const model::Model model =
        readModelText(std::string(MATERIAL) + "BeamProp;1;1;2.;1.;1.;0.;1.;0.;No;1.;4.;1.;1.;0.;1.;0.5\n" +
                      grid(1, 0, 0, 0) + grid(2, 6, 0, 0) + "Beam2;10;1;1;2;0.;;0.;1.\n");
    const ElementMasses masses = massOf(model, 10);
    const double structural = RHO * 3.0 * 6.0; // the mean area is 3
    const double nonStructural = 0.25 * 6.0;   // the mean NSM is 0.25
    REQUIRE(masses.structural.front() == Approx(structural));
    REQUIRE(masses.nonStructural.front() == Approx(nonStructural));
    // A(x) = 2 + 2x: the integral of x A over the integral of A is 5/9; NSM(x) = x / 2: 2/3.
    const double structuralX = 6.0 * 5.0 / 9.0;
    const double nonStructuralX = 6.0 * 2.0 / 3.0;
    requireVec(masses.center.front(),
               (structural * structuralX + nonStructural * nonStructuralX) / (structural + nonStructural), 0.0, 0.0);
    // The two masses at their own places have an inertia of their own about their common centre.
    REQUIRE(masses.inertia.size() == 1);
}

TEST_CASE("the non-structural mass of a beam sits at the offset the property gives", "[analysis][mass]")
{
    // M1A = M1B = 1 puts the NSM 1 away along the element y axis, which for the orientation vector z is the basic z.
    // The beam is 4 long along x with a uniform area 1 and an NSM of 0.5 per unit length.
    const model::Model model = readModelText(
        std::string(MATERIAL) + "BeamProp;1;1;1.;1.;1.;0.;1.;0.5;No;1.;1.;1.;1.;0.;1.;0.5;;1.;1.;;;;;;;1.;0.;1.;0.\n" +
        grid(1, 0, 0, 0) + grid(2, 4, 0, 0) + "Beam2;10;1;1;2;0.;;0.;1.\n");
    const ElementMasses masses = massOf(model, 10);
    const double structural = RHO * 4.0;
    const double nonStructural = 0.5 * 4.0;
    requireVec(masses.center.front(), 2.0, 0.0, nonStructural / (structural + nonStructural), 1e-9);
    // The total inertia about the origin is that of the two point masses: Iyy = m (x^2 + z^2) for each.
    MassSums sums;
    for (const RowSet& set : MassCalculator::allElements(model))
    {
        const ElementMasses all = MassCalculator::compute(model, set.type, set.rows);
        sums.add(all.mass[0], all.center[0], &all.inertia.front().tensor);
    }
    const MassProperties properties = sums.properties(base::Vec3{});
    const double iyy = structural * 4.0 + nonStructural * (4.0 + 1.0);
    REQUIRE(properties.inertiaAtReference(1, 1) == Approx(iyy));
}

TEST_CASE("the mass of an offset beam is between its grid points under MDLPRM OFFDEF ELMOFF, on its offsets else",
          "[analysis][mass]")
{
    // The tapered beam of the test above, from (0, 0, 0) to (6, 0, 0), offset at end B by (0, 0, 1): the length between
    // the offset ends is the square root of 37, and its mass is that length's whatever OFFDEF. A point at a fraction of
    // that length (the structural mass at 5/9, the NSM at 2/3) is at the same fraction between the grid points under
    // ELMOFF (no MDLPRM, the solver's default), between the offset ends under ELMZ and LROFF.
    const std::string beam = std::string(MATERIAL) + "BeamProp;1;1;2.;1.;1.;0.;1.;0.;No;1.;4.;1.;1.;0.;1.;0.5\n" +
                             grid(1, 0, 0, 0) + grid(2, 6, 0, 0) + "Beam2;10;1;1;2;0.;;1.;0.;;;;;;;;;1.\n";
    const double length = std::sqrt(37.0);
    const double structural = RHO * 3.0 * length;
    const double nonStructural = 0.25 * length;
    const double total = structural + nonStructural;
    const double x = (structural * 6.0 * 5.0 / 9.0 + nonStructural * 6.0 * 2.0 / 3.0) / total;
    const double z = (structural * 5.0 / 9.0 + nonStructural * 2.0 / 3.0) / total;
    for (const char* offdef :
         {"", "ModelParams;OffsetDefinition;ElemOffsetsOnly\n", "ModelParams;OffsetDefinition;NoMassOffsets\n",
          "ModelParams;OffsetDefinition;ElemOffsetZ\n", "ModelParams;OffsetDefinition;LoadRecoveryOffsets\n",
          "ModelParams;OffsetDefinition;NoThermalOffsets\n"})
    {
        const model::Model model = readModelText(beam + offdef);
        const bool onOffsets = MassCalculator::offsetMassOf(model) == OffsetMass::Offsets;
        INFO(offdef);
        const ElementMasses masses = massOf(model, 10);
        REQUIRE(masses.structural.front() == Approx(structural).epsilon(1e-14));
        REQUIRE(masses.nonStructural.front() == Approx(nonStructural).epsilon(1e-14));
        requireVec(masses.center.front(), x, 0.0, onOffsets ? z : 0.0, 1e-12);
    }
    // MassOptions::offsets overrides the deck's OFFDEF
    const model::Model model = readModelText(beam);
    MassOptions options;
    options.offsets = OffsetMass::Offsets;
    const auto location = model.locate(base::IdSpace::Element, 10);
    REQUIRE(location.has_value());
    requireVec(MassCalculator::of(model, *location, options).center.front(), x, 0.0, z, 1e-12);
}

TEST_CASE("the mass of a CBAR and of a CBEAM3 with offsets follows MDLPRM OFFDEF", "[analysis][mass]")
{
    // A CBAR from (0, 0, 0) to (2, 0, 0) offset by (0, 0.03, 0.05) at both ends, and a CBEAM3 from (0, 0, 0) to
    // (4, 0, 0) through (2, 0, 0) offset by (0, 0, 0.5) at its three points.
    const std::string deck = std::string(MATERIAL) + "BarProp;1;1;2.;1.;1.;1.\n" +
                             "BeamProp3;6;1;0.1;0.01;0.02;0.;0.03;0.05;0.5;0.;0.;0.5;-0.5;0.;0.;-0.5\n" +
                             grid(1, 0, 0, 0) + grid(2, 2, 0, 0) + grid(3, 4, 0, 0) +
                             "Bar2;10;1;1;2;0.;;1.;0.;;;;0.;0.03;0.05;0.;0.03;0.05\n"
                             "Beam3;11;6;1;3;2;;0.;1.;0.;0.;0.;0.5;0.;0.;0.5;0.;0.;0.5\n";
    const model::Model atGrids = readModelText(deck);
    REQUIRE(mass(atGrids, 10) == Approx(RHO * 2.0 * 2.0));
    requireVec(massOf(atGrids, 10).center.front(), 1.0, 0.0, 0.0);
    requireVec(massOf(atGrids, 11).center.front(), 2.0, 0.0, 0.0);
    const model::Model onOffsets = readModelText(deck + "ModelParams;OffsetDefinition;ElemOffsetZ\n");
    REQUIRE(mass(onOffsets, 10) == Approx(RHO * 2.0 * 2.0));
    requireVec(massOf(onOffsets, 10).center.front(), 1.0, 0.03, 0.05);
    requireVec(massOf(onOffsets, 11).center.front(), 2.0, 0.0, 0.5);
    REQUIRE(mass(onOffsets, 11) == Approx(mass(atGrids, 11)).epsilon(1e-14));
}

TEST_CASE("the mass of a shell with a ZOFFS is on its grid point plane under ELMOFF, on its reference plane else",
          "[analysis][mass]")
{
    // ELMOFF: the mass properties are not modified by the offset; LROFF and its variants keep the mass effects (ELMZ).
    const std::string plate = std::string(MATERIAL) + "ShellProp;1;1;0.5\n" + grid(1, 0, 0, 0) + grid(2, 1, 0, 0) +
                              grid(3, 1, 1, 0) + grid(4, 0, 1, 0) + "ShellQuad4;1;1;1;2;3;4;;;2.\n";
    for (const auto& [offdef, height] :
         std::vector<std::pair<std::string, double>>{{"", 0.0},
                                                     {"ElemOffsetsOnly", 0.0},
                                                     {"NoDampingOffsets", 0.0},
                                                     {"NoMassOffsets", 0.0},
                                                     {"LoadRecoveryOffsets", 2.0},
                                                     {"NoOffsetDifference", 2.0},
                                                     {"NoTemperatureGradientOffsets", 2.0},
                                                     {"ElemOffsetZ", 2.0}})
    {
        const model::Model model =
            readModelText(plate + (offdef.empty() ? "" : "ModelParams;OffsetDefinition;" + offdef + "\n"));
        INFO(offdef);
        REQUIRE(mass(model, 1) == Approx(RHO * 0.5));
        requireVec(massOf(model, 1).center.front(), 0.5, 0.5, height);
    }
}

TEST_CASE(
    "MDLPRM OFFDEF is read from the last card that gives it, and a value the format reference does not list is an "
    "error",
    "[analysis][mass]")
{
    REQUIRE(MassCalculator::offsetMassOf(readModelText(grid(1, 0, 0, 0))) == OffsetMass::GridPoints);
    REQUIRE(MassCalculator::offsetMassOf(readModelText(
                "ModelParams;OffsetDefinition;ElemOffsetZ\nModelParams;HDF5;1\n")) == OffsetMass::Offsets);
    REQUIRE(MassCalculator::offsetMassOf(readModelText(
                "ModelParams;OffsetDefinition;LoadRecoveryOffsets\nModelParams;OffsetDefinition;ElemOffsetsOnly\n")) ==
            OffsetMass::GridPoints);
    REQUIRE_THROWS_AS(MassCalculator::offsetMassOf(readModelText("ModelParams;OffsetDefinition;OTHER\n")), MassError);
    REQUIRE_THROWS_AS(MassCalculator::offsetMassOf(readModelText("ModelParams;OffsetDefinition;1\n")), MassError);
    // the error reaches the mass of the elements
    const model::Model model =
        readModelText(std::string(MATERIAL) + "RodProp;1;1;2.\n" + grid(1, 0, 0, 0) + grid(2, 1, 0, 0) +
                      "Rod2;10;1;1;2\nModelParams;OffsetDefinition;OTHER\n");
    REQUIRE_THROWS_AS(mass(model, 10), MassError);
}

TEST_CASE("a CONM2 is a mass at its offset with its inertia about its own centre", "[analysis][mass]")
{
    // 2 at the grid point (1, 2, 3) shifted by (0.5, 0, 0); I11 = 1, I22 = 2, I33 = 3, I21 = 0.1.
    const model::Model model = readModelText(grid(1, 1, 2, 3) + "PointMassInertia;10;1;;2.;0.5;0.;0.;1.;0.1;2.;0.;0.;3.\n");
    const ElementMasses masses = massOf(model, 10);
    REQUIRE(masses.mass.front() == 2.0);
    requireVec(masses.center.front(), 1.5, 2.0, 3.0);
    REQUIRE(masses.inertia.size() == 1);
    const base::Mat3& tensor = masses.inertia.front().tensor;
    REQUIRE(tensor(0, 0) == 1.0);
    REQUIRE(tensor(0, 1) == -0.1); // the card's I21 is the integral of the product: the tensor has its negative
    REQUIRE(tensor(1, 1) == 2.0);
    REQUIRE(tensor(2, 2) == 3.0);

    // About the origin: the parallel axis theorem adds m (|c|^2 - c c^T).
    const MassTotals total = totalOf(model);
    const MassProperties about = total.sums.properties(base::Vec3{});
    REQUIRE(about.mass == 2.0);
    requireVec(about.centerOfGravity, 1.5, 2.0, 3.0);
    REQUIRE(about.inertiaAtCenter(0, 0) == Approx(1.0).margin(1e-12));
    REQUIRE(about.inertiaAtCenter(0, 1) == Approx(-0.1).margin(1e-12));
    REQUIRE(about.inertiaAtReference(0, 0) == Approx(1.0 + 2.0 * (4.0 + 9.0)));
    REQUIRE(about.inertiaAtReference(0, 1) == Approx(-0.1 - 2.0 * 1.5 * 2.0));
    REQUIRE(about.inertiaAtReference(2, 2) == Approx(3.0 + 2.0 * (2.25 + 4.0)));
    // The six numbers with the products positive.
    const std::array<double, 6> products = inertiaProducts(about.inertiaAtCenter);
    REQUIRE(products[3] == Approx(0.1).margin(1e-12));
}

TEST_CASE("the coordinate system of a CONM2 turns its offset and its inertia", "[analysis][mass]")
{
    // System 5: x along the basic y axis and y along the negative basic x axis. The offset (1, 0, 0) is basic (0, 1, 0)
    // and the inertia I11 = 1 (about the local x) is the basic Iyy.
    const model::Model model = readModelText("RectFrame2;5;;0.;0.;0.;0.;0.;1.;0.;1.;0.\n" + grid(1, 0, 0, 0) +
                                             "PointMassInertia;10;1;5;2.;1.;0.;0.;1.;0.;2.;0.;0.;3.\n"
                                             "PointMassInertia;11;1;-1;4.;7.;8.;9.\n");
    const ElementMasses turned = massOf(model, 10);
    requireVec(turned.center.front(), 0.0, 1.0, 0.0);
    const base::Mat3& tensor = turned.inertia.front().tensor;
    REQUIRE(tensor(1, 1) == Approx(1.0)); // the local x is the basic y
    REQUIRE(tensor(0, 0) == Approx(2.0)); // the local y is the negative basic x
    REQUIRE(tensor(2, 2) == Approx(3.0));
    // CID -1: X1, X2, X3 are the position, not an offset.
    requireVec(massOf(model, 11).center.front(), 7.0, 8.0, 9.0);
}

TEST_CASE("a CONM1 gives its mass, its centre and the inertia that the offset does not explain", "[analysis][mass]")
{
    // A mass 3 at (0, 0, 0.5) from the grid point with an inertia diag(0.1, 0.2, 0.3) of its own: the matrix has the
    // coupling m [[0, z, -y], [-z, 0, x], [y, -x, 0]] and the inertia about the grid point diag(0.85, 0.95, 0.3).
    const model::Model model = readModelText(
        grid(1, 1, 1, 1) + "PointMassMatrix;10;1;;3.;0.;3.;0.;0.;3.;0.;-1.5;0.;0.85;1.5;0.;0.;0.;0.95;0.;0.;0.;0.;0.;0.3\n");
    const ElementMasses masses = massOf(model, 10);
    REQUIRE(masses.mass.front() == Approx(3.0));
    requireVec(masses.center.front(), 1.0, 1.0, 1.5);
    const base::Mat3& tensor = masses.inertia.front().tensor;
    REQUIRE(tensor(0, 0) == Approx(0.1));
    REQUIRE(tensor(1, 1) == Approx(0.2));
    REQUIRE(tensor(2, 2) == Approx(0.3));
    REQUIRE(tensor(0, 1) == Approx(0.0).margin(1e-12));
}

TEST_CASE("scalar masses count on translations, are inertias on rotations and nothing on scalar points",
          "[analysis][mass]")
{
    const model::Model model =
        readModelText("MassProp;7;2.5\n" + grid(1, 0, 0, 0) + grid(2, 2, 0, 0) + "ScalarPoint;100\nScalarPoint;101\n" +
                      "MassDofInline;10;4.;1;1\n"
                      "MassDofInline;11;3.;1;5\n"
                      "MassDofProperty;12;7;1;1;2;1\n"
                      "MassScalarInline;13;9.;100;101\n"
                      "MassDofInline;14;6.;100;;101\n"
                      "MassDofInline;15;2.;1;4\n");
    REQUIRE(mass(model, 10) == 4.0);
    requireVec(massOf(model, 10).center.front(), 0.0, 0.0, 0.0);
    // A mass on a rotation (component 5, about y) is an inertia about that axis, no mass.
    REQUIRE(mass(model, 11) == 0.0);
    REQUIRE(massOf(model, 11).inertia.front().tensor(1, 1) == Approx(3.0));
    // Between two grid points the mass is at their middle.
    REQUIRE(mass(model, 12) == 2.5);
    requireVec(massOf(model, 12).center.front(), 1.0, 0.0, 0.0);
    REQUIRE(mass(model, 13) == 0.0); // scalar points only: the grid point weight generator ignores them
    REQUIRE(mass(model, 14) == 0.0);
    // CMASS2 15: 2 on the rotation about x at grid 1: an inertia 2 about x, no mass.
    const ElementMasses rotational = massOf(model, 15);
    REQUIRE(rotational.mass.front() == 0.0);
    REQUIRE(rotational.inertia.size() == 1);
    REQUIRE(rotational.inertia.front().tensor(0, 0) == Approx(2.0));
    const MassTotals total = totalOf(model);
    REQUIRE(total.sums.mass() == Approx(4.0 + 2.5));
    REQUIRE(total.sums.properties(base::Vec3{}).inertiaAtReference(0, 0) >= 2.0);
}

TEST_CASE("a bush and a fastener carry the mass of their property, the others have none", "[analysis][mass]")
{
    const model::Model model = readModelText(
        "BushProp;1;Stiffness;1.;1.;1.;1.;1.;1.;Mass;0.5\nSpringProp;2;100.\n" + grid(1, 0, 0, 0) + grid(2, 2, 0, 0) +
        "Bushing;10;1;1;2;;0.;0.;1.\nSpringDofProperty;11;2;1;1;2;1\nRigidBodyStar;12;1;123456;2\n");
    REQUIRE(mass(model, 10) == 0.5);
    requireVec(massOf(model, 10).center.front(), 1.0, 0.0, 0.0);
    REQUIRE(mass(model, 11) == 0.0);
    REQUIRE(mass(model, 12) == 0.0);
    REQUIRE(totalOf(model).sums.mass() == 0.5);
}

TEST_CASE("a model's mass properties follow from point masses: centre, parallel axis, reference point",
          "[analysis][mass]")
{
    // Two masses of 1 at x = -1 and x = +1.
    const model::Model model =
        readModelText(grid(1, -1, 0, 0) + grid(2, 1, 0, 0) + "PointMassInertia;10;1;;1.\nPointMassInertia;11;2;;1.\n");
    const MassTotals total = totalOf(model);
    REQUIRE(total.elements == 2);
    const MassProperties about = total.sums.properties(base::Vec3{0.0, 0.0, 5.0});
    REQUIRE(about.mass == 2.0);
    requireVec(about.centerOfGravity, 0.0, 0.0, 0.0);
    REQUIRE(about.inertiaAtCenter(0, 0) == Approx(0.0).margin(1e-12));
    REQUIRE(about.inertiaAtCenter(1, 1) == Approx(2.0));
    REQUIRE(about.inertiaAtCenter(2, 2) == Approx(2.0));
    // About (0, 0, 5): the parallel axis theorem adds 2 (25) to Ixx and Iyy and nothing to Izz; Ixz is -m x z = 0.
    REQUIRE(about.inertiaAtReference(0, 0) == Approx(50.0));
    REQUIRE(about.inertiaAtReference(1, 1) == Approx(52.0));
    REQUIRE(about.inertiaAtReference(2, 2) == Approx(2.0));
}

TEST_CASE("the weight factor multiplies every mass and inertia, and PARAM,WTMASS is found", "[analysis][mass]")
{
    const model::Model model =
        readModelText(std::string(MATERIAL) + "Parameter;WeightToMass;0.00259\nShellProp;1;1;2.\n" + grid(1, 0, 0, 0) +
                      grid(2, 1, 0, 0) + grid(3, 1, 1, 0) + grid(4, 0, 1, 0) +
                      "ShellQuad4;10;1;1;2;3;4\nPointMassInertia;11;1;;10.;0.;0.;0.;1.;0.;1.;0.;0.;1.\n");
    REQUIRE(MassCalculator::weightFactorOf(model) == Approx(0.00259));
    const MassTotals plain = totalOf(model);
    const MassTotals weighted = totalOf(model, 0, MassCalculator::weightFactorOf(model));
    REQUIRE(weighted.sums.mass() == Approx(plain.sums.mass() * 0.00259));
    REQUIRE(weighted.sums.properties(base::Vec3{}).inertiaAtCenter(0, 0) ==
            Approx(plain.sums.properties(base::Vec3{}).inertiaAtCenter(0, 0) * 0.00259));
    REQUIRE(MassCalculator::weightFactorOf(readModelText(grid(1, 0, 0, 0))) == 1.0);
}

TEST_CASE("the masses are grouped by property, material, element type and total", "[analysis][mass]")
{
    const model::Model model = readModelText(
        std::string(MATERIAL) + "IsoMat;2;1.E+5;;0.3;0.004\nShellProp;1;1;2.\nShellProp;2;2;1.\nRodProp;3;2;1.\n" +
        grid(1, 0, 0, 0) + grid(2, 1, 0, 0) + grid(3, 1, 1, 0) + grid(4, 0, 1, 0) +
        "ShellQuad4;10;1;1;2;3;4\n"
        "ShellQuad4;11;2;1;2;3;4\n"
        "Rod2;12;3;1;3\n"
        "PointMassInertia;13;1;;5.\n");
    const std::vector<RowSet> everything = MassCalculator::allElements(model);
    const MassGroups byProperty = MassCalculator::group(model, everything, MassGrouping::Property);
    REQUIRE(byProperty.byId.at(1).mass() == Approx(RHO * 2.0));
    REQUIRE(byProperty.byId.at(2).mass() == Approx(0.004));
    REQUIRE(byProperty.byId.at(3).mass() == Approx(0.004 * std::sqrt(2.0)));
    REQUIRE(byProperty.byId.at(0).mass() == Approx(5.0)); // the CONM2 has no property
    const MassGroups byMaterial = MassCalculator::group(model, everything, MassGrouping::Material);
    REQUIRE(byMaterial.byId.at(1).mass() == Approx(RHO * 2.0));
    REQUIRE(byMaterial.byId.at(2).mass() == Approx(0.004 + 0.004 * std::sqrt(2.0)));
    REQUIRE(byMaterial.byId.at(0).mass() == Approx(5.0));
    const MassGroups byType = MassCalculator::group(model, everything, MassGrouping::ElementType);
    REQUIRE(byType.byName.at("ShellQuad4").mass() == Approx(RHO * 2.0 + 0.004));
    REQUIRE(byType.byName.at("Rod2").mass() == Approx(0.004 * std::sqrt(2.0)));
    REQUIRE(byType.byName.at("PointMassInertia").mass() == Approx(5.0));
    double total = 0.0;
    for (const auto& [name, sums] : byType.byName)
        total += sums.mass();
    REQUIRE(total == Approx(totalOf(model).sums.mass()));
}

namespace
{

// The deck of the non-structural mass cases: shells of property 1 (elements 10, 20 of area 2 and 15 of area 8) and 2
// (element 30, area 2), a PCOMP shell (35, area 2), bars 41 and 42 (PBAR 5, length 2) and a CONROD 51 (length 2), plus
// every kind of NSM card.
std::string nsmDeck()
{
    return std::string(MATERIAL) +
           "ShellProp;1;1;2.\nShellProp;2;1;1.\nCompProp;3;;0.;;;;;;1;1.;0.\nBarProp;5;1;1.;1.;1.;1.\n" +
           grid(1, 0, 0, 0) + grid(2, 2, 0, 0) + grid(3, 4, 0, 0) + grid(4, 6, 0, 0) + grid(5, 0, 1, 0) +
           grid(6, 2, 1, 0) + grid(7, 4, 1, 0) + grid(8, 6, 1, 0) + grid(9, 0, 5, 0) + grid(10, 4, 5, 0) +
           grid(11, 4, 7, 0) + grid(12, 0, 7, 0) + grid(13, 8, 0, 0) + grid(14, 10, 0, 0) + grid(15, 8, 1, 0) +
           grid(16, 10, 1, 0) +
           "ShellQuad4;10;1;1;2;6;5\nShellQuad4;20;1;2;3;7;6\nShellQuad4;30;2;3;4;8;7\nShellQuad4;15;1;9;10;11;12\n"
           "ShellQuad4;35;3;13;14;16;15\nBar2;41;5;1;2;0.;;0.;1.\nBar2;42;5;3;4;0.;;0.;1.\n"
           "Rod2InlineSection;51;5;6;1;0.5\n"
           // set 1: a property, a second property; set 2: PCOMP counts as PSHELL, all of them
           "NonStructMass;1;ShellProp;1;0.1\nNonStructMass;1;ShellProp;2;0.2\nNonStructMass1;2;CompProp;0.05;All\n"
           // set 3: elements by ID; set 4: lumped on a property; set 5: one lumped mass on two properties
           "NonStructMass;3;Element;10;0.3;30;0.7\nNonStructMassLumped;4;ShellProp;1;12.\n"
           "NonStructMassLumped1;5;ShellProp;14.;1;2\n"
           // set 6: THRU with and without BY; set 7: sets added; set 8: line elements
           "NonStructMass1;6;Element;0.1;10;Through;30;By;10\nNonStructMass1;7;Element;0.1;10;Through;30\n"
           "NonStructMassUnion;9;1;3\n"
           "NonStructMass;8;BarProp;5;0.2\nNonStructMassLumped;10;BarProp;5;1.5\nNonStructMass1;11;Rod2InlineSection;0.3;"
           "All\n"
           "NonStructMass;12;Element;41;0.1\n"
           // set 13: ids that do not exist; set 14: shared by two cards; set 15: every element
           "NonStructMass;13;ShellProp;99;5.\nNonStructMass;14;ShellProp;1;0.1\nNonStructMass1;14;ShellProp;0.1;2\n"
           "NonStructMass1;15;Element;0.01;All\n"
           // set 16: a lumped mass on a shell and a bar
           "NonStructMassLumped1;16;Element;1.;10;41\n";
}

} // namespace

TEST_CASE("NSM and NSM1 put a mass per unit area or length on the elements of a property or on elements",
          "[analysis][mass]")
{
    const model::Model model = readModelText(nsmDeck());
    const double base10 = mass(model, 10);
    // Set 1: 0.1 per unit area on the elements of PSHELL 1 (area 2), 0.2 on PSHELL 2; the PCOMP and the bars are
    // untouched.
    REQUIRE(mass(model, 10, 1) == Approx(base10 + 0.1 * 2.0));
    REQUIRE(mass(model, 15, 1) == Approx(mass(model, 15) + 0.1 * 8.0));
    REQUIRE(mass(model, 30, 1) == Approx(mass(model, 30) + 0.2 * 2.0));
    REQUIRE(mass(model, 35, 1) == Approx(mass(model, 35)));
    REQUIRE(mass(model, 41, 1) == Approx(mass(model, 41)));
    // Set 2: PCOMP ALL is every shell property: the PCOMP element and the PSHELL
    // ones.
    REQUIRE(mass(model, 35, 2) == Approx(mass(model, 35) + 0.05 * 2.0));
    REQUIRE(mass(model, 10, 2) == Approx(base10 + 0.05 * 2.0));
    REQUIRE(mass(model, 41, 2) == Approx(mass(model, 41)));
    // Set 3: elements by ID.
    REQUIRE(mass(model, 10, 3) == Approx(base10 + 0.3 * 2.0));
    REQUIRE(mass(model, 30, 3) == Approx(mass(model, 30) + 0.7 * 2.0));
    REQUIRE(mass(model, 20, 3) == Approx(mass(model, 20)));
    // Set 8 on the PBAR (per unit length), set 12 on an element.
    REQUIRE(mass(model, 41, 8) == Approx(mass(model, 41) + 0.2 * 2.0));
    REQUIRE(mass(model, 42, 8) == Approx(mass(model, 42) + 0.2 * 2.0));
    REQUIRE(mass(model, 42, 12) == Approx(mass(model, 42)));
    REQUIRE(mass(model, 41, 12) == Approx(mass(model, 41) + 0.1 * 2.0));
    // CONROD as a type: the connection rods of the model.
    REQUIRE(mass(model, 51, 11) == Approx(mass(model, 51) + 0.3 * 2.0));
    // The non-structural mass of the set is separate from the one of the property in the arrays.
    REQUIRE(massOf(model, 10, 1).nonStructural.front() == Approx(0.1 * 2.0));
}

TEST_CASE("THRU and BY list the elements of a range, with the IDs between the ends that exist", "[analysis][mass]")
{
    const model::Model model = readModelText(nsmDeck());
    // 10 THRU 30 BY 10 is 10, 20, 30; without BY every element with an ID from 10 to 30: also 15.
    REQUIRE(mass(model, 20, 6) == Approx(mass(model, 20) + 0.1 * 2.0));
    REQUIRE(mass(model, 15, 6) == Approx(mass(model, 15)));
    REQUIRE(mass(model, 15, 7) == Approx(mass(model, 15) + 0.1 * 8.0));
    REQUIRE(mass(model, 35, 7) == Approx(mass(model, 35)));
}

TEST_CASE("a THRU to the end of the ID range is matched through the elements of the model and not ID by ID",
          "[analysis][mass][robustness]")
{
    // "10 THRU 2147483647" would be two billion lookups (or 8 GB of IDs); it is matched as the elements in the range.
    const model::Model model = readModelText(nsmDeck() +
                                             "NonStructMass1;17;Element;0.1;10;Through;2147483647\n"
                                             "NonStructMass1;18;ShellProp;0.1;2;Through;99999999\n"
                                             "NonStructMass1;19;Element;0.1;20;Through;2147483647;By;10\n");
    // set 17: every element from 10 on, as set 7 does up to 30
    REQUIRE(mass(model, 15, 17) == Approx(mass(model, 15) + 0.1 * 8.0));
    REQUIRE(mass(model, 35, 17) == Approx(mass(model, 35) + 0.1 * 2.0));
    // set 18: the shells of the properties 2 and 3 (a PCOMP counts as a PSHELL, as in set 2), not of PSHELL 1
    REQUIRE(mass(model, 30, 18) == Approx(mass(model, 30) + 0.1 * 2.0));
    REQUIRE(mass(model, 35, 18) == Approx(mass(model, 35) + 0.1 * 2.0));
    REQUIRE(mass(model, 15, 18) == Approx(mass(model, 15)));
    // set 19: BY 10 from 20 is 20, 30, ...: not 15 nor 35
    REQUIRE(mass(model, 20, 19) == Approx(mass(model, 20) + 0.1 * 2.0));
    REQUIRE(mass(model, 30, 19) == Approx(mass(model, 30) + 0.1 * 2.0));
    REQUIRE(mass(model, 15, 19) == Approx(mass(model, 15)));
    REQUIRE(mass(model, 35, 19) == Approx(mass(model, 35)));
}

TEST_CASE("NSML and NSML1 spread a mass over the elements in proportion to their area or length", "[analysis][mass]")
{
    const model::Model model = readModelText(nsmDeck());
    // Set 4: 12 over the elements of PSHELL 1: the areas are 2, 2 and 8, so 2, 2 and 8 of the mass.
    REQUIRE(mass(model, 10, 4) == Approx(mass(model, 10) + 2.0));
    REQUIRE(mass(model, 20, 4) == Approx(mass(model, 20) + 2.0));
    REQUIRE(mass(model, 15, 4) == Approx(mass(model, 15) + 8.0));
    REQUIRE(mass(model, 30, 4) == Approx(mass(model, 30)));
    // Set 5: 14 over the elements of PSHELL 1 and 2 together: the areas total 14, so each element gets its area.
    REQUIRE(mass(model, 10, 5) == Approx(mass(model, 10) + 2.0));
    REQUIRE(mass(model, 30, 5) == Approx(mass(model, 30) + 2.0));
    REQUIRE(mass(model, 15, 5) == Approx(mass(model, 15) + 8.0));
    // Set 10: 1.5 over the two bars, of equal length.
    REQUIRE(mass(model, 41, 10) == Approx(mass(model, 41) + 0.75));
    REQUIRE(mass(model, 42, 10) == Approx(mass(model, 42) + 0.75));
    // The total mass added is the value, whatever the elements.
    REQUIRE(totalOf(model, 4).sums.mass() == Approx(totalOf(model).sums.mass() + 12.0));
    REQUIRE(totalOf(model, 5).sums.mass() == Approx(totalOf(model).sums.mass() + 14.0));
}

TEST_CASE("NSMADD adds sets, cards of one set ID all apply, an ID that names nothing is ignored", "[analysis][mass]")
{
    const model::Model model = readModelText(nsmDeck());
    // Set 9 is set 1 plus set 3.
    REQUIRE(mass(model, 10, 9) == Approx(mass(model, 10) + 0.1 * 2.0 + 0.3 * 2.0));
    REQUIRE(mass(model, 30, 9) == Approx(mass(model, 30) + 0.2 * 2.0 + 0.7 * 2.0));
    // Set 14: an NSM card and an NSM1 card with the same set ID.
    REQUIRE(mass(model, 10, 14) == Approx(mass(model, 10) + 0.1 * 2.0));
    REQUIRE(mass(model, 30, 14) == Approx(mass(model, 30) + 0.1 * 2.0));
    // Set 13 names a property that does not exist.
    REQUIRE(totalOf(model, 13).sums.mass() == Approx(totalOf(model).sums.mass()));
    // ELEMENT ALL: every element that has an area or a length.
    REQUIRE(mass(model, 41, 15) == Approx(mass(model, 41) + 0.01 * 2.0));
    REQUIRE(mass(model, 10, 15) == Approx(mass(model, 10) + 0.01 * 2.0));
    const std::vector<base::EntityId> sets = NonStructuralMass::setIds(model);
    REQUIRE(sets.front() == 1);
    REQUIRE(sets.back() == 16);
    REQUIRE(NonStructuralMass::exists(model, 9));
    REQUIRE_FALSE(NonStructuralMass::exists(model, 77));
}

TEST_CASE("a set that does not exist, and a lumped mass shared by areas and lengths, are errors", "[analysis][mass]")
{
    const model::Model model = readModelText(nsmDeck());
    REQUIRE_THROWS_AS(NonStructuralMass::resolve(model, 77), MassError);
    REQUIRE_THROWS_AS(NonStructuralMass::resolve(model, 16), MassError);
}

TEST_CASE("the Case Control selects the set of non-structural mass", "[analysis][mass]")
{
    const model::Model selected = readModelText(
        "[executive]\nSolutionSequence;101\n[case]\nNonStructMassSelection;4\nSubcaseStart;1\n[model]\n" + nsmDeck());
    REQUIRE(NonStructuralMass::selectedByCaseControl(selected) == std::optional<base::EntityId>(4));
    const model::Model inSubcase = readModelText(
        "[executive]\nSolutionSequence;101\n[case]\nSubcaseStart;1\nNonStructMassSelection;8\n[model]\n" + nsmDeck());
    REQUIRE(NonStructuralMass::selectedByCaseControl(inSubcase) == std::optional<base::EntityId>(8));
    REQUIRE_FALSE(NonStructuralMass::selectedByCaseControl(readModelText(nsmDeck())).has_value());
}

TEST_CASE("a wrong property or a missing material is an error that names the element", "[analysis][mass]")
{
    // A property that is not a shell property on a shell, a property or a material that does not exist.
    const model::Model bad =
        readModelText(std::string(MATERIAL) + "BarProp;1;1;1.;1.;1.;1.\n" + grid(1, 0, 0, 0) + grid(2, 1, 0, 0) +
                      grid(3, 1, 1, 0) + grid(4, 0, 1, 0) + "ShellQuad4;10;1;1;2;3;4\n");
    REQUIRE_THROWS_AS(mass(bad, 10), MassError);
    const model::Model missing = readModelText(grid(1, 0, 0, 0) + grid(2, 1, 0, 0) + grid(3, 1, 1, 0) +
                                               grid(4, 0, 1, 0) + "ShellQuad4;10;9;1;2;3;4\n");
    REQUIRE_THROWS_AS(mass(missing, 10), MassError);
    const model::Model noMaterial = readModelText("ShellProp;1;5;1.\n" + grid(1, 0, 0, 0) + grid(2, 1, 0, 0) +
                                                  grid(3, 1, 1, 0) + grid(4, 0, 1, 0) + "ShellQuad4;10;1;1;2;3;4\n");
    REQUIRE_THROWS_AS(mass(noMaterial, 10), MassError);
}

TEST_CASE("the mass of many elements equals the mass of each one", "[analysis][mass]")
{
    const model::Model model = readModelText(nsmDeck());
    base::ThreadPool pool(3);
    const std::vector<RowSet> everything = MassCalculator::allElements(model);
    for (const RowSet& set : everything)
    {
        const ElementMasses together = MassCalculator::compute(model, set.type, set.rows, MassOptions{}, &pool);
        for (std::size_t index = 0; index < set.rows.size(); ++index)
        {
            const ElementMasses one = MassCalculator::of(model, model::CardLocation{set.type, set.rows[index]});
            REQUIRE(together.mass[index] == one.mass.front());
            REQUIRE(together.center[index].x == one.center.front().x);
        }
    }
}
