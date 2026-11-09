#pragma once

#include "oor/analysis/components/ModelEntities.hpp"
#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace oor::analysis
{

// The entities that act on each grid point: the reverse of ModelEntities::nodesOf, built on demand by the code that
// needs it (groups, zones).
class NodeEntityIndex
{
public:
    explicit NodeEntityIndex(const ModelEntities& entities);

    // The entities that act on the node, in increasing order (each once).
    [[nodiscard]] std::span<const std::uint32_t> entitiesAt(base::EntityId node) const noexcept;
    // The nodes some entity acts on, sorted.
    [[nodiscard]] const std::vector<base::EntityId>& nodes() const noexcept { return m_nodes; }

private:
    std::vector<base::EntityId> m_nodes;
    std::vector<std::uint32_t> m_start;
    std::vector<std::uint32_t> m_entities;
};

} // namespace oor::analysis
