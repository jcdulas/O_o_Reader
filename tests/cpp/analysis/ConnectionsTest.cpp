// Fastener connections, connection sharing, spiders, clusters, groups and constraint stamps, on small decks whose
// answers are worked out by hand.

#include "oor/analysis/components/ComponentCache.hpp"
#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/analysis/components/ConstraintStamps.hpp"
#include "oor/analysis/components/EntityDofs.hpp"
#include "oor/analysis/components/GroupSet.hpp"
#include "oor/analysis/components/NodeEntityIndex.hpp"
#include "oor/analysis/components/UnitMembership.hpp"
#include "oor/analysis/connections/ConnectionClusters.hpp"
#include "oor/analysis/connections/ConnectionSharing.hpp"
#include "oor/analysis/connections/FastenerPiercings.hpp"
#include "oor/analysis/connections/Spiders.hpp"
#include "support/ModelRecord.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace oor;
using oor::testing::readModelText;

namespace
{

// Two plates of 2 by 2 unit squares, at z = 0 (nodes 1 to 9, elements 1 to 4) and z = 0.5 (nodes 101 to 109, elements
// 101 to 104), and a CWELD ELPAT between elements 1 and 101 located by GS 600 at (0.5, 0.4, 0.3).
std::string weldedPlates(const std::string& extra = "")
{
    std::string text = "IsoMat;1;2.E+5;7.5E+4;0.3;0.001\nShellProp;1;1;0.1\nShellProp;2;1;0.1\n";
    for (const auto& [first, pid, z] : {std::tuple{1, 1, 0.0}, std::tuple{101, 2, 0.5}})
    {
        for (int j = 0; j <= 2; ++j)
        {
            for (int i = 0; i <= 2; ++i)
                text += oor::testing::modelRecord("Node", first + i + 3 * j, "", static_cast<double>(i),
                                                  static_cast<double>(j), z);
        }
        for (int j = 0; j < 2; ++j)
        {
            for (int i = 0; i < 2; ++i)
            {
                const int n = first + i + 3 * j;
                text += oor::testing::modelRecord("ShellQuad4", first + i + 2 * j, pid, n, n + 1, n + 4, n + 3);
            }
        }
    }
    return text + "Node;600;;0.5;0.4;0.3\nWeldProp;501;1;0.2\nWeld;500;501;600;ElemPatch;;;;1;101\n" + extra;
}

} // namespace

TEST_CASE("a weld acts on the grid points of the elements it pierces, not on GS", "[analysis][connections]")
{
    // weld 520 is 0.05 from element 2: it still acts on element 1 alone, the element it pierces
    const model::Model model =
        readModelText(weldedPlates("Node;620;;0.95;0.4;0.3\nWeld;520;501;620;ElemPatch;;;;1;101\n"));
    const auto piercings = analysis::FastenerPiercings::of(model);
    REQUIRE(piercings->fasteners().size() == 2);
    const analysis::PiercedFastener& weld = piercings->fasteners().front();
    CHECK(weld.eid == 500);
    CHECK(weld.form == analysis::FastenerForm::Elpat);
    CHECK(weld.ownNodes.empty());
    const auto sides = piercings->sidesOf(weld);
    REQUIRE(sides.size() == 2);
    CHECK(sides[0].elementId == 1);
    CHECK(sides[1].elementId == 101);
    CHECK(piercings->connectionNodes(weld) == std::vector<base::EntityId>{1, 2, 4, 5, 101, 102, 104, 105});
    const analysis::PiercedFastener& nearEdge = piercings->fasteners().back();
    CHECK(nearEdge.eid == 520);
    CHECK(piercings->find(nearEdge.card) == &nearEdge);
    CHECK(piercings->connectionNodes(nearEdge) == std::vector<base::EntityId>{1, 2, 4, 5, 101, 102, 104, 105});

    // the weld ties the translations of the grid points it acts on
    analysis::ComponentCache& cache = analysis::ComponentCache::of(model);
    const auto entities = cache.entities(model);
    const auto entity = entities->indexOf(weld.card);
    REQUIRE(entity.has_value());
    CHECK(analysis::EntityDofs::text(analysis::EntityDofs::at(model, entities->entity(*entity), 1, piercings.get())) ==
          "123");
    const auto components = analysis::ComponentModel::of(model);
    REQUIRE(components->interfaceNodes(1).size() == 4);
    CHECK(components->interfaceNodes(1)[0].node == 1);
}

TEST_CASE("an ALIGN weld joins its GA and GB, a point-to-patch weld its GS and the element it pierces",
          "[analysis][connections]")
{
    const model::Model model = readModelText(weldedPlates(
        "Weld;530;501;;AlignedNodes;3;103\nNode;640;;1.5;1.5;0.2\nWeld;540;501;640;ElemIdPatch;;;;4\n"));
    const auto piercings = analysis::FastenerPiercings::of(model);
    REQUIRE(piercings->fasteners().size() == 3);
    const analysis::PiercedFastener& align = piercings->fasteners()[1];
    CHECK(align.eid == 530);
    CHECK(align.sideCount == 0);
    CHECK(align.ownNodes == std::vector<base::EntityId>{3, 103});
    CHECK(piercings->connectionNodes(align) == std::vector<base::EntityId>{3, 103});
    const analysis::PiercedFastener& pointToPatch = piercings->fasteners()[2];
    CHECK(pointToPatch.eid == 540);
    REQUIRE(pointToPatch.sideCount == 1);
    CHECK(piercings->sidesOf(pointToPatch)[0].elementId == 4);
    CHECK(pointToPatch.ownNodes == std::vector<base::EntityId>{640});
    CHECK(piercings->connectionNodes(pointToPatch) == std::vector<base::EntityId>{5, 6, 8, 9, 640});

    // a weld acts on its own grid points with all six components
    analysis::ComponentCache& cache = analysis::ComponentCache::of(model);
    const auto entities = cache.entities(model);
    const auto entity = entities->indexOf(align.card);
    REQUIRE(entity.has_value());
    CHECK(analysis::EntityDofs::text(analysis::EntityDofs::at(model, entities->entity(*entity), 3, piercings.get())) ==
          "123456");
    const auto nodes = entities->nodesOf(*entity);
    CHECK(std::vector<base::EntityId>(nodes.begin(), nodes.end()) == std::vector<base::EntityId>{3, 103});
}

TEST_CASE("connection sharing, spiders and clusters on a plate", "[analysis][connections]")
{
    // two RBE3 spiders on elements 2 and 4 of the lower plate share node 5; an RBE2 hangs a free point from node 9
    const model::Model model =
        readModelText(weldedPlates("Node;700;;1.5;0.5;1.\n"
                                   "RigidInterpolation;701;700;123456;1;1.;123;4;2;3;6;5\n"
                                   "Node;710;;1.5;1.5;1.\n"
                                   "RigidInterpolation;711;710;123456;1;1.;123;4;5;6;9;8\n"
                                   "Node;720;;3.;3.;0.\n"
                                   "RigidBodyStar;721;9;123456;720\n"));
    const auto components = analysis::ComponentModel::of(model);
    const analysis::ConnectionSharing sharing = analysis::ConnectionSharing::build(*components);
    // node 5: the weld (it pierces element 1), both spiders
    CHECK(sharing.connectionsAt(5).size() == 3);
    CHECK(sharing.connectionsAt(3).size() == 1);
    const analysis::ComponentMembership units(*components);
    const std::vector<analysis::Spider> spiders = analysis::Spiders::find(model, *components, sharing, units);
    REQUIRE(spiders.size() == 3);
    CHECK(spiders[0].eid == 701);
    CHECK(spiders[0].isSpider);
    CHECK_FALSE(spiders[0].isEligible);
    CHECK(spiders[0].blockingNodes == std::vector<base::EntityId>{2, 5, 6});
    CHECK(spiders[2].eid == 721); // GN 9 on the plate, the leg 720 on no component
    CHECK(spiders[2].reason == analysis::SpiderReason::LegsOnSeveralUnits);
    const analysis::ConnectionClusters clusters = analysis::ConnectionClusters::build(model, *components);
    REQUIRE(clusters.clusters().size() == 3);
    CHECK(clusters.connectionOnlyNodes() == std::vector<base::EntityId>{700, 710, 720});
    CHECK(clusters.clusters()[0].isSingleComponent());
}

TEST_CASE("a group of two components absorbs the weld between them", "[analysis][connections]")
{
    const model::Model model =
        readModelText(weldedPlates("Node;800;;3.;0.;0.\n"
                                   "SpringDofInline;801;10.;3;1;800;1\n"
                                   "Node;810;;3.;0.;0.5\n"
                                   "SpringDofInline;811;10.;103;1;810;1\n"
                                   "SpringDofInline;812;10.;800;1;810;1\n"));
    analysis::ComponentCache& cache = analysis::ComponentCache::of(model);
    const auto entities = cache.entities(model);
    const auto components = cache.components(model);
    const analysis::NodeEntityIndex index(*entities);
    const analysis::ConnectionClusters clusters = analysis::ConnectionClusters::build(model, *components);
    std::vector<analysis::GroupDefinition> definitions(2);
    definitions[0] = {"lower", analysis::GroupKind::Components, {1}};
    definitions[1] = {"both", analysis::GroupKind::Components, {1, 2}};
    const analysis::GroupSet groups = analysis::GroupSet::resolve(*entities, index, *components, clusters, definitions);
    // the lower plate: the weld acts on 1, 2, 4, 5 and the spring chain on 3
    std::vector<base::EntityId> lower;
    for (const analysis::BoundaryNode& node : groups.interfaceOf(1))
        lower.push_back(node.node);
    CHECK(lower == std::vector<base::EntityId>{1, 2, 3, 4, 5});
    // both plates hold the weld and the chain of springs through 800 and 810: no boundary is left
    CHECK(groups.interfaceOf(2).empty());
    CHECK(groups.connectionCount(2) == 4);
    CHECK(groups.group(2).nodes.size() == 20);
}

TEST_CASE("a constraint stamp follows the constraints on the nodes of its unit only", "[analysis][connections]")
{
    const std::string deck = weldedPlates("SinglePointConstrList;1;123456;1\nSinglePointConstrList;1;123456;101\n");
    const model::Model first = readModelText(deck);
    const auto components = analysis::ComponentModel::of(first);
    const auto before = analysis::ConstraintStamps::compute(first, analysis::ComponentMembership(*components));
    REQUIRE(before.size() == 2);
    CHECK(before[0].spcSet == 1);
    CHECK(before[0].constrainedNodeCount == 1);
    const model::Model second =
        readModelText(weldedPlates("SinglePointConstrList;1;123;1\nSinglePointConstrList;1;123456;101\n"));
    const auto after = analysis::ConstraintStamps::compute(
        second, analysis::ComponentMembership(*analysis::ComponentModel::of(second)));
    CHECK(after[0].revision != before[0].revision);
    CHECK(after[1].revision == before[1].revision);
}

TEST_CASE("the cache keeps a result under its key while the content of the model is the one it was made for",
          "[analysis][components][cache]")
{
    model::Model model = readModelText("Node;1;;0.;0.;0.\nNode;2;;1.;0.;0.\n");
    analysis::ComponentCache& cache = analysis::ComponentCache::of(model);
    int made = 0;
    const auto make = [&made] { return std::static_pointer_cast<const void>(std::make_shared<const int>(++made)); };
    const std::shared_ptr<const void> first = cache.memo(model, "table", make);
    REQUIRE(cache.memo(model, "table", make) == first); // kept: not made again
    REQUIRE(made == 1);
    REQUIRE(cache.memo(model, "other", make) != first); // another key
    REQUIRE(made == 2);
    model.remove(*model.locate(base::IdSpace::Node, 2)); // another content
    const std::shared_ptr<const void> again = cache.memo(model, "table", make);
    REQUIRE(again != first);
    REQUIRE(*std::static_pointer_cast<const int>(again) == 3);

    // A result that depends on the Case Control too is kept while its control version is the one it was made for.
    const std::shared_ptr<const void> selected = cache.memo(model, "nsm", make, true);
    REQUIRE(cache.memo(model, "nsm", make, true) == selected);
    (void)std::as_const(model).control(); // a read
    REQUIRE(cache.memo(model, "nsm", make, true) == selected);
    REQUIRE(cache.memo(model, "table", make) == again);
    (void)model.control(); // an edit may follow: made again; a result of the content alone is kept
    REQUIRE(cache.memo(model, "nsm", make, true) != selected);
    REQUIRE(cache.memo(model, "table", make) == again);
}
