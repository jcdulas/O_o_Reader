#pragma once

#include "oor/analysis/validation/engine/ValidationSupport.hpp"

#include <algorithm>

namespace oor::analysis
{

// Calls fn(location) for every element of the model whose ID is in [first, last], in increasing ID order for a short
// range. A range that is long (a THRU over millions of IDs, most of them unused) is walked through the elements instead
// of the IDs.
template <typename Fn>
void forEachElementInRange(const model::Model& model, base::EntityId first, base::EntityId last, Fn&& fn)
{
    constexpr std::int64_t SHORT_RANGE = 2'000'000;
    if (last < first)
        last = first;
    if (static_cast<std::int64_t>(last) - first <= SHORT_RANGE)
    {
        for (std::int64_t id = first; id <= last; ++id)
        {
            if (const std::optional<model::CardLocation> found =
                    model.locate(base::IdSpace::Element, static_cast<base::EntityId>(id)))
                fn(*found);
        }
        return;
    }
    const model::CardRegistry& registry = model::CardRegistry::instance();
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        const model::CardTable* table = model.findTable(type);
        if (table == nullptr || registry.entry(type).info.idSpace != base::IdSpace::Element)
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (table->rows().isAlive(row) && table->idAt(row) >= first && table->idAt(row) <= last)
                fn(model::CardLocation{type, row});
        }
    }
}

} // namespace oor::analysis
