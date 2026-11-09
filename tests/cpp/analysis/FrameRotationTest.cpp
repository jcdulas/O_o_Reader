#include "oor/analysis/frames/FrameConventions.hpp"
#include "oor/analysis/frames/FrameRotation.hpp"
#include "oor/analysis/frames/StoredFrames.hpp"
#include "oor/base/errors/Errors.hpp"
#include "support/ModelTestSupport.hpp"
#include "support/ResultTestTables.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <memory>
#include <numbers>
#include <random>
#include <string>

using namespace oor;
using namespace oor::analysis;
using Catch::Approx;
using oor::testing::makeTable;
using oor::testing::makeTables;
using oor::testing::MemoryResultFile;
using oor::testing::readModelText;
using oor::testing::TableSpec;

namespace
{

// A unit square turned nowhere with THETA 30, a ply laminate on a second one, and grid points in a cylindrical system.
const char* const DECK =
    "IsoMat;1;2.1E+5;;0.3\n"
    "ShellProp;1;1;0.1\n"
    "CompProp;2;;;;;;;;1;0.01;0.;Yes;1;0.01;45.\n"
    "SolidProp;3;1;0\n"
    "CylindricalFrame2;1;0;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
    "Node;1;;1.;0.;0.\n"
    "Node;2;;0.;2.;0.;1\n"
    "Node;3;;0.;0.;0.;1\n"
    "Node;11;;0.;0.;0.\n"
    "Node;12;;1.;0.;0.\n"
    "Node;13;;1.;1.;0.\n"
    "Node;14;;0.;1.;0.\n"
    "ShellQuad4;10;1;11;12;13;14;30.\n"
    "ShellQuad4;20;2;11;12;13;14\n"
    "Node;21;;0.;0.;0.\n"
    "Node;22;;1.;0.;0.\n"
    "Node;23;;1.;1.;0.\n"
    "Node;24;;0.;1.;0.\n"
    "Node;25;;0.;0.;1.\n"
    "Node;26;;1.;0.;1.\n"
    "Node;27;;1.;1.;1.\n"
    "Node;28;;0.;1.;1.\n"
    "SolidHexa;30;3;21;22;23;24;25;26;27;28\n"
    "Rod2;40;1;11;12\n";

struct Case
{
    std::shared_ptr<model::Model> model;
    std::shared_ptr<results::ResultTables> tables;
    std::shared_ptr<results::ResultAttachment> attachment;
    std::vector<std::shared_ptr<results::ResultTable>> list;
};

Case attach(const std::vector<TableSpec>& specs)
{
    Case result;
    result.model = std::make_shared<model::Model>(readModelText(DECK));
    for (const TableSpec& spec : specs)
        result.list.push_back(makeTable(spec));
    result.tables = makeTables(result.list);
    result.attachment = attachResults(result.model, std::make_shared<MemoryResultFile>(), result.tables);
    return result;
}

TargetFrame basic()
{
    return TargetFrame{FrameTargetKind::Basic, 0, {}};
}

TargetFrame target(FrameTargetKind kind, std::int32_t system = 0)
{
    return TargetFrame{kind, system, {}};
}

TableSpec nodal(std::vector<std::int32_t> ids, std::vector<double> values, bool complexValues = false,
                bool single = false)
{
    TableSpec spec;
    spec.kind = results::ResultKind::Displacements;
    spec.ids = std::move(ids);
    spec.components = {"t1", "t2", "t3", "r1", "r2", "r3"};
    spec.values = std::move(values);
    spec.type = complexValues ? (single ? results::ValueType::Complex64 : results::ValueType::Complex128)
                              : (single ? results::ValueType::Float32 : results::ValueType::Float64);
    return spec;
}

TableSpec tensor(const std::string& name, results::ResultKind kind, std::vector<std::string> components,
                 std::vector<std::int32_t> elements, std::vector<std::int32_t> layers, std::vector<double> values)
{
    TableSpec spec;
    spec.kind = kind;
    spec.elementName = name;
    spec.ids = std::move(elements);
    spec.layers = std::move(layers);
    spec.components = std::move(components);
    spec.values = std::move(values);
    return spec;
}

const results::ResultData& dataOf(const std::shared_ptr<results::ResultTable>& table)
{
    return *table->data();
}

double at(const std::shared_ptr<results::ResultTable>& table, std::size_t row, std::size_t component)
{
    const results::ResultData& data = dataOf(table);
    return data.values<double>()[row * data.componentCount + component];
}

} // namespace

TEST_CASE("a displacement in a cylindrical system turns to the basic system with the directions at the node",
          "[analysis][frames]")
{
    // node 1 in the basic system at (1, 0, 0); node 2 in the cylindrical system 1 at (0, 2, 0): radial is y, tangential
    // -x
    Case c = attach({nodal({1, 2, 3}, {1, 2, 3, 4, 5, 6, /**/ 1, 0, 0, 0, 0, 1, /**/ 0, 1, 0, 1, 0, 0})});
    const std::shared_ptr<results::ResultTable> basicTable =
        FrameRotation::rotate(*c.model, *c.attachment, *c.list[0], basic());
    REQUIRE(basicTable->meta().derivation == "inFrame(basic)");
    REQUIRE(basicTable->meta().storedFrame == results::StoredFrame::Basic);
    for (std::size_t component = 0; component < 6; ++component)
        REQUIRE(at(basicTable, 0, component) == Approx(static_cast<double>(component + 1)));
    REQUIRE(at(basicTable, 1, 0) == Approx(0.0).margin(1e-12)); // radial (1, 0, 0) is y
    REQUIRE(at(basicTable, 1, 1) == Approx(1.0));
    REQUIRE(at(basicTable, 1, 5) == Approx(1.0)); // r3 along z
    // node 3 is on the axis: the angle is 0 there, the radial direction is x
    REQUIRE(at(basicTable, 2, 0) == Approx(0.0).margin(1e-12));
    REQUIRE(at(basicTable, 2, 1) == Approx(1.0)); // t2 is the tangential direction (y at angle 0)
    REQUIRE(at(basicTable, 2, 3) == Approx(1.0));
    // the matrices say it
    const std::vector<base::Mat3> matrices = FrameRotation::storedMatrices(*c.model, *c.attachment, *c.list[0]);
    REQUIRE(matrices.size() == 3);
    REQUIRE(matrices[1].column(0).y == Approx(1.0));
    REQUIRE(matrices[1].column(1).x == Approx(-1.0));
    // and back
    const auto back = FrameRotation::rotate(*c.model, *c.attachment, *basicTable, target(FrameTargetKind::NodeOutput));
    for (std::size_t row = 0; row < 3; ++row)
        for (std::size_t component = 0; component < 6; ++component)
            REQUIRE(at(back, row, component) ==
                    Approx(c.list[0]->data()->values<double>()[row * 6 + component]).margin(1e-12));
}

TEST_CASE("a plane stress turns from the element system to the material system by hand", "[analysis][frames]")
{
    Case c = attach({tensor("ShellQuad4", results::ResultKind::Stress, {"oxx", "oyy", "txy"}, {10, 10}, {1, 2},
                            {100, 50, 20, 30, 20, 5})});
    const auto material = FrameRotation::rotate(*c.model, *c.attachment, *c.list[0], target(FrameTargetKind::Material));
    const double s = 0.5;
    const double k = std::sqrt(3.0) / 2.0;
    REQUIRE(material->meta().components == std::vector<std::string>{"oxx", "oyy", "txy"});
    REQUIRE(at(material, 0, 0) == Approx(100 * k * k + 50 * s * s + 40 * s * k));
    REQUIRE(at(material, 0, 1) == Approx(100 * s * s + 50 * k * k - 40 * s * k));
    REQUIRE(at(material, 0, 2) == Approx(-50 * s * k + 20 * (k * k - s * s)));
    REQUIRE(at(material, 1, 0) == Approx(30 * k * k + 20 * s * s + 10 * s * k));
    // the element system is stored: the element target is the identity
    const auto element = FrameRotation::rotate(*c.model, *c.attachment, *c.list[0], target(FrameTargetKind::Element));
    REQUIRE(at(element, 0, 0) == Approx(100.0));
    REQUIRE(at(element, 0, 2) == Approx(20.0));
    // turned out of the plane: six components, the same von Mises value by hand
    const auto basicTable = FrameRotation::rotate(*c.model, *c.attachment, *c.list[0], basic());
    REQUIRE(basicTable->meta().components.size() == 6);
    // the element axes are the basic axes: the same numbers
    REQUIRE(at(basicTable, 0, 0) == Approx(100.0));
    REQUIRE(at(basicTable, 0, 3) == Approx(20.0));
    REQUIRE(at(basicTable, 0, 2) == Approx(0.0).margin(1e-12));
}

TEST_CASE("a strain turns with half of its engineering shear and gives it back doubled", "[analysis][frames]")
{
    Case c =
        attach({tensor("ShellQuad4", results::ResultKind::Strain, {"exx", "eyy", "exy"}, {10}, {1}, {100, 50, 40})});
    const auto material = FrameRotation::rotate(*c.model, *c.attachment, *c.list[0], target(FrameTargetKind::Material));
    const double s = 0.5;
    const double k = std::sqrt(3.0) / 2.0;
    REQUIRE(at(material, 0, 0) == Approx(100 * k * k + 50 * s * s + 40 * s * k));
    REQUIRE(at(material, 0, 2) == Approx(2.0 * (-50 * s * k + 20 * (k * k - s * s))));
}

TEST_CASE("shell resultants turn as two tensors and a vector in the plane", "[analysis][frames]")
{
    Case c = attach(
        {tensor("ShellQuad4", results::ResultKind::ElementForces, {"nx", "ny", "nxy", "mx", "my", "mxy", "qx", "qy"},
                {10}, {1}, {100, 50, 20, 100, 50, 20, 3, 4})});
    const auto material = FrameRotation::rotate(*c.model, *c.attachment, *c.list[0], target(FrameTargetKind::Material));
    const double s = 0.5;
    const double k = std::sqrt(3.0) / 2.0;
    REQUIRE(at(material, 0, 0) == Approx(100 * k * k + 50 * s * s + 40 * s * k));
    REQUIRE(at(material, 0, 3) == Approx(at(material, 0, 0)));
    REQUIRE(at(material, 0, 6) == Approx(k * 3 + s * 4));
    REQUIRE(at(material, 0, 7) == Approx(-s * 3 + k * 4));
    // out of the plane the resultants are refused
    REQUIRE_THROWS_AS(FrameRotation::rotate(*c.model, *c.attachment, *c.list[0], basic()), base::UnsupportedFrameError);
}

TEST_CASE("plies turn by their angle: the laminate of the property gives it", "[analysis][frames]")
{
    // element 20 has the plies of PCOMP 2: 0 degrees and 45 degrees; o11 = 100 in each
    Case c = attach({tensor("ShellQuad4Composite", results::ResultKind::Stress, {"o11", "o22", "t12", "t1z", "t2z"},
                            {20, 20}, {1, 2}, {100, 0, 0, 1, 0, 100, 0, 0, 1, 0})});
    const auto material = FrameRotation::rotate(*c.model, *c.attachment, *c.list[0], target(FrameTargetKind::Material));
    REQUIRE(material->meta().components == std::vector<std::string>{"oxx", "oyy", "txy", "txz", "tyz"});
    const double h = std::sqrt(0.5);
    REQUIRE(at(material, 0, 0) == Approx(100.0));
    REQUIRE(at(material, 1, 0) == Approx(50.0));
    REQUIRE(at(material, 1, 1) == Approx(50.0));
    REQUIRE(at(material, 1, 2) == Approx(50.0));
    REQUIRE(at(material, 1, 3) == Approx(h));
    REQUIRE(at(material, 1, 4) == Approx(h));
    // and back to the ply system
    const auto plies = FrameRotation::rotate(*c.model, *c.attachment, *material, target(FrameTargetKind::Ply));
    REQUIRE(plies->meta().components == std::vector<std::string>{"o11", "o22", "t12", "t1z", "t2z"});
    for (std::size_t row = 0; row < 2; ++row)
        for (std::size_t component = 0; component < 5; ++component)
            REQUIRE(at(plies, row, component) ==
                    Approx(c.list[0]->data()->values<double>()[row * 5 + component]).margin(1e-9));
}

TEST_CASE("a solid is in the basic system or in the system of its property", "[analysis][frames]")
{
    Case c = attach({tensor("SolidHexa", results::ResultKind::Stress, {"oxx", "oyy", "ozz", "txy", "tyz", "txz"}, {30},
                            {1}, {10, 20, 30, 4, 5, 6})});
    const StoredFrameInfo stored = StoredFrames::describe(*c.model, *c.attachment, *c.list[0]);
    REQUIRE(stored.kind == results::StoredFrame::Basic);
    // a basic target is the identity, a cylindrical one about system 1 at the centre of the cube (0.5, 0.5, 0.5)
    const auto same = FrameRotation::rotate(*c.model, *c.attachment, *c.list[0], basic());
    REQUIRE(at(same, 0, 0) == Approx(10.0));
    const auto cylindrical =
        FrameRotation::rotate(*c.model, *c.attachment, *c.list[0], target(FrameTargetKind::Cylindrical, 1));
    const double angle = std::numbers::pi / 4.0; // the centre is at 45 degrees about the axis of system 1
    const double c1 = std::cos(angle);
    const double s1 = std::sin(angle);
    // sigma_rr = c^2 sxx + s^2 syy + 2 s c sxy
    REQUIRE(at(cylindrical, 0, 0) == Approx(c1 * c1 * 10 + s1 * s1 * 20 + 2 * s1 * c1 * 4));
    REQUIRE(at(cylindrical, 0, 1) == Approx(s1 * s1 * 10 + c1 * c1 * 20 - 2 * s1 * c1 * 4));
    REQUIRE(at(cylindrical, 0, 2) == Approx(30.0)); // szz does not move
    // a solid is not in an element system
    REQUIRE_THROWS_AS(FrameRotation::rotate(*c.model, *c.attachment, *c.list[0], target(FrameTargetKind::Element)),
                      base::UnsupportedFrameError);
}

TEST_CASE("complex values turn their real and imaginary parts alike, single precision stays single",
          "[analysis][frames]")
{
    for (const bool single : {false, true})
    {
        Case c = attach({nodal({2}, {1, 0, /**/ 0, 2, /**/ 0, 0, 0, 0, 0, 0, 0, 0}, true, single)});
        const auto turned = FrameRotation::rotate(*c.model, *c.attachment, *c.list[0], basic());
        REQUIRE(turned->meta().valueType == (single ? results::ValueType::Complex64 : results::ValueType::Complex128));
        const results::ResultData& data = dataOf(turned);
        const auto value = [&](std::size_t component, std::size_t part)
        {
            return single ? static_cast<double>(data.values<float>()[component * 2 + part])
                          : data.values<double>()[component * 2 + part];
        };
        // t1 = 1 + 0i (radial) and t2 = 0 + 2i (tangential) at a node whose radial direction is y: x = -t2, y = t1
        REQUIRE(value(0, 0) == Approx(0.0).margin(1e-6)); // x
        REQUIRE(value(1, 0) == Approx(1.0));              // y real: from the radial real part
        REQUIRE(value(0, 1) == Approx(-2.0));             // x imaginary: from the tangential imaginary part
        REQUIRE(value(1, 1) == Approx(0.0).margin(1e-6));
    }
}

TEST_CASE("the component along a direction is made from the rotated values", "[analysis][frames]")
{
    Case c = attach({nodal({2}, {1, 0, 0, 0, 0, 1})});
    TargetFrame along;
    along.kind = FrameTargetKind::Direction;
    along.directions = {base::Vec3{0.0, 2.0, 0.0}}; // not a unit vector
    const auto result = FrameRotation::rotate(*c.model, *c.attachment, *c.list[0], along);
    REQUIRE(result->meta().components == std::vector<std::string>{"translation", "rotation"});
    REQUIRE(at(result, 0, 0) == Approx(1.0)); // the radial translation points along y
    REQUIRE(at(result, 0, 1) == Approx(0.0).margin(1e-12));
    along.directions = {base::Vec3{0.0, 0.0, 0.0}};
    REQUIRE(std::isnan(at(FrameRotation::rotate(*c.model, *c.attachment, *c.list[0], along), 0, 0)));
    along.directions = {base::Vec3{1, 0, 0}, base::Vec3{0, 1, 0}}; // two directions for one row
    REQUIRE_THROWS_AS(FrameRotation::rotate(*c.model, *c.attachment, *c.list[0], along), base::UsageError);
}

TEST_CASE("a pair that is not validated raises with the reason of its row", "[analysis][frames]")
{
    Case c = attach({tensor("Rod2", results::ResultKind::Stress, {"axial", "torsion"}, {40}, {1}, {1, 2}),
                     nodal({1}, {1, 2, 3, 4, 5, 6})});
    REQUIRE_THROWS_AS(FrameRotation::rotate(*c.model, *c.attachment, *c.list[0], basic()), base::UnsupportedFrameError);
    try
    {
        (void)FrameRotation::rotate(*c.model, *c.attachment, *c.list[0], basic());
    }
    catch (const base::UnsupportedFrameError& error)
    {
        REQUIRE(std::string(error.what()).find("oneDimensionalStress") != std::string::npos);
    }
    // a grid point has no element system
    try
    {
        (void)FrameRotation::rotate(*c.model, *c.attachment, *c.list[1], target(FrameTargetKind::Element));
        FAIL("a nodal table has no element system");
    }
    catch (const base::UnsupportedFrameError& error)
    {
        REQUIRE(std::string(error.what()).find("no element or material system") != std::string::npos);
    }
}

TEST_CASE("after a change of the structure a frame change refuses until a refresh", "[analysis][frames]")
{
    Case c = attach({nodal({1, 2}, {1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6})});
    REQUIRE_NOTHROW(FrameRotation::rotate(*c.model, *c.attachment, *c.list[0], basic()));
    c.model->noteStructureChange();
    REQUIRE_THROWS_AS(FrameRotation::rotate(*c.model, *c.attachment, *c.list[0], basic()), base::StaleAttachmentError);
    REQUIRE_THROWS_AS(FrameRotation::storedMatrices(*c.model, *c.attachment, *c.list[0]), base::StaleAttachmentError);
    refreshAttachment(*c.model, *c.attachment);
    REQUIRE_NOTHROW(FrameRotation::rotate(*c.model, *c.attachment, *c.list[0], basic()));
}

TEST_CASE("the convention table has a row for every pair and the family of a table follows from what it holds",
          "[analysis][frames]")
{
    const std::vector<ConventionRow>& rows = FrameConventionTable::rows();
    std::size_t validated = 0;
    for (const ConventionRow& row : rows)
    {
        REQUIRE((row.status == ConventionStatus::Refused) == !row.reason.empty());
        REQUIRE((row.status == ConventionStatus::Validated) == !row.evidence.empty());
        if (row.status == ConventionStatus::Validated)
            ++validated;
    }
    REQUIRE(validated == 86);
    REQUIRE(rows.size() ==
            86 + 49 + 8); // 86 validated, 49 refused pairs of the families with a frame change, 8 families without
    results::ResultTableMeta meta;
    meta.kind = results::ResultKind::Stress;
    meta.elementName = "ShellQuad4";
    REQUIRE(FrameConventionTable::familyOf(meta) == FrameFamily::ShellStress);
    meta.elementName = "ShellQuad4Composite";
    REQUIRE(FrameConventionTable::familyOf(meta) == FrameFamily::PlyStress);
    meta.elementName = "SolidHexa";
    meta.kind = results::ResultKind::Strain;
    REQUIRE(FrameConventionTable::familyOf(meta) == FrameFamily::SolidStrain);
    meta.elementName = "ShellQuad4Nonlinear";
    REQUIRE(FrameConventionTable::familyOf(meta) == FrameFamily::Nonlinear);
    meta.kind = results::ResultKind::SinglePointConstrForces;
    REQUIRE(FrameConventionTable::familyOf(meta) == FrameFamily::NodalVector);
    meta.variant = results::ResultVariant::Psd;
    REQUIRE(FrameConventionTable::familyOf(meta) == FrameFamily::RandomOrModal);
    const ConventionRow row =
        FrameConventionTable::find(FrameFamily::ShellForce, results::StoredFrame::Element, FrameTargetKind::Basic);
    REQUIRE(row.status == ConventionStatus::Refused);
    REQUIRE(row.reason.find("unit length") != std::string::npos);
    REQUIRE(FrameConventionTable::find(FrameFamily::ShellForce, results::StoredFrame::Basic, FrameTargetKind::Basic)
                .status == ConventionStatus::Refused);
}

TEST_CASE("turning to a frame and back is the identity for random data, real and complex, single and double",
          "[analysis][frames]")
{
    std::mt19937 generator(7);
    std::uniform_real_distribution<double> value(-10.0, 10.0);
    for (const bool complexValues : {false, true})
        for (const bool single : {false, true})
        {
            std::vector<double> values;
            const std::size_t parts = complexValues ? 2 : 1;
            for (std::size_t index = 0; index < 3 * 6 * parts; ++index)
                values.push_back(value(generator));
            Case c = attach({nodal({1, 2, 3}, values, complexValues, single)});
            const double tolerance = single ? 5e-5 : 1e-12;
            for (const FrameTargetKind kind : {FrameTargetKind::Basic, FrameTargetKind::Cylindrical,
                                               FrameTargetKind::Spherical, FrameTargetKind::Coordinate})
            {
                const auto there = FrameRotation::rotate(*c.model, *c.attachment, *c.list[0],
                                                         target(kind, kind == FrameTargetKind::Basic ? 0 : 1));
                const auto back =
                    FrameRotation::rotate(*c.model, *c.attachment, *there, target(FrameTargetKind::NodeOutput));
                const results::ResultData& original = *c.list[0]->data();
                const results::ResultData& returned = dataOf(back);
                const std::size_t count = original.valueCount() * parts;
                for (std::size_t index = 0; index < count; ++index)
                {
                    const double a = single ? static_cast<double>(original.values<float>()[index])
                                            : original.values<double>()[index];
                    const double b = single ? static_cast<double>(returned.values<float>()[index])
                                            : returned.values<double>()[index];
                    REQUIRE(std::fabs(a - b) < tolerance * 10.0);
                }
            }
        }
}
