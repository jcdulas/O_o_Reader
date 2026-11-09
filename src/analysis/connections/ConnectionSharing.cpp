#include "oor/analysis/connections/ConnectionSharing.hpp"

#include "oor/model/cards/elements/fastener/FastenerTable.hpp"
#include "oor/model/cards/elements/weld/WeldTable.hpp"
#include "oor/model/cards/rigid_elements/rigidInterpolation/RigidInterpolationTable.hpp"

#include <algorithm>
#include <map>
#include <utility>

namespace oor::analysis
{

std::string_view connectionClassName(ConnectionClass kind) noexcept
{
    switch (kind)
    {
    case ConnectionClass::Rigid:
        return "rigid";
    case ConnectionClass::RigidInterpolation:
        return "rigidInterpolation";
    case ConnectionClass::Mpc:
        return "mpc";
    case ConnectionClass::Fastener:
        return "fastener";
    case ConnectionClass::Connector:
        return "connector";
    case ConnectionClass::Contact:
        return "contact";
    }
    return "?";
}

ConnectionSharing ConnectionSharing::build(const ComponentModel& components)
{
    ConnectionSharing result;
    const std::vector<Connection>& connections = components.connections();
    const std::uint16_t interpolation = model::typeIndexOf<model::RigidInterpolationTable>();
    const std::uint16_t fastenerType = model::typeIndexOf<model::FastenerTable>();
    const std::uint16_t weldType = model::typeIndexOf<model::WeldTable>();
    result.m_classes.resize(connections.size());
    result.m_equations.assign(connections.size(), 0);
    std::map<base::EntityId, std::int32_t> equationsOfSet;
    std::vector<std::pair<base::EntityId, std::uint32_t>> pairs;
    for (std::size_t index = 0; index < connections.size(); ++index)
    {
        const Connection& connection = connections[index];
        switch (connection.kind)
        {
        case ConnectionKind::Rigid:
            result.m_classes[index] =
                connection.card.table == interpolation ? ConnectionClass::RigidInterpolation : ConnectionClass::Rigid;
            break;
        case ConnectionKind::Mpc:
            result.m_classes[index] = ConnectionClass::Mpc;
            result.m_equations[index] = ++equationsOfSet[connection.id];
            break;
        case ConnectionKind::Fastener:
            result.m_classes[index] = connection.card.table == fastenerType || connection.card.table == weldType
                                          ? ConnectionClass::Fastener
                                          : ConnectionClass::Connector;
            break;
        case ConnectionKind::Line:
        case ConnectionKind::Mass:
            result.m_classes[index] = ConnectionClass::Connector;
            break;
        case ConnectionKind::Contact:
            result.m_classes[index] = ConnectionClass::Contact;
            break;
        }
        for (const base::EntityId node : components.connectionNodes(index))
        {
            if (node != 0)
                pairs.emplace_back(node, static_cast<std::uint32_t>(index));
        }
    }
    std::sort(pairs.begin(), pairs.end());
    pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
    for (std::size_t position = 0; position < pairs.size(); ++position)
    {
        if (result.m_nodes.empty() || result.m_nodes.back() != pairs[position].first)
        {
            result.m_start.push_back(static_cast<std::uint32_t>(position));
            result.m_nodes.push_back(pairs[position].first);
        }
        result.m_connections.push_back(pairs[position].second);
    }
    result.m_start.push_back(static_cast<std::uint32_t>(pairs.size()));
    return result;
}

std::span<const std::uint32_t> ConnectionSharing::connectionsAt(base::EntityId node) const noexcept
{
    const auto found = std::lower_bound(m_nodes.begin(), m_nodes.end(), node);
    if (found == m_nodes.end() || *found != node)
        return {};
    const auto position = static_cast<std::size_t>(found - m_nodes.begin());
    return std::span<const std::uint32_t>(m_connections)
        .subspan(m_start[position], m_start[position + 1] - m_start[position]);
}

std::vector<std::uint32_t> ConnectionSharing::othersAt(std::span<const base::EntityId> nodes, std::uint32_t self,
                                                       std::vector<base::EntityId>* blockingNodes) const
{
    std::vector<std::uint32_t> others;
    for (const base::EntityId node : nodes)
    {
        bool blocked = false;
        for (const std::uint32_t connection : connectionsAt(node))
        {
            if (connection == self)
                continue;
            others.push_back(connection);
            blocked = true;
        }
        if (blocked && blockingNodes != nullptr)
            blockingNodes->push_back(node);
    }
    std::sort(others.begin(), others.end());
    others.erase(std::unique(others.begin(), others.end()), others.end());
    if (blockingNodes != nullptr)
    {
        std::sort(blockingNodes->begin(), blockingNodes->end());
        blockingNodes->erase(std::unique(blockingNodes->begin(), blockingNodes->end()), blockingNodes->end());
    }
    return others;
}

} // namespace oor::analysis
