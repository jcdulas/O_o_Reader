#include "oor/analysis/derived/DerivedQuantities.hpp"
#include "oor/base/errors/Errors.hpp"
#include "support/ResultTestTables.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstddef>
#include <memory>
#include <numbers>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::analysis;
using Catch::Approx;
using oor::testing::makeTable;
using oor::testing::TableSpec;

namespace
{

std::shared_ptr<results::ResultTable> shells(std::vector<double> values, std::vector<std::int32_t> ids,
                                             std::vector<std::int32_t> layers, bool strain = false,
                                             std::size_t steps = 1, std::vector<double> stepValues = {})
{
    TableSpec spec;
    spec.kind = strain ? results::ResultKind::Strain : results::ResultKind::Stress;
    spec.elementName = "ShellQuad4";
    spec.ids = std::move(ids);
    spec.layers = std::move(layers);
    spec.components =
        strain ? std::vector<std::string>{"exx", "eyy", "exy"} : std::vector<std::string>{"oxx", "oyy", "txy"};
    spec.values = std::move(values);
    spec.steps = steps;
    spec.stepValues = std::move(stepValues);
    return makeTable(spec);
}

std::shared_ptr<results::ResultTable> solids(std::vector<double> values, bool strain = false)
{
    TableSpec spec;
    spec.kind = strain ? results::ResultKind::Strain : results::ResultKind::Stress;
    spec.elementName = "SolidHexa";
    spec.ids.assign(values.size() / 6, 5);
    spec.layers.assign(values.size() / 6, 1);
    spec.components = strain ? std::vector<std::string>{"exx", "eyy", "ezz", "exy", "eyz", "exz"}
                             : std::vector<std::string>{"oxx", "oyy", "ozz", "txy", "tyz", "txz"};
    spec.values = std::move(values);
    return makeTable(spec);
}

double at(const std::shared_ptr<results::ResultTable>& table, std::size_t step, std::size_t row, std::size_t component)
{
    const results::ResultData& data = *table->data();
    return data.values<double>()[(step * data.rowCount + row) * data.componentCount + component];
}

} // namespace

TEST_CASE("von Mises, principal values, angle and maximum shear of a plane stress by hand", "[analysis][derived]")
{
    const auto table = shells({100, 0, 0, 0, 0, 50, 60, -20, 30}, {1, 2, 3}, {1, 1, 1});
    const auto vonMises = DerivedQuantities::compute(*table, DerivedKind::VonMises);
    REQUIRE(vonMises->meta().derivation == "vonMises");
    REQUIRE(vonMises->meta().components == std::vector<std::string>{"vonMises"});
    REQUIRE(at(vonMises, 0, 0, 0) == Approx(100.0));
    REQUIRE(at(vonMises, 0, 1, 0) == Approx(50.0 * std::sqrt(3.0)));
    REQUIRE(at(vonMises, 0, 2, 0) == Approx(std::sqrt(7900.0)));
    const auto principal = DerivedQuantities::compute(*table, DerivedKind::Principal);
    REQUIRE(principal->meta().components == std::vector<std::string>{"major", "minor", "angle"});
    REQUIRE(at(principal, 0, 0, 0) == Approx(100.0));
    REQUIRE(at(principal, 0, 0, 1) == Approx(0.0).margin(1e-12));
    REQUIRE(at(principal, 0, 0, 2) == Approx(0.0).margin(1e-12));
    REQUIRE(at(principal, 0, 1, 0) == Approx(50.0));
    REQUIRE(at(principal, 0, 1, 1) == Approx(-50.0));
    REQUIRE(at(principal, 0, 1, 2) == Approx(45.0));
    REQUIRE(at(principal, 0, 2, 0) == Approx(70.0));
    REQUIRE(at(principal, 0, 2, 1) == Approx(-30.0));
    REQUIRE(at(principal, 0, 2, 2) == Approx(0.5 * std::atan2(60.0, 80.0) * 180.0 / std::numbers::pi));
    const auto shear = DerivedQuantities::compute(*table, DerivedKind::MaxShear);
    REQUIRE(at(shear, 0, 0, 0) == Approx(50.0));
    REQUIRE(at(shear, 0, 2, 0) == Approx(50.0));
    REQUIRE_THROWS_AS(DerivedQuantities::compute(*table, DerivedKind::OctahedralShear), base::UsageError);
}

TEST_CASE("a strain uses its tensor shear and the equivalent strain of the solver's tables", "[analysis][derived]")
{
    const auto table = shells({1.0e-3, 0.0, 2.0e-3}, {1}, {1}, true);
    const double radius = std::hypot(0.5e-3, 1.0e-3);
    const auto principal = DerivedQuantities::compute(*table, DerivedKind::Principal);
    REQUIRE(at(principal, 0, 0, 0) == Approx(0.5e-3 + radius));
    REQUIRE(at(principal, 0, 0, 1) == Approx(0.5e-3 - radius));
    REQUIRE(at(DerivedQuantities::compute(*table, DerivedKind::MaxShear), 0, 0, 0) == Approx(2.0 * radius));
    REQUIRE(at(DerivedQuantities::compute(*table, DerivedKind::VonMises), 0, 0, 0) ==
            Approx((2.0 / 3.0) * std::sqrt(1.0e-6 + 3.0e-6)));
}

TEST_CASE("solid quantities by hand: uniaxial, hydrostatic, pure shear", "[analysis][derived]")
{
    const auto table =
        solids({100, 0, 0, 0, 0, 0, /**/ 50, 50, 50, 0, 0, 0, /**/ 0, 0, 0, 40, 0, 0, /**/ 10, 20, 30, 0, 0, 0});
    const auto vonMises = DerivedQuantities::compute(*table, DerivedKind::VonMises);
    REQUIRE(at(vonMises, 0, 0, 0) == Approx(100.0));
    REQUIRE(at(vonMises, 0, 1, 0) == Approx(0.0).margin(1e-9));
    REQUIRE(at(vonMises, 0, 2, 0) == Approx(40.0 * std::sqrt(3.0)));
    REQUIRE(at(vonMises, 0, 3, 0) == Approx(std::sqrt(0.5 * (100 + 100 + 400))));
    const auto principal = DerivedQuantities::compute(*table, DerivedKind::Principal);
    REQUIRE(principal->meta().components == std::vector<std::string>{"o1", "o2", "o3"});
    REQUIRE(at(principal, 0, 0, 0) == Approx(100.0));
    REQUIRE(at(principal, 0, 1, 2) == Approx(50.0));
    REQUIRE(at(principal, 0, 2, 0) == Approx(40.0));
    REQUIRE(at(principal, 0, 2, 1) == Approx(0.0).margin(1e-9));
    REQUIRE(at(principal, 0, 2, 2) == Approx(-40.0));
    REQUIRE(at(principal, 0, 3, 0) == Approx(30.0));
    REQUIRE(at(principal, 0, 3, 2) == Approx(10.0));
    const auto octahedral = DerivedQuantities::compute(*table, DerivedKind::OctahedralShear);
    REQUIRE(at(octahedral, 0, 0, 0) == Approx(std::sqrt(2.0) * 100.0 / 3.0));
    REQUIRE(at(octahedral, 0, 2, 0) == Approx(std::sqrt(6.0) * 40.0 / 3.0));
    REQUIRE(at(DerivedQuantities::compute(*table, DerivedKind::MaxShear), 0, 0, 0) == Approx(50.0));
}

TEST_CASE("the eigenvalues of a general solid state have the right invariants", "[analysis][derived]")
{
    const auto table = solids({12, -7, 30, 5, -9, 14});
    const auto principal = DerivedQuantities::compute(*table, DerivedKind::Principal);
    const double o1 = at(principal, 0, 0, 0);
    const double o2 = at(principal, 0, 0, 1);
    const double o3 = at(principal, 0, 0, 2);
    REQUIRE(o1 >= o2);
    REQUIRE(o2 >= o3);
    REQUIRE(o1 + o2 + o3 == Approx(12 - 7 + 30));
    REQUIRE(o1 * o2 + o2 * o3 + o3 * o1 == Approx(12 * -7 + -7 * 30 + 30 * 12 - 25 - 81 - 196));
    const double determinant = 12 * (-7 * 30 - 81) - 5 * (5 * 30 - -9 * 14) + 14 * (5 * -9 - -7 * 14);
    REQUIRE(o1 * o2 * o3 == Approx(determinant));
}

TEST_CASE("the surface of a shell is a fibre or the mean of two, by place", "[analysis][derived]")
{
    // two elements, Z1 and Z2 each: oxx oyy txy
    const auto table = shells({10, 0, 0, 30, 20, 5, 50, 0, 0, -40, 0, 0}, {10, 10, 20, 20}, {1, 2, 1, 2});
    const auto z2 = DerivedQuantities::surface(*table, Surface::Z2);
    REQUIRE(z2->data()->layers == std::vector<std::int32_t>{2, 2});
    REQUIRE(at(z2, 0, 0, 0) == Approx(30.0));
    const auto middle = DerivedQuantities::surface(*table, Surface::Middle);
    REQUIRE(middle->meta().derivation == "surface(middle)");
    REQUIRE(middle->data()->ids == std::vector<std::int32_t>{10, 20});
    REQUIRE(middle->data()->layers == std::vector<std::int32_t>{0, 0});
    REQUIRE(at(middle, 0, 0, 0) == Approx(20.0));
    REQUIRE(at(middle, 0, 0, 1) == Approx(10.0));
    REQUIRE(at(middle, 0, 0, 2) == Approx(2.5));
    REQUIRE(at(middle, 0, 1, 0) == Approx(5.0));
    // a table of solids has no fibres
    REQUIRE_THROWS_AS(DerivedQuantities::surface(*solids({1, 2, 3, 4, 5, 6}), Surface::Z1), base::UsageError);
    const auto byLayer = DerivedQuantities::byLayer(*table);
    REQUIRE(byLayer.size() == 2);
    REQUIRE(byLayer[1].first == 2);
}

TEST_CASE("the maximum over the layers is the largest value at each place and says where", "[analysis][derived]")
{
    const auto table = shells({10, 0, 0, 30, 20, 5, 50, 0, 0, -40, 0, 0}, {10, 10, 20, 20}, {1, 2, 1, 2});
    const auto best = DerivedQuantities::maxOverLayers(*table, "vonMises");
    REQUIRE(best->meta().components == std::vector<std::string>{"vonMises", "layer"});
    REQUIRE(best->data()->ids == std::vector<std::int32_t>{10, 20});
    REQUIRE(at(best, 0, 0, 0) == Approx(std::sqrt(900.0 - 600.0 + 400.0 + 75.0)));
    REQUIRE(at(best, 0, 0, 1) == Approx(2.0));
    REQUIRE(at(best, 0, 1, 0) == Approx(50.0));
    REQUIRE(at(best, 0, 1, 1) == Approx(1.0));
    // a stored component, and a quantity that is neither
    REQUIRE(at(DerivedQuantities::maxOverLayers(*table, "oxx"), 0, 0, 0) == Approx(30.0));
    REQUIRE_THROWS_AS(DerivedQuantities::maxOverLayers(*table, "nothing"), base::UsageError);
}

TEST_CASE("the extremes over the steps give the value and the step", "[analysis][derived]")
{
    const auto table = shells({1, 0, 0, -4, 0, 0, 3, 0, 0, 2, 0, 0}, {1}, {1}, false, 4, {0.5, 1.0, 1.5, 2.0});
    const auto extremes = DerivedQuantities::maxMin(*table, {"oxx"});
    REQUIRE(extremes->meta().components == std::vector<std::string>{"oxxMax", "oxxMaxStep", "oxxMin", "oxxMinStep"});
    REQUIRE(at(extremes, 0, 0, 0) == Approx(3.0));
    REQUIRE(at(extremes, 0, 0, 1) == Approx(1.5));
    REQUIRE(at(extremes, 0, 0, 2) == Approx(-4.0));
    REQUIRE(at(extremes, 0, 0, 3) == Approx(1.0));
    REQUIRE(extremes->meta().stepKind == results::StepKind::None);
}

TEST_CASE("a complex table is taken at a phase or as magnitudes, and has no von Mises value", "[analysis][derived]")
{
    TableSpec spec;
    spec.kind = results::ResultKind::Displacements;
    spec.ids = {1};
    spec.components = {"t1", "t2"};
    spec.values = {3, 4, 0, 1}; // t1 = 3 + 4i, t2 = 0 + 1i
    spec.type = results::ValueType::Complex128;
    const auto table = makeTable(spec);
    const auto zero = DerivedQuantities::atPhase(*table, 0.0);
    REQUIRE(zero->meta().valueType == results::ValueType::Float64);
    REQUIRE(at(zero, 0, 0, 0) == Approx(3.0));
    const auto ninety = DerivedQuantities::atPhase(*table, 90.0);
    REQUIRE(at(ninety, 0, 0, 0) == Approx(-4.0));
    REQUIRE(at(ninety, 0, 0, 1) == Approx(-1.0));
    REQUIRE(at(DerivedQuantities::magnitude(*table), 0, 0, 0) == Approx(5.0));
    REQUIRE_THROWS_AS(DerivedQuantities::atPhase(*zero, 10.0), base::UsageError);
    REQUIRE_THROWS_AS(DerivedQuantities::compute(*table, DerivedKind::VonMises), base::UsageError);
}

TEST_CASE("a table of single precision values gives single precision derived values", "[analysis][derived]")
{
    TableSpec spec;
    spec.kind = results::ResultKind::Stress;
    spec.elementName = "ShellQuad4";
    spec.ids = {1};
    spec.layers = {1};
    spec.components = {"oxx", "oyy", "txy"};
    spec.values = {100, 50, 20};
    spec.type = results::ValueType::Float32;
    const auto vonMises = DerivedQuantities::compute(*makeTable(spec), DerivedKind::VonMises);
    REQUIRE(vonMises->meta().valueType == results::ValueType::Float32);
    REQUIRE(vonMises->data()->values<float>()[0] ==
            Approx(std::sqrt(10000.0 - 5000.0 + 2500.0 + 1200.0)).epsilon(1e-6));
}

TEST_CASE("vonMisesOf gives the bits of scalarsOf on strided and repeated columns", "[analysis][derived]")
{
    // eight tensors of six components, interleaved (a stride of 6), the yy entry one value repeated (a stride of 0)
    constexpr std::size_t ROWS = 8;
    std::vector<double> interleaved(ROWS * 6);
    for (std::size_t index = 0; index < interleaved.size(); ++index)
        interleaved[index] = std::sin(0.37 * static_cast<double>(index) + 0.1) * 150.0;
    const double yy = -42.5;
    TensorColumns columns;
    for (std::size_t entry = 0; entry < 6; ++entry)
    {
        columns.data[entry] = interleaved.data() + entry;
        columns.stride[entry] = 6;
    }
    columns.data[1] = &yy;
    columns.stride[1] = 0;
    for (const bool solid : {false, true})
        for (const bool strain : {false, true})
        {
            std::vector<double> found(ROWS - 3);
            DerivedQuantities::vonMisesOf(strain, solid, columns, 3, ROWS, found.data());
            for (std::size_t row = 3; row < ROWS; ++row)
            {
                double components[6];
                for (std::size_t entry = 0; entry < 6; ++entry)
                    components[entry] = interleaved[row * 6 + entry];
                components[1] = yy;
                REQUIRE(found[row - 3] == DerivedQuantities::scalarsOf(strain, solid, components).vonMises);
            }
        }
}
