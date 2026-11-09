#include "oor/analysis/components/Zones.hpp"

#include <algorithm>

namespace oor::analysis
{

std::string_view causeKindName(CauseKind kind) noexcept
{
    switch (kind)
    {
    case CauseKind::Element:
        return "element";
    case CauseKind::Rigid:
        return "rigid";
    case CauseKind::Mpc:
        return "mpc";
    case CauseKind::Connection:
        return "connection";
    }
    return "?";
}

namespace
{

CauseKind causeOf(const ModelEntity& entity) noexcept
{
    if (entity.isEquation)
        return CauseKind::Mpc;
    if (entity.role == ElementRole::Rigid)
        return CauseKind::Rigid;
    if (entity.role == ElementRole::Connector)
        return CauseKind::Connection;
    return CauseKind::Element;
}

} // namespace

std::vector<ZoneBoundaryPoint> Zones::boundaryOf(const model::Model& model, const GroupSet& groups, std::int32_t unit,
                                                 const ModelEntities& entities, const NodeEntityIndex& index,
                                                 const FastenerPiercings* piercings)
{
    std::vector<ZoneBoundaryPoint> points;
    const ResolvedGroup& group = groups.group(unit);
    for (const base::EntityId node : group.nodes)
    {
        ZoneBoundaryPoint point;
        point.node = node;
        for (const std::uint32_t entity : index.entitiesAt(node))
        {
            if (group.isMember(entity))
                continue;
            const ModelEntity& outside = entities.entity(entity);
            point.causes.push_back(ZoneCause{causeOf(outside), entity});
            point.components |= EntityDofs::at(model, outside, node, piercings);
            if (piercings != nullptr && !outside.isEquation)
            {
                if (const PiercedFastener* fastener = piercings->find(outside.card))
                {
                    for (const FastenerPiercing& side : piercings->sidesOf(*fastener))
                        point.isPiercedPoint = point.isPiercedPoint ||
                                               std::find(side.nids.begin(), side.nids.end(), node) != side.nids.end();
                }
            }
        }
        if (!point.causes.empty())
            points.push_back(std::move(point));
    }
    return points;
}

std::vector<EnclosedElement> Zones::enclosedBy(const GroupSet& groups, std::int32_t unit, const ModelEntities& entities)
{
    std::vector<EnclosedElement> enclosed;
    for (const std::uint32_t entity : groups.group(unit).absorbed)
    {
        if (entities.entity(entity).isEquation)
            continue;
        const auto nodes = entities.nodesOf(entity);
        const bool onePoint = !nodes.empty() && std::all_of(nodes.begin(), nodes.end(), [&](base::EntityId node)
                                                            { return node == nodes.front(); });
        enclosed.push_back(EnclosedElement{entity, onePoint});
    }
    return enclosed;
}

} // namespace oor::analysis
