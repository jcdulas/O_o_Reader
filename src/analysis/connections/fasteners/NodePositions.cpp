#include "NodePositions.hpp"

#include "oor/model/cards/nodes/node/NodeTable.hpp"

namespace oor::analysis::detail
{

NodePositions::NodePositions(const model::Model& model, std::span<const std::array<double, 3>> positions)
    : m_model(model), m_positions(positions), m_nodeType(model::typeIndexOf<model::NodeTable>())
{
}

std::optional<base::Vec3> NodePositions::of(base::EntityId node) const noexcept
{
    if (node == 0)
        return std::nullopt;
    const auto location = m_model.locate(base::IdSpace::Node, node);
    if (!location || location->table != m_nodeType || location->row >= m_positions.size())
        return std::nullopt;
    const std::array<double, 3>& value = m_positions[location->row];
    return base::Vec3{value[0], value[1], value[2]};
}

} // namespace oor::analysis::detail
