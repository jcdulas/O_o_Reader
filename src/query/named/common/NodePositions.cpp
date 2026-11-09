#include "oor/query/named/common/NodePositions.hpp"

#include "oor/model/cards/nodes/node/NodeTable.hpp"

#include <limits>

namespace oor::query
{

NodePositions::NodePositions(const model::Model& model, base::ThreadPool* pool)
    : m_model(model), m_nodeType(model::typeIndexOf<model::NodeTable>())
{
    if (model.findTable(m_nodeType) != nullptr)
        m_positions = analysis::BasicPositionCache::of(model, pool).positions();
}

std::int64_t NodePositions::row(base::EntityId node) const noexcept
{
    const auto location = m_model.locate(base::IdSpace::Node, node);
    if (!location || location->table != m_nodeType || location->row >= m_positions.size())
        return -1;
    return location->row;
}

base::Vec3 NodePositions::position(base::EntityId node) const noexcept
{
    const std::int64_t found = row(node);
    if (found < 0)
        return base::Vec3{};
    const std::array<double, 3>& value = m_positions[static_cast<std::size_t>(found)];
    return base::Vec3{value[0], value[1], value[2]};
}

double NodePositions::distance(base::EntityId first, base::EntityId second) const noexcept
{
    if (!has(first) || !has(second))
        return std::numeric_limits<double>::quiet_NaN();
    return base::norm(position(first) - position(second));
}

} // namespace oor::query
