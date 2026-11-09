#include "oor/analysis/superelements/AutomaticBoundarySearch.hpp"

#include "oor/analysis/geometry/PointCellIndex.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/superelements/superelementBoundarySearch/SuperelementBoundarySearchTable.hpp"
#include "oor/model/cards/superelements/superelementExcludedNodes/SuperelementExcludedNodesTable.hpp"

#include <algorithm>
#include <tuple>

namespace oor::analysis
{

namespace
{

// The point lists of the SEBNDRY (or SEEXCLD) cards of one superelement SEIDA, each with its SEIDB (nothing: ALL).
using PointLists = std::multimap<base::EntityId, std::pair<model::SuperelementOrAll, std::vector<model::IdRun>>>;

bool holds(const std::vector<model::IdRun>& runs, base::EntityId nid) noexcept
{
    return std::any_of(runs.begin(), runs.end(), [nid](const model::IdRun& run)
                       { return nid >= run.first && nid <= std::max(run.first, run.last); });
}

// Whether the lists of `seid` that concern the search against `other` exist, and whether they hold `nid`.
struct ListAnswer
{
    bool concerned = false;
    bool holdsPoint = false;
};

ListAnswer ask(const PointLists& lists, base::EntityId seid, base::EntityId nid, base::EntityId other)
{
    ListAnswer answer;
    const auto [first, last] = lists.equal_range(seid);
    for (auto it = first; it != last; ++it)
    {
        const auto& [seidb, runs] = it->second;
        if (seidb.has_value() && *seidb != other)
            continue;
        answer.concerned = true;
        answer.holdsPoint = answer.holdsPoint || holds(runs, nid);
    }
    return answer;
}

struct SearchPoint
{
    base::EntityId seid = 0;
    base::EntityId nid = 0;
};

} // namespace

std::vector<BoundaryLink> AutomaticBoundarySearch::search(const SuperelementCatalogue& catalogue,
                                                          const PlacedPoints& residualPoints,
                                                          const std::map<base::EntityId, PlacedPoints>& searched)
{
    std::vector<BoundaryLink> links;
    if (searched.empty())
        return links;
    const model::Model& residual = catalogue.residual();
    PointLists boundaries;
    forEachCard<model::SuperelementBoundarySearchTable>(
        residual, [&](const model::SuperelementBoundarySearchCard& card, model::CardLocation)
        { boundaries.emplace(card.superelementIdA, std::pair{card.superelementIdB, card.grids}); });
    PointLists exclusions;
    forEachCard<model::SuperelementExcludedNodesTable>(
        residual, [&](const model::SuperelementExcludedNodesCard& card, model::CardLocation)
        { exclusions.emplace(card.superelementIdA, std::pair{card.superelementIdB, card.grids}); });

    std::vector<SearchPoint> points;
    std::vector<base::Vec3> positions;
    const auto addAll = [&](base::EntityId seid, const PlacedPoints& placed)
    {
        for (std::size_t index = 0; index < placed.nids.size(); ++index)
        {
            points.push_back(SearchPoint{seid, placed.nids[index]});
            positions.push_back(placed.positions[index]);
        }
    };
    addAll(0, residualPoints);
    for (const auto& [seid, placed] : searched)
        addAll(seid, placed);

    const PointCellIndex index(positions, catalogue.largestTolerance());
    index.forEachPairWithin(
        catalogue.largestTolerance(),
        [&](std::uint32_t first, std::uint32_t second)
        {
            SearchPoint a = points[first];
            SearchPoint b = points[second];
            // Points of one superelement never join each other; two residual points are not a boundary.
            if (a.seid == b.seid)
                return;
            const base::Vec3 offset = positions[first] - positions[second];
            const double tolerance = catalogue.searchTolerance(a.seid, b.seid);
            if (base::dot(offset, offset) > tolerance * tolerance)
                return;
            if (ask(exclusions, a.seid, a.nid, b.seid).holdsPoint || ask(exclusions, b.seid, b.nid, a.seid).holdsPoint)
                return;
            const ListAnswer fromA = ask(boundaries, a.seid, a.nid, b.seid);
            const ListAnswer fromB = ask(boundaries, b.seid, b.nid, a.seid);
            if ((fromA.concerned && !fromA.holdsPoint) || (fromB.concerned && !fromB.holdsPoint))
                return;
            if (a.seid < b.seid)
                std::swap(a, b);
            const bool restricted = a.seid == points[first].seid ? fromA.concerned : fromB.concerned;
            links.push_back(BoundaryLink{
                SuperelementPoint{a.seid, a.nid}, SuperelementPoint{b.seid, b.nid},
                restricted ? BoundarySource::SuperelementBoundarySearch : BoundarySource::Automatic, true});
        });
    std::sort(links.begin(), links.end(), [](const BoundaryLink& left, const BoundaryLink& right)
              { return std::tie(left.upstream, left.joined) < std::tie(right.upstream, right.joined); });
    return links;
}

} // namespace oor::analysis
