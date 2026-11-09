// Components, interface nodes, boundaries, load case groups and revision stamps, on small decks whose
// answers are worked out by hand: two plates joined by a rigid element, a chain of rods between two squares, loads on
// different nodes.

#include "oor/analysis/components/ComponentBoundary.hpp"
#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/analysis/components/LoadCaseGroups.hpp"
#include "oor/analysis/components/RevisionStamp.hpp"
#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "support/ModelRecord.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <map>
#include <string>
#include <vector>

using oor::testing::modelRecord;

using namespace oor;
using oor::testing::readModelText;
using oor::testing::writeModelText;

namespace
{

// The cards of a plate of nx by ny unit squares (the first node and element numbers are given), at x = x0, in the plane
// z = 0.
std::string plate(int nx, int ny, int firstNode, int firstElement, int pid, double x0)
{
    std::string text;
    for (int j = 0; j <= ny; ++j)
    {
        for (int i = 0; i <= nx; ++i)
            text += modelRecord("Node", firstNode + i + j * (nx + 1), "", x0 + i, static_cast<double>(j), "0.");
    }
    int element = firstElement;
    for (int j = 0; j < ny; ++j)
    {
        for (int i = 0; i < nx; ++i)
        {
            const int n = firstNode + i + j * (nx + 1);
            text += modelRecord("ShellQuad4", element++, pid, n, n + 1, n + nx + 2, n + nx + 1);
        }
    }
    return text;
}

const char* const MATERIALS = "IsoMat;1;2.E+5;7.5E+4;0.3;0.001\nShellProp;1;1;1.\nShellProp;2;1;2.\n";
const char* const CASES =
    "[executive]\n"
    "SolutionSequence;101\n"
    "[case]\n"
    "SubcaseStart;1\n"
    "StaticLoadSelection;10\n"
    "SubcaseStart;2\n"
    "StaticLoadSelection;11\n"
    "SubcaseStart;3\n"
    "StaticLoadSelection;10\n"
    "SubcaseStart;4\n"
    "StaticLoadSelection;99\n"
    "[model]\n";

// Plate A (2 by 2, nodes 1 to 9, elements 1 to 4, PSHELL 1), plate B (1 by 2, nodes 11 to 16, elements 5 and 6, PSHELL
// 2), an RBE2 from node 9 to node 11, forces: set 10 on node 5 (A) and 16 (B), set 11 on node 6 (A).
std::string twoPlates(const std::string& loads =
                          "PointForceLoad;10;5;0;100.;0.;0.;1.\nPointForceLoad;10;16;0;50.;0.;1.;0.\nPointForceLoad;11;"
                          "6;0;10.;0.;0.;1.\n")
{
    return std::string(CASES) + MATERIALS + plate(2, 2, 1, 1, 1, 0.0) + plate(1, 2, 11, 5, 2, 10.0) +
           "RigidBodyStar;100;9;123456;11\n" + loads;
}

} // namespace

TEST_CASE("components are numbered by size and a rigid element is a connection", "[analysis][components]")
{
    const model::Model model = readModelText(twoPlates());
    const auto components = analysis::ComponentModel::of(model);
    REQUIRE(components->componentCount() == 2);
    CHECK(components->component(1).elementCount == 4);
    CHECK(components->component(1).nodeCount == 9);
    CHECK(components->component(2).elementCount == 2);
    CHECK(components->component(2).nodeCount == 6);
    CHECK(components->component(1).smallestElementId == 1);
    CHECK(components->componentOfNode(5) == 1);
    CHECK(components->componentOfNode(16) == 2);
    CHECK(components->componentOfNode(99) == 0);
    REQUIRE(components->connections().size() == 1);
    CHECK(components->connections().front().kind == analysis::ConnectionKind::Rigid);
    CHECK(components->connections().front().id == 100);
    // the interface node of each plate is the one the rigid element uses
    REQUIRE(components->interfaceNodes(1).size() == 1);
    CHECK(components->interfaceNodes(1)[0].node == 9);
    REQUIRE(components->interfaceNodes(2).size() == 1);
    CHECK(components->interfaceNodes(2)[0].node == 11);
}

TEST_CASE("a chain of rods between two squares is a connection and does not join them", "[analysis][components]")
{
    const std::string deck = std::string("IsoMat;1;2.E+5;7.5E+4;0.3;0.001\nShellProp;1;1;1.\nRodProp;5;1;0.25\n") +
                             plate(1, 1, 1, 1, 1, 0.0) + plate(1, 1, 11, 2, 1, 5.0) +
                             "Node;30;;3.;0.;0.\nRod2;10;5;2;30\nRod2;11;5;30;11\n";
    const model::Model model = readModelText(deck);
    const auto components = analysis::ComponentModel::of(model);
    CHECK(components->componentCount() == 2);
    REQUIRE(components->connections().size() == 2);
    CHECK(components->connections()[0].kind == analysis::ConnectionKind::Line);
    // each rod is listed with the nodes of the components the chain reaches: node 2 of the first square and node 11 of
    // the second
    REQUIRE(components->interfaceNodes(1).size() == 1);
    CHECK(components->interfaceNodes(1)[0].node == 2);
    REQUIRE(components->interfaceNodes(2).size() == 1);
    CHECK(components->interfaceNodes(2)[0].node == 11);
    // a rod that goes to a free node and nowhere else belongs to the square
    const model::Model alone =
        readModelText(std::string("IsoMat;1;2.E+5;7.5E+4;0.3;0.001\nShellProp;1;1;1.\nRodProp;5;1;0.25\n") +
                      plate(1, 1, 1, 1, 1, 0.0) + "Node;30;;3.;0.;0.\nRod2;10;5;2;30\n");
    const auto owned = analysis::ComponentModel::of(alone);
    REQUIRE(owned->componentCount() == 1);
    CHECK(owned->component(1).lineCount == 1);
    CHECK(owned->connections().empty());
}

TEST_CASE("components are the same for one and for twelve threads", "[analysis][components]")
{
    const model::Model model = readModelText(twoPlates());
    base::ThreadPool many(11);
    const analysis::ComponentModel one = analysis::ComponentModel::build(model, nullptr);
    const analysis::ComponentModel twelve = analysis::ComponentModel::build(model, &many);
    REQUIRE(one.componentCount() == twelve.componentCount());
    for (std::int32_t id = 1; id <= static_cast<std::int32_t>(one.componentCount()); ++id)
    {
        CHECK(one.component(id).elementCount == twelve.component(id).elementCount);
        CHECK(std::vector<base::EntityId>(one.nodesOf(id).begin(), one.nodesOf(id).end()) ==
              std::vector<base::EntityId>(twelve.nodesOf(id).begin(), twelve.nodesOf(id).end()));
    }
    const auto first = analysis::RevisionStamp::compute(model, one, nullptr);
    const auto second = analysis::RevisionStamp::compute(model, twelve, &many);
    CHECK(first == second);
}

TEST_CASE("load cases are grouped by the nodes they load on each component", "[analysis][components]")
{
    const model::Model model = readModelText(twoPlates());
    const auto components = analysis::ComponentModel::of(model);
    const analysis::LoadCaseGroups groups = analysis::LoadCaseGroups::compute(model, *components);
    // component 1: cases 1 and 3 load node 5 (group 1), case 2 node 6 (group 2), case 4 nothing (group 0)
    // component 2: cases 1 and 3 load node 16 (group 1), cases 2 and 4 nothing (group 0)
    std::vector<std::array<std::int64_t, 3>> rows;
    for (const analysis::LoadCaseRow& row : groups.rows())
        rows.push_back({row.component, row.group, row.subcase});
    const std::vector<std::array<std::int64_t, 3>> expected = {{1, 0, 4}, {1, 1, 1}, {1, 1, 3}, {1, 2, 2},
                                                               {2, 0, 2}, {2, 0, 4}, {2, 1, 1}, {2, 1, 3}};
    CHECK(rows == expected);
    REQUIRE(groups.find(1, 1) != nullptr);
    CHECK(groups.find(1, 1)->nodes == std::vector<base::EntityId>{5});
    CHECK(groups.find(1, 2)->nodes == std::vector<base::EntityId>{6});
    CHECK(groups.find(2, 1)->nodes == std::vector<base::EntityId>{16});
    CHECK(groups.find(1, 0)->nodes.empty());
    for (const analysis::LoadCaseRow& row : groups.rows())
        CHECK_FALSE(row.isDistributed);
}

TEST_CASE("the load revision stamp follows the loads of the component in that case only", "[analysis][components]")
{
    const auto stampsOf = [](const std::string& loads)
    {
        const model::Model model = readModelText(twoPlates(loads));
        const auto components = analysis::ComponentModel::of(model);
        const analysis::LoadCaseGroups groups = analysis::LoadCaseGroups::compute(model, *components);
        std::map<std::pair<std::int32_t, std::int64_t>, std::uint64_t> stamps;
        for (const analysis::LoadCaseRow& row : groups.rows())
            stamps[{row.component, row.subcase}] = row.loadRevision;
        return stamps;
    };
    const auto base = stampsOf(
        "PointForceLoad;10;5;0;100.;0.;0.;1.\nPointForceLoad;10;16;0;50.;0.;1.;0.\nPointForceLoad;11;6;0;10.;0.;0.;1."
        "\n");
    // the force of set 10 on plate A is changed: the stamps of plate A for the cases 1 and 3 change, nothing else does
    const auto changed = stampsOf(
        "PointForceLoad;10;5;0;120.;0.;0.;1.\nPointForceLoad;10;16;0;50.;0.;1.;0.\nPointForceLoad;11;6;0;10.;0.;0.;1."
        "\n");
    for (const auto& [key, stamp] : base)
    {
        const bool expectedChange = key.first == 1 && (key.second == 1 || key.second == 3);
        CHECK((changed.at(key) != stamp) == expectedChange);
    }
    CHECK(base.at({1, 1}) == base.at({1, 3}));
    CHECK(base.at({1, 1}) != base.at({1, 2}));
    CHECK(base.at({1, 4}) == base.at({2, 4}));
    CHECK(base.at({1, 4}) == base.at({2, 2}));
}

TEST_CASE("boundaries are the interface nodes, plus the retained nodes, plus the load nodes of each group",
          "[analysis][components]")
{
    const model::Model model = readModelText(twoPlates());
    const auto components = analysis::ComponentModel::of(model);
    const analysis::LoadCaseGroups groups = analysis::LoadCaseGroups::compute(model, *components);

    const auto nodesOf = [](const analysis::Boundary& boundary)
    {
        std::vector<base::EntityId> nodes;
        for (const analysis::BoundaryNode& node : boundary.nodes)
            nodes.push_back(node.node);
        return nodes;
    };

    const auto plain = analysis::ComponentBoundaries::compute(model, *components, nullptr, {});
    REQUIRE(plain.size() == 2);
    CHECK(nodesOf(plain[0]) == std::vector<base::EntityId>{9});
    CHECK(nodesOf(plain[1]) == std::vector<base::EntityId>{11});
    CHECK_FALSE(plain[0].loadGroup.has_value());
    CHECK(plain[0].nodes[0].isInterface);
    CHECK_FALSE(plain[0].nodes[0].isRetained);

    analysis::BoundaryOptions retained;
    retained.retain = analysis::RetainMode::Nodes;
    retained.retainedNodes = {5, 1, 99}; // 99 is on no component
    const auto withNodes = analysis::ComponentBoundaries::compute(model, *components, nullptr, retained);
    REQUIRE(withNodes.size() == 2);
    CHECK(nodesOf(withNodes[0]) == std::vector<base::EntityId>{1, 5, 9});
    CHECK(nodesOf(withNodes[1]) == std::vector<base::EntityId>{11});

    analysis::BoundaryOptions loadNodes;
    loadNodes.retain = analysis::RetainMode::LoadNodes;
    const auto perGroup = analysis::ComponentBoundaries::compute(model, *components, &groups, loadNodes);
    // plate A: groups 0 (the interface), 1 (+ node 5), 2 (+ node 6); plate B: groups 0 and 1 (+ node 16)
    REQUIRE(perGroup.size() == 5);
    std::vector<std::vector<base::EntityId>> found;
    for (const analysis::Boundary& boundary : perGroup)
        found.push_back(nodesOf(boundary));
    const std::vector<std::vector<base::EntityId>> expected = {{9}, {5, 9}, {6, 9}, {11}, {11, 16}};
    CHECK(found == expected);
}

TEST_CASE("the boundary of a component does not depend on what its neighbour holds", "[analysis][components]")
{
    const model::Model reference = readModelText(twoPlates());
    // the other neighbour: a plate of nine squares with other node numbers, tied to node 9 by an RBE2 on its node 21
    const model::Model other = readModelText(
        std::string(CASES) + MATERIALS + plate(2, 2, 1, 1, 1, 0.0) + plate(3, 3, 21, 50, 2, 10.0) +
        "RigidBodyStar;100;9;123456;21\nPointForceLoad;10;5;0;100.;0.;0.;1.\nPointForceLoad;11;6;0;10.;0.;0.;1.\n");
    const auto left = analysis::ComponentModel::of(reference);
    const auto right = analysis::ComponentModel::of(other);
    // plate A is component 1 in the first model and component 2 in the other (nine squares are more than four)
    REQUIRE(right->componentCount() == 2);
    CHECK(right->component(2).elementCount == 4);
    const auto leftBoundary = analysis::ComponentBoundaries::compute(reference, *left, nullptr, {});
    const auto rightBoundary = analysis::ComponentBoundaries::compute(other, *right, nullptr, {});
    REQUIRE(leftBoundary[0].nodes.size() == 1);
    REQUIRE(rightBoundary[1].nodes.size() == 1);
    CHECK(leftBoundary[0].nodes[0].node == rightBoundary[1].nodes[0].node);
    const auto leftStamps = analysis::RevisionStamp::compute(reference, *left, nullptr);
    const auto rightStamps = analysis::RevisionStamp::compute(other, *right, nullptr);
    CHECK(leftStamps[0] == rightStamps[1]);
    CHECK(leftStamps[1] != rightStamps[0]);
}

TEST_CASE("the revision stamp follows the content of the component and what it depends on", "[analysis][components]")
{
    const auto stamps = [](const std::string& materials, const std::string& cards)
    {
        const model::Model model = readModelText(materials + cards);
        const auto components = analysis::ComponentModel::of(model);
        return analysis::RevisionStamp::compute(model, *components, nullptr);
    };
    const std::string a = plate(2, 2, 1, 1, 1, 0.0);
    const std::string b = plate(1, 2, 11, 5, 2, 10.0);
    const std::string steel = "IsoMat;1;2.E+5;7.5E+4;0.3;0.001\n";
    const auto base = stamps(steel + "ShellProp;1;1;1.\nShellProp;2;1;2.\n", a + b);
    REQUIRE(base.size() == 2);
    CHECK(base[0] != base[1]);
    // the property of B changes: only B's stamp changes
    const auto thicker = stamps(steel + "ShellProp;1;1;1.\nShellProp;2;1;3.\n", a + b);
    CHECK(thicker[0] == base[0]);
    CHECK(thicker[1] != base[1]);
    // the material both use changes: both change
    const auto denser =
        stamps("IsoMat;1;2.E+5;7.5E+4;0.3;0.002\n" + std::string("ShellProp;1;1;1.\nShellProp;2;1;2.\n"), a + b);
    CHECK(denser[0] != base[0]);
    CHECK(denser[1] != base[1]);
    // a node of A moves: only A changes
    std::string moved = a;
    moved.replace(moved.find("Node;5;;1.;1.;0."), 16, "Node;5;;1.;1.;0.5");
    const auto bent = stamps(steel + "ShellProp;1;1;1.\nShellProp;2;1;2.\n", moved + b);
    CHECK(bent[0] != base[0]);
    CHECK(bent[1] == base[1]);
    // a rigid element or a load changes nothing in a stamp
    const auto joined = stamps(steel + "ShellProp;1;1;1.\nShellProp;2;1;2.\n",
                               a + b + "RigidBodyStar;100;9;123456;11\nPointForceLoad;10;5;0;100.;0.;0.;1.\n");
    CHECK(joined == base);
}

TEST_CASE("the revision stamp is the same after the model is written and read back", "[analysis][components]")
{
    const model::Model original = readModelText(twoPlates());
    const auto first = analysis::RevisionStamp::of(original);
    const model::Model again = readModelText(oor::testing::writeModelText(original));
    const auto second = analysis::RevisionStamp::of(again);
    REQUIRE(first->size() == second->size());
    // (a MAT1 whose G is blank is written with the G it derives: the deck here gives G, so the content is the same)
    CHECK(*first == *second);
    const model::Model copy = original;
    CHECK(*analysis::RevisionStamp::of(copy) == *first);
}

TEST_CASE("a column of defaults hashes and writes as a filled column does", "[analysis][components]")
{
    // The CD column of grid points that all have CD 0 holds no buffer; once a view materialises it, the
    // card hashes of the revision stamps and the written deck are the same.
    const std::string deck = "[model]\nNode;1;;0.;0.;0.\nNode;2;;1.;0.;0.\n";
    model::Model lazy = readModelText(deck);
    model::Model filled = readModelText(deck);
    REQUIRE_FALSE(lazy.table<model::NodeTable>().displacementFrameIdColumn().isMaterialised());
    (void)filled.table<model::NodeTable>().displacementFrameIdColumn().view();
    REQUIRE(filled.table<model::NodeTable>().displacementFrameIdColumn().isMaterialised());
    for (std::uint32_t row = 0; row < 2; ++row)
    {
        const model::CardLocation card{model::typeIndexOf<model::NodeTable>(), row};
        CHECK(analysis::RevisionStamp::cardHash(lazy, card) == analysis::RevisionStamp::cardHash(filled, card));
    }
    CHECK(writeModelText(lazy) == writeModelText(filled));
}
