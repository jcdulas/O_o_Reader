#include "oor/analysis/superelements/ListSuperelementPartition.hpp"

#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/ids/IdRangeExpansion.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/superelements/superelementElemTransfer/SuperelementElemTransferTable.hpp"
#include "oor/model/cards/superelements/superelementExteriorPoints/SuperelementExteriorPointsTable.hpp"
#include "oor/model/cards/superelements/superelementInteriorNodes/SuperelementInteriorNodesTable.hpp"
#include "oor/model/cards/superelements/superelementReleasedDofs/SuperelementReleasedDofsTable.hpp"
#include "oor/model/core/CardRegistry.hpp"

#include <algorithm>
#include <set>
#include <unordered_map>

namespace oor::analysis
{

namespace
{

// The ID runs of the cards of one kind (SuperelementInteriorNodes, SuperelementElemTransfer), each with the
// superelement it gives its IDs to, sorted by their first ID: the owner of an ID is found by a binary search, so a THRU
// over millions of IDs is never walked.
struct OwnedRun
{
    base::EntityId first = 0;
    base::EntityId last = 0;
    base::EntityId seid = 0;
};

class RunOwners
{
public:
    void add(const std::vector<model::IdRun>& runs, base::EntityId seid)
    {
        for (const model::IdRun& run : runs)
            m_runs.push_back(OwnedRun{run.first, std::max(run.first, run.last), seid});
    }
    void finish()
    {
        std::sort(m_runs.begin(), m_runs.end(),
                  [](const OwnedRun& left, const OwnedRun& right) { return left.first < right.first; });
    }
    [[nodiscard]] bool isEmpty() const noexcept { return m_runs.empty(); }
    // The superelement whose run holds the ID; 0 when none does. Of runs that overlap (a point on two lists,
    // which the check superelement.node-in-two reports) the one that starts last is asked.
    [[nodiscard]] base::EntityId ownerOf(base::EntityId id) const noexcept
    {
        auto it = std::upper_bound(m_runs.begin(), m_runs.end(), id,
                                   [](base::EntityId value, const OwnedRun& run) { return value < run.first; });
        if (it == m_runs.begin())
            return 0;
        --it;
        return id <= it->last ? it->seid : 0;
    }

private:
    std::vector<OwnedRun> m_runs;
};

bool listedIn(const std::vector<model::IdRun>& runs, base::EntityId id) noexcept
{
    return std::any_of(runs.begin(), runs.end(), [id](const model::IdRun& run)
                       { return id >= run.first && id <= std::max(run.first, run.last); });
}

// The digits 1 to 6 that a component number does not hold ("123456" less the released ones).
std::string remainingComponents(std::int32_t released)
{
    std::string kept;
    for (char digit = '1'; digit <= '6'; ++digit)
    {
        if (std::to_string(released).find(digit) == std::string::npos)
            kept.push_back(digit);
    }
    return kept;
}

} // namespace

ListSuperelementPartition::ListSuperelementPartition(const SuperelementCatalogue& catalogue)
{
    std::set<base::EntityId> listSeids;
    for (const SuperelementEntry& entry : catalogue.entries())
    {
        if (entry.type == SuperelementType::List)
            listSeids.insert(entry.seid);
    }
    if (listSeids.empty())
        return;
    const model::Model& residual = catalogue.residual();

    // Interior points, a SuperelementInteriorNodes takes precedence over the SEID of
    // the Node.
    RunOwners sesets;
    forEachCard<model::SuperelementInteriorNodesTable>(
        residual, [&](const model::SuperelementInteriorNodesCard& card, model::CardLocation)
        { sesets.add(card.grids, card.superelementId); });
    sesets.finish();
    std::unordered_map<base::EntityId, base::EntityId> interiorOf;
    if (const model::NodeTable* grids = residual.findTable<model::NodeTable>())
    {
        for (std::uint32_t row = 0; row < grids->rows().rowCount(); ++row)
        {
            if (!grids->rows().isAlive(row))
                continue;
            const base::EntityId nid = grids->idAtRow(row);
            base::EntityId owner = sesets.isEmpty() ? 0 : sesets.ownerOf(nid);
            const bool fromInteriorNodes = owner != 0;
            if (owner == 0)
                owner = grids->card(row).superelementId;
            if (owner != 0)
            {
                interiorOf[nid] = owner;
                ++m_interiorCounts[owner];
                if (listSeids.contains(owner))
                    m_interiorPoints.push_back(InteriorPoint{owner, nid, fromInteriorNodes});
            }
        }
    }

    // Elements: an element goes to the superelement of its interior points; a SuperelementElemTransfer moves one.
    RunOwners seelts;
    forEachCard<model::SuperelementElemTransferTable>(
        residual, [&](const model::SuperelementElemTransferCard& card, model::CardLocation)
        { seelts.add(card.elements, card.superelementId); });
    seelts.finish();
    std::map<SuperelementPoint, BoundarySource> exterior;
    const model::CardRegistry& registry = model::CardRegistry::instance();
    std::vector<base::EntityId> nodes;
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        const model::CardTable* table = residual.findTable(type);
        if (table == nullptr || !ElementTopology::supports(type))
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            nodes.clear();
            ElementTopology::nodesOf(*table, type, row, nodes);
            const base::EntityId eid = table->idAt(row);
            base::EntityId owner = seelts.isEmpty() ? 0 : seelts.ownerOf(eid);
            const bool fromElemTransfer = owner != 0;
            if (owner == 0)
            {
                for (const base::EntityId node : nodes)
                {
                    const auto interior = interiorOf.find(node);
                    if (interior == interiorOf.end() || interior->second == owner)
                        continue;
                    if (owner != 0)
                    {
                        m_problems.push_back("element " + std::to_string(eid) + " (" +
                                             std::string(registry.entry(type).info.name) +
                                             ") touches interior points of superelements " + std::to_string(owner) +
                                             " and " + std::to_string(interior->second));
                        break;
                    }
                    owner = interior->second;
                }
            }
            if (owner == 0 || !listSeids.contains(owner))
                continue;
            ++m_elementCounts[owner];
            m_elements.push_back(PartitionElement{owner, eid, type, fromElemTransfer});
            for (const base::EntityId node : nodes)
            {
                const auto interior = interiorOf.find(node);
                if (node > 0 && (interior == interiorOf.end() || interior->second != owner))
                    exterior.try_emplace(SuperelementPoint{owner, node}, BoundarySource::SuperelementInteriorNodes);
            }
        }
    }
    // the points an MPC alone links are made exterior by the card.
    forEachCard<model::SuperelementExteriorPointsTable>(
        residual,
        [&](const model::SuperelementExteriorPointsCard& card, model::CardLocation)
        {
            if (!listSeids.contains(card.superelementId))
                return;
            for (const model::IdRun& run : card.points)
            {
                // a range longer than the bound of THRU expansions is left out, named
                if (base::idRangeCount(run.first, run.last) > static_cast<std::uint64_t>(base::MAX_EXPANDED_IDS))
                {
                    m_problems.push_back(std::string("SuperelementExteriorPoints") + " " +
                                         std::to_string(card.superelementId) + ": " + std::to_string(run.first) +
                                         " THRU " + std::to_string(run.last) + " names more than " +
                                         std::to_string(base::MAX_EXPANDED_IDS) + " points: left out");
                    continue;
                }
                const std::int64_t last = std::max(run.first, run.last);
                for (std::int64_t id = run.first; id <= last; ++id)
                {
                    const auto nid = static_cast<base::EntityId>(id);
                    if (residual.locate(base::IdSpace::Node, nid))
                        exterior[SuperelementPoint{card.superelementId, nid}] =
                            BoundarySource::SuperelementExteriorPoints;
                }
            }
        });

    std::sort(m_interiorPoints.begin(), m_interiorPoints.end(),
              [](const InteriorPoint& left, const InteriorPoint& right)
              { return std::pair(left.seid, left.nid) < std::pair(right.seid, right.nid); });
    std::sort(m_elements.begin(), m_elements.end(), [](const PartitionElement& left, const PartitionElement& right)
              { return std::pair(left.seid, left.eid) < std::pair(right.seid, right.eid); });

    std::vector<model::SuperelementReleasedDofsCard> releases;
    forEachCard<model::SuperelementReleasedDofsTable>(residual, [&](const model::SuperelementReleasedDofsCard& card,
                                                                    model::CardLocation) { releases.push_back(card); });
    for (const auto& [point, source] : exterior)
    {
        const SuperelementEntry* entry = catalogue.find(point.seid);
        const base::EntityId downstream = entry != nullptr ? entry->downstream : 0;
        m_links.push_back(BoundaryLink{point, SuperelementPoint{downstream, point.nid}, source, false});
        if (isScalarPoint(residual, point.nid))
        {
            m_dofs[point] = "0";
            continue;
        }
        // format reference RELEASE: the components C of the listed exterior grid points (or of all of them) are not
        // connected.
        std::int32_t released = 0;
        for (const model::SuperelementReleasedDofsCard& release : releases)
        {
            if (release.superelementId == point.seid && (release.all || listedIn(release.points, point.nid)))
                released = released * 10 + release.components;
        }
        m_dofs[point] = released == 0 ? std::string("123456") : remainingComponents(released);
    }
}

std::string ListSuperelementPartition::dofOf(const SuperelementPoint& point) const
{
    const auto found = m_dofs.find(point);
    return found != m_dofs.end() ? found->second : std::string();
}

std::size_t ListSuperelementPartition::interiorCount(base::EntityId seid) const
{
    const auto found = m_interiorCounts.find(seid);
    return found != m_interiorCounts.end() ? found->second : 0;
}

std::size_t ListSuperelementPartition::elementCount(base::EntityId seid) const
{
    const auto found = m_elementCounts.find(seid);
    return found != m_elementCounts.end() ? found->second : 0;
}

} // namespace oor::analysis
