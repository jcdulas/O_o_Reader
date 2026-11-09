#include "ElementPoints.hpp"

#include "oor/model/cards/nodes/acousticSlotSurfacePoint/AcousticSlotSurfacePointTable.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"

#include <algorithm>
#include <string>

namespace oor::analysis::detail
{

NodeLookup::NodeLookup(const model::Model& model, std::size_t expectedLookups) : m_model(model)
{
    m_nodeType = model::typeIndexOf<model::NodeTable>();
    const model::NodeTable* grids = model.findTable<model::NodeTable>();
    if (grids == nullptr)
        return;
    const std::size_t rowCount = grids->rows().rowCount();
    if (rowCount == 0 || expectedLookups * 4 < rowCount)
        return;
    base::EntityId highest = 0;
    for (std::uint32_t row = 0; row < rowCount; ++row)
    {
        if (grids->rows().isAlive(row))
            highest = std::max(highest, grids->idAtRow(row));
    }
    if (highest <= 0 || static_cast<std::size_t>(highest) > rowCount * 16 + 1024)
        return;
    m_direct.assign(static_cast<std::size_t>(highest) + 1, -1);
    for (std::uint32_t row = 0; row < rowCount; ++row)
    {
        if (grids->rows().isAlive(row) && grids->idAtRow(row) > 0)
            m_direct[static_cast<std::size_t>(grids->idAtRow(row))] = static_cast<std::int32_t>(row);
    }
    m_useDirect = true;
}

std::int32_t NodeLookup::nodeRow(base::EntityId id) const noexcept
{
    if (m_useDirect)
        return id > 0 && static_cast<std::size_t>(id) < m_direct.size() ? m_direct[static_cast<std::size_t>(id)] : -1;
    const auto location = m_model.locate(base::IdSpace::Node, id);
    return location && location->table == m_nodeType ? static_cast<std::int32_t>(location->row) : -1;
}

bool NodeLookup::exists(base::EntityId id) const noexcept
{
    return m_model.locate(base::IdSpace::Node, id).has_value();
}

std::optional<base::Vec3> NodeLookup::slotPoint(base::EntityId id) const
{
    const std::optional<model::CardLocation> location = m_model.locate(base::IdSpace::Node, id);
    if (!location || location->table != model::typeIndexOf<model::AcousticSlotSurfacePointTable>())
        return std::nullopt;
    const model::AcousticSlotSurfacePointCard card =
        m_model.findTable<model::AcousticSlotSurfacePointTable>()->card(location->row);
    return base::Vec3{card.radius, card.zCoord, 0.0};
}

void gatherElementPoints(const ElementProvider& provider, const model::CardTable& table, BasicPositionCache& cache,
                         const NodeLookup& lookup, RowScratch& scratch, std::uint32_t row)
{
    scratch.ids.clear();
    provider.nodes(table, row, scratch.ids);
    scratch.nodeRows.assign(scratch.ids.size(), -1);
    scratch.points.assign(scratch.ids.size(), undefinedVector());
    for (std::size_t slot = 0; slot < scratch.ids.size(); ++slot)
    {
        const base::EntityId id = scratch.ids[slot];
        if (id == 0)
            continue;
        const std::int32_t nodeRow = lookup.nodeRow(id);
        if (nodeRow >= 0)
        {
            scratch.nodeRows[slot] = nodeRow;
            scratch.points[slot] = cache.position(static_cast<std::uint32_t>(nodeRow));
        }
        else if (const std::optional<base::Vec3> slotPoint = lookup.slotPoint(id))
        {
            scratch.points[slot] = *slotPoint;
        }
        else if (!lookup.exists(id))
        {
            throw GeometryError(std::string(provider.name()) + " " + std::to_string(table.idAt(row)) +
                                " refers to grid point " + std::to_string(id) + ", which does not exist");
        }
    }
}

} // namespace oor::analysis::detail
