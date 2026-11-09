#include "oor/analysis/superelements/ManualConnections.hpp"

#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/ids/IdRangeExpansion.hpp"
#include "oor/model/cards/superelements/superelementConnection/SuperelementConnectionTable.hpp"
#include "oor/model/cards/superelements/superelementCopyPoints/SuperelementCopyPointsTable.hpp"
#include "oor/model/cards/superelements/superelementCopySequence/SuperelementCopySequenceTable.hpp"
#include "oor/model/cards/superelements/superelementExternalBoundary/SuperelementExternalBoundaryTable.hpp"

#include <algorithm>
#include <cmath>

namespace oor::analysis
{

namespace
{

// The model the points of a superelement are named in (0: the residual structure).
const model::Model* idModelOf(const SuperelementCatalogue& catalogue, base::EntityId seid)
{
    if (seid == 0)
        return &catalogue.residual();
    const SuperelementEntry* entry = catalogue.find(seid);
    return entry != nullptr ? entry->idModel : nullptr;
}

bool hasPoint(const model::Model* model, base::EntityId nid)
{
    return model != nullptr && model->locate(base::IdSpace::Node, nid).has_value();
}

std::string text(double value)
{
    return std::to_string(value);
}

// The start of a message about a superelement connection card: its keyword and its two superelements ("SECONCT 1 2").
std::string connectionText(const model::Model&, const model::SuperelementConnectionCard& card)
{
    return std::string("SuperelementConnection") + " " + std::to_string(card.superelementIdA) + " " +
           std::to_string(card.superelementIdB);
}

} // namespace

ManualConnections::ManualConnections(const SuperelementCatalogue& catalogue,
                                     const std::map<base::EntityId, PlacedPoints>& placed)
{
    const model::Model& residual = catalogue.residual();
    const auto positionOf = [&placed](const SuperelementPoint& point) -> const base::Vec3*
    {
        const auto found = placed.find(point.seid);
        return found != placed.end() ? found->second.find(point.nid) : nullptr;
    };
    forEachCard<model::SuperelementConnectionTable>(
        residual,
        [&](const model::SuperelementConnectionCard& card, model::CardLocation)
        {
            const model::Model* modelA = idModelOf(catalogue, card.superelementIdA);
            const model::Model* modelB = idModelOf(catalogue, card.superelementIdB);
            const bool checksLocation = card.location == model::SuperelementBulkOptionsLocation::Yes;
            const double tolerance = catalogue.connectionTolerance(card.superelementIdA, card.tolerance);
            for (const model::SuperelementConnectionPair& pair : card.pairs)
            {
                const std::int64_t countA = static_cast<std::int64_t>(pair.a.last) - pair.a.first + 1;
                const std::int64_t countB = static_cast<std::int64_t>(pair.b.last) - pair.b.first + 1;
                if (countA != countB || countA <= 0)
                {
                    m_problems.push_back(connectionText(residual, card) + ": the ranges from " +
                                         std::to_string(pair.a.first) + " and from " + std::to_string(pair.b.first) +
                                         " do not have the same length");
                    continue;
                }
                for (std::int64_t offset = 0; offset < countA; ++offset)
                {
                    const SuperelementPoint upstream{card.superelementIdA,
                                                     static_cast<base::EntityId>(pair.a.first + offset)};
                    const SuperelementPoint joined{card.superelementIdB,
                                                   static_cast<base::EntityId>(pair.b.first + offset)};
                    // An ID of a THRU range that is not a point of its superelement is skipped (the library's reading:
                    // the solver asks for ranges whose IDs all exist); a single pair that names no point is reported.
                    const bool isRange = countA > 1;
                    if (!hasPoint(modelA, upstream.nid) || !hasPoint(modelB, joined.nid))
                    {
                        if (!isRange)
                            m_problems.push_back(connectionText(residual, card) + ": point " +
                                                 std::to_string(upstream.nid) + " or " + std::to_string(joined.nid) +
                                                 " does not exist in its superelement");
                        continue;
                    }
                    m_links.push_back(
                        BoundaryLink{upstream, joined, BoundarySource::SuperelementConnection, checksLocation});
                    const base::Vec3* from = positionOf(upstream);
                    const base::Vec3* to = positionOf(joined);
                    if (checksLocation && from != nullptr && to != nullptr)
                    {
                        const double distance = base::norm(*from - *to);
                        if (distance > tolerance)
                            m_problems.push_back(connectionText(residual, card) + ": points " +
                                                 std::to_string(upstream.nid) + " and " + std::to_string(joined.nid) +
                                                 " are " + text(distance) + " apart, more than the tolerance " +
                                                 text(tolerance) + " (" + std::string("location") + " = YES)");
                    }
                }
            }
        });

    for (const SuperelementEntry& entry : catalogue.entries())
    {
        if (!entry.isExternal() || entry.part == nullptr)
            continue;
        forEachCard<model::SuperelementExternalBoundaryTable>(
            *entry.part,
            [&](const model::SuperelementExternalBoundaryCard& card, model::CardLocation)
            {
                for (const model::SuperelementExternalBoundaryItem& item : card.items)
                {
                    // every point of a range is a boundary point, defined or not: a range
                    // longer than the bound of THRU expansions is left out and named
                    if (base::idRangeCount(item.points.first, item.points.last) >
                        static_cast<std::uint64_t>(base::MAX_EXPANDED_IDS))
                    {
                        m_problems.push_back(std::string("SuperelementExternalBoundary") + " of superelement " +
                                             std::to_string(entry.seid) + ": " + std::to_string(item.points.first) +
                                             " THRU " + std::to_string(item.points.last) + " names more than " +
                                             std::to_string(base::MAX_EXPANDED_IDS) + " points: left out");
                        continue;
                    }
                    const std::int64_t last = std::max(item.points.first, item.points.last);
                    for (std::int64_t id = item.points.first; id <= last; ++id)
                    {
                        const auto nid = static_cast<base::EntityId>(id);
                        // the components of a scalar point are 0 (or
                        // blank).
                        const bool isScalar = item.components == 0 || isScalarPoint(*entry.part, nid);
                        m_externalPoints[SuperelementPoint{entry.seid, nid}] =
                            isScalar ? std::string("0") : std::to_string(item.components);
                    }
                }
            });
    }
}

std::vector<BoundaryLink> ManualConnections::superelementCopyPointsLinks(
    const SuperelementCatalogue& catalogue,
    const std::function<std::vector<base::EntityId>(base::EntityId)>& exteriorOf, std::vector<std::string>& problems)
{
    std::vector<BoundaryLink> links;
    const model::Model& residual = catalogue.residual();
    std::map<base::EntityId, std::vector<base::EntityId>> sequences; // SuperelementCopySequence by secondary number
    forEachCard<model::SuperelementCopySequenceTable>(
        residual, [&](const model::SuperelementCopySequenceCard& card, model::CardLocation)
        { sequences[card.superelementId % model::SuperelementCopyPointsCard::NUMBER_RANGE] = card.points; });
    forEachCard<model::SuperelementCopyPointsTable>(
        residual,
        [&](const model::SuperelementCopyPointsCard& card, model::CardLocation)
        {
            const base::EntityId seid = card.number();
            const SuperelementEntry* entry = catalogue.find(seid);
            const base::EntityId downstream = entry != nullptr ? entry->downstream : 0;
            const base::EntityId primary = std::abs(card.primarySuperelementId);
            std::vector<base::EntityId> primaryPoints;
            if (const auto sequence = sequences.find(seid); sequence != sequences.end())
                primaryPoints = sequence->second;
            else if (primary != 0)
                primaryPoints = exteriorOf(primary);
            else
                primaryPoints = card.points; // matrices from an external source: the points are their own
            if (primaryPoints.size() != card.points.size())
                problems.push_back(std::string("SuperelementCopyPoints") + " " + std::to_string(card.superelementId) +
                                   ": " + std::to_string(card.points.size()) + " points where its primary has " +
                                   std::to_string(primaryPoints.size()) + " exterior points");
            const std::size_t count = std::min(primaryPoints.size(), card.points.size());
            for (std::size_t index = 0; index < count; ++index)
                links.push_back(BoundaryLink{SuperelementPoint{seid, primaryPoints[index]},
                                             SuperelementPoint{downstream, card.points[index]},
                                             BoundarySource::SuperelementCopyPoints, false});
        });
    return links;
}

} // namespace oor::analysis
