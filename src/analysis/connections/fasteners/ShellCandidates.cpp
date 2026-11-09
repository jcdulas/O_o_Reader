#include "ShellCandidates.hpp"

#include <algorithm>

namespace oor::analysis::detail
{

ShellCandidates::ShellCandidates(const model::Model& model, const ShellElements& elements,
                                 const NodeToElementIndex& nodeIndex)
    : m_model(model), m_elements(elements), m_nodeIndex(nodeIndex)
{
}

std::vector<ShellElement> ShellCandidates::ringOf(const ShellElement& element) const
{
    std::vector<model::CardLocation> found;
    for (std::size_t slot = 0; slot < element.slotCount(); ++slot)
    {
        if (!element.hasNode(slot))
            continue;
        for (const ElementRef& neighbour : m_nodeIndex.elementsOf(element.nodes[slot]))
        {
            const model::CardLocation location{neighbour.type, neighbour.row};
            if (!ShellElements::isShellType(neighbour.type) || (!element.isVirtual && location == element.location))
                continue;
            if (std::find(found.begin(), found.end(), location) == found.end())
                found.push_back(location);
        }
    }
    std::vector<ShellElement> ring;
    ring.reserve(found.size());
    for (const model::CardLocation location : found)
    {
        if (auto neighbour = m_elements.read(location))
            ring.push_back(std::move(*neighbour));
    }
    std::sort(ring.begin(), ring.end(),
              [](const ShellElement& left, const ShellElement& right) { return left.eid < right.eid; });
    return ring;
}

const PropertyShellIndex* ShellCandidates::property(base::EntityId pid) const noexcept
{
    const auto found = m_properties.find(pid);
    return found == m_properties.end() || found->second.isEmpty() ? nullptr : &found->second;
}

void ShellCandidates::prepare(const std::vector<base::EntityId>& pids)
{
    std::vector<base::EntityId> missing;
    for (const base::EntityId pid : pids)
    {
        if (!m_properties.contains(pid) && std::find(missing.begin(), missing.end(), pid) == missing.end())
            missing.push_back(pid);
    }
    if (missing.empty())
        return;
    std::sort(missing.begin(), missing.end());
    std::map<base::EntityId, std::vector<ShellElement>> byProperty;
    const model::CardRegistry& registry = model::CardRegistry::instance();
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        if (!ShellElements::isShellType(type))
            continue;
        const model::CardTable* table = m_model.findTable(type);
        if (table == nullptr)
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            auto element = m_elements.read(model::CardLocation{type, row});
            if (element && std::binary_search(missing.begin(), missing.end(), element->pid))
                byProperty[element->pid].push_back(std::move(*element));
        }
    }
    for (const base::EntityId pid : missing)
        m_properties.emplace(pid, PropertyShellIndex(std::move(byProperty[pid])));
}

std::optional<HostedPoint> ShellCandidates::nearestInside(const std::vector<ShellElement>& candidates,
                                                          const base::Vec3& point, double tolerance)
{
    std::optional<HostedPoint> best;
    for (const ShellElement& element : candidates)
    {
        const SurfaceProjection projection = SurfaceProjection::onto(element, point);
        if (!projection.isInside(tolerance))
            continue;
        if (best)
        {
            const double held = best->projection.distance;
            const bool same = isSameDistance(projection.distance, held);
            if ((same && element.eid >= best->element.eid) || (!same && projection.distance > held))
                continue;
        }
        best = HostedPoint{element, projection};
    }
    return best;
}

} // namespace oor::analysis::detail
