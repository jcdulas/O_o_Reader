#include "oor/analysis/components/ComponentChanges.hpp"

#include "oor/analysis/components/ComponentCache.hpp"

namespace oor::analysis
{

namespace
{

// Two partitions are the same when every component has the same size and the same smallest element: the numbering is
// a function of the connectivity alone (by decreasing element count, ties by the smallest element ID), so an edit that
// keeps the connectivity keeps them all.
bool sameComponents(const ComponentModel& left, const ComponentModel& right)
{
    if (left.componentCount() != right.componentCount())
        return false;
    for (std::size_t index = 0; index < left.componentCount(); ++index)
    {
        const ComponentSummary& a = left.components()[index];
        const ComponentSummary& b = right.components()[index];
        if (a.elementCount != b.elementCount || a.nodeCount != b.nodeCount ||
            a.smallestElementId != b.smallestElementId)
            return false;
    }
    return true;
}

} // namespace

std::vector<std::int32_t> ComponentChanges::between(const model::Model& before, const model::Model& after,
                                                    base::ThreadPool* pool)
{
    ComponentCache& beforeCache = ComponentCache::of(before);
    ComponentCache& afterCache = ComponentCache::of(after);
    const std::shared_ptr<const ComponentModel> oldComponents = beforeCache.components(before, pool);
    const std::shared_ptr<const ComponentModel> newComponents = afterCache.components(after, pool);
    std::vector<std::int32_t> changed;
    if (!sameComponents(*oldComponents, *newComponents))
    {
        for (const ComponentSummary& component : newComponents->components())
            changed.push_back(component.id);
        return changed;
    }
    const std::shared_ptr<const std::vector<std::uint64_t>> oldStamps = beforeCache.stamps(before, pool);
    const std::shared_ptr<const std::vector<std::uint64_t>> newStamps = afterCache.stamps(after, pool);
    for (std::size_t index = 0; index < newStamps->size(); ++index)
    {
        if ((*oldStamps)[index] != (*newStamps)[index])
            changed.push_back(static_cast<std::int32_t>(index + 1));
    }
    return changed;
}

} // namespace oor::analysis
