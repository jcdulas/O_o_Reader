#include "oor/analysis/superelements/PartBoundaries.hpp"

#include "oor/analysis/superelements/AutomaticBoundarySearch.hpp"
#include "oor/analysis/superelements/BoundaryPointNumbering.hpp"
#include "oor/analysis/superelements/ManualConnections.hpp"
#include "oor/analysis/superelements/PartPlacement.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/errors/OorError.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>

namespace oor::analysis
{

namespace
{

// When several links make a point a boundary point, the source shown is the first of this list that applies: a card
// that names the point before the search that finds it, the search before the element connectivity.
constexpr std::array<BoundarySource, 7> SOURCE_PRIORITY{
    BoundarySource::SuperelementConnection,      BoundarySource::SuperelementExteriorPoints,
    BoundarySource::SuperelementBoundarySearch,  BoundarySource::Automatic,
    BoundarySource::SuperelementInteriorNodes,   BoundarySource::SuperelementCopyPoints,
    BoundarySource::SuperelementExternalBoundary};

std::size_t rankOf(BoundarySource source) noexcept
{
    for (std::size_t rank = 0; rank < SOURCE_PRIORITY.size(); ++rank)
    {
        if (SOURCE_PRIORITY[rank] == source)
            return rank;
    }
    return SOURCE_PRIORITY.size();
}

// What the links say about one point: its source and whether any of them has a location.
struct PointFacts
{
    BoundarySource source = BoundarySource::SuperelementExternalBoundary;
    bool hasLocation = false;
};

void note(std::map<SuperelementPoint, PointFacts>& facts, const SuperelementPoint& point, BoundarySource source,
          bool hasLocation)
{
    const auto [it, inserted] = facts.try_emplace(point, PointFacts{source, hasLocation});
    if (inserted)
        return;
    if (rankOf(source) < rankOf(it->second.source))
        it->second.source = source;
    it->second.hasLocation = it->second.hasLocation || hasLocation;
}

} // namespace

PartBoundaries::PartBoundaries(const model::Model& model) : m_catalogue(model), m_listPartition(m_catalogue)
{
    m_problems = m_listPartition.problems();
    const model::Model& residual = m_catalogue.residual();
    std::map<base::EntityId, PlacedPoints> placed; // the residual structure under 0
    try
    {
        placed[0] = PartPlacement::gridPoints(residual);
    }
    catch (const base::OorError& error)
    {
        m_problems.push_back(std::string("the positions of the residual structure cannot be computed: ") +
                             error.what());
    }
    for (const SuperelementEntry& entry : m_catalogue.entries())
    {
        if (entry.hasPartPoints() && entry.type != SuperelementType::Secondary &&
            entry.type != SuperelementType::Collector)
            placed[entry.seid] = PartPlacement::place(m_catalogue, entry, m_problems);
    }

    const ManualConnections manual(m_catalogue, placed);
    m_problems.insert(m_problems.end(), manual.problems().begin(), manual.problems().end());

    // format reference SEBULK METHOD = AUTO: the parts searched; an external superelement by the grid points of its
    // SuperelementExternalBoundary.
    std::map<base::EntityId, PlacedPoints> searched;
    for (const auto& [seid, points] : placed)
    {
        const SuperelementEntry* entry = m_catalogue.find(seid);
        if (entry == nullptr || entry->method != model::SuperelementBulkOptionsMethod::Auto)
            continue;
        if (!entry->isExternal())
        {
            searched[seid] = points;
            continue;
        }
        PlacedPoints boundary;
        for (std::size_t index = 0; index < points.nids.size(); ++index)
        {
            if (manual.externalPoints().contains(SuperelementPoint{seid, points.nids[index]}))
            {
                boundary.nids.push_back(points.nids[index]);
                boundary.positions.push_back(points.positions[index]);
            }
        }
        searched[seid] = std::move(boundary);
    }
    const PlacedPoints noPoints;
    const auto residualPoints = placed.find(0);
    std::vector<BoundaryLink> links = AutomaticBoundarySearch::search(
        m_catalogue, residualPoints != placed.end() ? residualPoints->second : noPoints, searched);
    links.insert(links.end(), manual.links().begin(), manual.links().end());
    links.insert(links.end(), m_listPartition.links().begin(), m_listPartition.links().end());
    std::vector<SuperelementPoint> lonePoints;
    for (const auto& [point, components] : manual.externalPoints())
        lonePoints.push_back(point);

    // A SuperelementCopyPoints pairs with the exterior points of its primary, known once the other connections are
    // joined.
    {
        const BoundaryPointNumbering first(m_catalogue, links, lonePoints);
        const std::vector<BoundaryLink> copies = ManualConnections::superelementCopyPointsLinks(
            m_catalogue, [&first](base::EntityId seid) { return first.pointsOf(seid); }, m_problems);
        links.insert(links.end(), copies.begin(), copies.end());
    }
    const BoundaryPointNumbering numbering(m_catalogue, links, lonePoints);

    std::map<SuperelementPoint, PointFacts> facts;
    for (const BoundaryLink& link : links)
    {
        note(facts, link.upstream, link.source, link.hasLocation);
        note(facts, link.joined, link.source, link.hasLocation);
    }
    const auto positionOf = [&placed](const SuperelementPoint& point) -> const base::Vec3*
    {
        const auto found = placed.find(point.seid);
        return found != placed.end() ? found->second.find(point.nid) : nullptr;
    };
    for (const SuperelementEntry& entry : m_catalogue.entries())
    {
        for (const base::EntityId nid : numbering.pointsOf(entry.seid))
        {
            const SuperelementPoint point{entry.seid, nid};
            PartBoundaryRow row;
            row.seid = entry.seid;
            row.nid = nid;
            row.downstreamId = entry.downstream;
            row.downstreamNid = numbering.pointIn(point, entry.downstream);
            row.boundaryPoint = numbering.numberOf(point);
            const auto known = facts.find(point);
            const PointFacts fact = known != facts.end() ? known->second : PointFacts{};
            row.source = fact.source;
            if (entry.type == SuperelementType::List)
                row.dof = m_listPartition.dofOf(point);
            else if (const auto external = manual.externalPoints().find(point);
                     external != manual.externalPoints().end())
                row.dof = external->second;
            else
                row.dof = isScalarPoint(*entry.idModel, nid) ? "0" : "123456";
            row.distance = std::numeric_limits<double>::quiet_NaN();
            const base::Vec3* own = positionOf(point);
            if (fact.hasLocation && own != nullptr)
            {
                const base::Vec3* target = nullptr;
                if (row.downstreamNid)
                    target = positionOf(SuperelementPoint{entry.downstream, *row.downstreamNid});
                if (target != nullptr)
                    row.distance = base::norm(*own - *target);
                else
                {
                    double nearest = std::numeric_limits<double>::infinity();
                    for (const SuperelementPoint& member : numbering.membersOf(point))
                    {
                        const base::Vec3* other = member == point ? nullptr : positionOf(member);
                        if (other != nullptr)
                            nearest = std::min(nearest, base::norm(*own - *other));
                    }
                    if (std::isfinite(nearest))
                        row.distance = nearest;
                }
            }
            m_rows.push_back(std::move(row));
        }
    }
}

} // namespace oor::analysis
