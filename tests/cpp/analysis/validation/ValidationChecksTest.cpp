#include "oor/analysis/validation/engine/ValidationEngine.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "support/ModelRecord.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <chrono>
#include <cstdio>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

using oor::testing::modelRecord;

using namespace oor;
using namespace oor::analysis;
using oor::testing::readModelText;

namespace
{

const char* const MATERIALS = "IsoMat;1;2.1E+5;;0.3;7.8E-9\nShellProp;1;1;0.1;1\n";

std::string gridsOf(int count)
{
    std::string text;
    for (int id = 1; id <= count; ++id)
        text += modelRecord("Node", id, "", static_cast<double>(id), static_cast<double>(id % 3), "0.");
    return text;
}

std::set<std::string> findingsOf(const std::string& bulk, std::vector<std::string> select = {})
{
    const model::Model model = readModelText(std::string(MATERIALS) + bulk);
    ValidationOptions options;
    options.select = std::move(select);
    options.minSeverity = base::Severity::Info;
    std::set<std::string> codes;
    for (const ValidationFinding& finding : ValidationEngine::run(model, options).findings)
        codes.insert(finding.code);
    return codes;
}

} // namespace

TEST_CASE("numbered user elements resolve attributes through their canonical names", "[validation][compatibility]")
{
    for (int number = 1; number <= 9; ++number)
    {
        const std::string root = "UserDefined" + std::to_string(number);
        const std::string records = gridsOf(3) + root + "Prop;200;1;1.5;;3\n" +
                                    root + ";100;200;1;2;3;;2.5;7\n";
        REQUIRE(findingsOf(records, {"compat.dummy-entry"}) == std::set<std::string>{"compat.dummy-entry"});
        REQUIRE(findingsOf(records + root + "Attributes;6;2;3;3;USER\n", {"compat.dummy-entry"}).empty());
    }
}

TEST_CASE("a rigid loop is a loop of degrees of freedom, not of grid points", "[validation][constraints]")
{
    const std::string grids = gridsOf(8);
    // Two RBE2 that make each other's translations depend on one another: a loop.
    REQUIRE(findingsOf(grids + "RigidBodyStar;60;1;123456;2\nRigidBodyStar;61;2;123456;1\n",
                       {"constraint.rigid-loop"}) == std::set<std::string>{"constraint.rigid-loop"});
    // An RBE1 whose independent and dependent degrees of freedom are at the same two grid points but are not the same
    // degrees of freedom: no loop.
    REQUIRE(
        findingsOf(grids + "RigidBodyMulti;70;2;1;234;2;123;DependentDofs;2;1;156;2;456\n", {"constraint.rigid-loop"})
            .empty());
    // An RBE3 with UM: the dependent degrees of freedom are chosen among the ones of the weighted grid points.
    REQUIRE(findingsOf(grids + "RigidInterpolation;80;4;123;1;1.;123;6;1;2;3;DependentDofs;1;123\n",
                       {"constraint.rigid-loop"})
                .empty());
    // A chain is not a loop (and is reported only on request).
    const std::string chain = grids + "RigidBodyStar;60;1;123456;2\nRigidBodyStar;61;2;123456;3\n";
    REQUIRE(findingsOf(chain, {"constraint.rigid-loop"}).empty());
    REQUIRE(findingsOf(chain, {"constraint.rigid-chain"}) == std::set<std::string>{"constraint.rigid-chain"});
}

TEST_CASE("the constraints are judged on the sets the Case Control selects", "[validation][constraints]")
{
    const std::string base = std::string(
                                 "[executive]\n"
                                 "SolutionSequence;101\n"
                                 "[case]\n"
                                 "SubcaseStart;1\n"
                                 "SinglePointConstrSelection;10\n"
                                 "StaticLoadSelection;1\n"
                                 "DisplacementOutput;All\n"
                                 "SubcaseStart;2\n"
                                 "SinglePointConstrSelection;20\n"
                                 "StaticLoadSelection;1\n"
                                 "DisplacementOutput;All\n"
                                 "[model]\n") +
                             MATERIALS + gridsOf(4) + "RigidBodyStar;60;1;123456;2\nPointForceLoad;1;3;;1.;0.;0.;1.\n";
    // SPC 10 fixes grid point 3, SPC 20 fixes the dependent grid point 2: only the second subcase has the clash.
    const model::Model clash =
        readModelText(base + "SinglePointConstrList;10;123456;3\nSinglePointConstrList;20;123456;2\n");
    ValidationOptions options;
    options.select = {"constraint.spc-on-rigid-dependent"};
    REQUIRE(ValidationEngine::run(clash, options).findings.size() == 1);
    const model::Model fine =
        readModelText(base + "SinglePointConstrList;10;123456;3\nSinglePointConstrList;20;123456;4\n");
    REQUIRE(ValidationEngine::run(fine, options).findings.empty());
    // SPCADD combines sets: SPC = 20 selects set 21 through it.
    const model::Model added = readModelText(
        base + "SinglePointConstrList;10;123456;3\nSinglePointConstrUnion;20;21\nSinglePointConstrList;21;123456;2\n");
    REQUIRE(ValidationEngine::run(added, options).findings.size() == 1);
}

TEST_CASE("coincident grid points are found by a spatial hash, in the basic system", "[validation][geometry]")
{
    // A lattice of 20 000 points with a copy of every thousandth one.
    std::string bulk;
    int next = 1;
    for (int ix = 0; ix < 200; ++ix)
    {
        for (int iy = 0; iy < 100; ++iy)
            bulk += modelRecord("Node", next++, "", ix * 0.5, iy * 0.25, "0.");
    }
    int copies = 0;
    for (int id = 1; id < next; id += 1000)
    {
        const int ix = (id - 1) / 100;
        const int iy = (id - 1) % 100;
        bulk += modelRecord("Node", 100000 + id, "", ix * 0.5, iy * 0.25, "1.E-7");
        ++copies;
    }
    const model::Model model = readModelText(std::string(MATERIALS) + bulk);
    ValidationOptions options;
    options.select = {"geometry.coincident"};
    const auto start = std::chrono::steady_clock::now();
    const ValidationReport report = ValidationEngine::run(model, options);
    const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
    REQUIRE(static_cast<int>(report.findings.size()) == copies);
    for (const ValidationFinding& finding : report.findings)
        REQUIRE(finding.cardId >= 100000);
    REQUIRE(elapsed.count() < 0.5);
}

TEST_CASE("coincident grid points are found across the faces of their cubes as a look at every pair finds them",
          "[validation][geometry]")
{
    // 2 000 points in a box of 0.02 with a tolerance of 0.001: cubes of 0.002, a point a mean 0.0016 from the next,
    // many pairs within the tolerance on either side of a face, an edge or a corner of their cubes, at negative
    // coordinates too.
    std::mt19937 random(7);
    std::uniform_real_distribution<double> coordinate(-0.01, 0.01);
    model::Model model;
    std::vector<std::array<double, 3>> xyz(2000);
    for (std::size_t index = 0; index < xyz.size(); ++index)
    {
        for (double& value : xyz[index])
            value = coordinate(random);
        model::NodeCard card;
        card.id = static_cast<base::EntityId>(xyz.size() - index); // IDs against the order of the rows
        card.xyz = base::Vec3{xyz[index][0], xyz[index][1], xyz[index][2]};
        model.add<model::NodeTable>(card);
    }
    for (const double tolerance : {0.0004, 0.001, 0.003})
    {
        // Each point and the lowest ID within the tolerance of it, lower than its own.
        std::map<base::EntityId, base::EntityId> expected;
        for (std::size_t index = 0; index < xyz.size(); ++index)
        {
            const auto mine = static_cast<base::EntityId>(xyz.size() - index);
            for (std::size_t other = 0; other < xyz.size(); ++other)
            {
                const auto id = static_cast<base::EntityId>(xyz.size() - other);
                const double dx = xyz[index][0] - xyz[other][0];
                const double dy = xyz[index][1] - xyz[other][1];
                const double dz = xyz[index][2] - xyz[other][2];
                if (id < mine && dx * dx + dy * dy + dz * dz <= tolerance * tolerance &&
                    (!expected.contains(mine) || id < expected[mine]))
                    expected[mine] = id;
            }
        }
        REQUIRE(expected.size() > 20);
        ValidationOptions options;
        options.select = {"geometry.coincident"};
        options.coincidentTolerance = tolerance;
        std::map<base::EntityId, base::EntityId> found;
        for (const ValidationFinding& finding : ValidationEngine::run(model, options).findings)
        {
            const std::string prefix = "coincides with grid point ";
            REQUIRE(finding.message.starts_with(prefix));
            found[finding.cardId] = static_cast<base::EntityId>(std::stol(finding.message.substr(prefix.size())));
        }
        REQUIRE(found == expected);
    }
}

TEST_CASE("the checks leave a model with parts and Case Control alone", "[validation]")
{
    const std::string text =
        std::string("[executive]\nSolutionSequence;101\n[case]\nSubcaseStart;1\nDisplacementOutput;All\n[model]\n") +
        MATERIALS + gridsOf(4) + "ShellQuad4;10;1;1;2;3;4\n[model;SuperelementPart;1]\nNode;1;;0.;0.;0.\n";
    const model::Model model = readModelText(text);
    const std::size_t cards = model.cardCount();
    ValidationOptions options;
    options.select = {"all"};
    options.minSeverity = base::Severity::Info;
    (void)ValidationEngine::run(model, options);
    REQUIRE(model.cardCount() == cards);
    REQUIRE(model.parts().size() == 1);
}
