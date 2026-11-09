#include "oor/analysis/components/NodeEntityIndex.hpp"

#include <algorithm>
#include <utility>

namespace oor::analysis
{

NodeEntityIndex::NodeEntityIndex(const ModelEntities& entities)
{
    std::vector<std::pair<base::EntityId, std::uint32_t>> pairs;
    for (std::size_t index = 0; index < entities.size(); ++index)
    {
        for (const base::EntityId node : entities.nodesOf(index))
        {
            if (node != 0)
                pairs.emplace_back(node, static_cast<std::uint32_t>(index));
        }
    }
    std::sort(pairs.begin(), pairs.end());
    pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
    m_start.push_back(0);
    for (std::size_t position = 0; position < pairs.size(); ++position)
    {
        if (m_nodes.empty() || m_nodes.back() != pairs[position].first)
        {
            if (!m_nodes.empty())
                m_start.push_back(static_cast<std::uint32_t>(position));
            m_nodes.push_back(pairs[position].first);
        }
        m_entities.push_back(pairs[position].second);
    }
    if (!m_nodes.empty())
        m_start.push_back(static_cast<std::uint32_t>(pairs.size()));
}

std::span<const std::uint32_t> NodeEntityIndex::entitiesAt(base::EntityId node) const noexcept
{
    const auto found = std::lower_bound(m_nodes.begin(), m_nodes.end(), node);
    if (found == m_nodes.end() || *found != node)
        return {};
    const auto position = static_cast<std::size_t>(found - m_nodes.begin());
    return std::span<const std::uint32_t>(m_entities)
        .subspan(m_start[position], m_start[position + 1] - m_start[position]);
}

} // namespace oor::analysis
