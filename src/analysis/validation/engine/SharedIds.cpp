#include "oor/analysis/validation/engine/SharedIds.hpp"

#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/core/CardRegistry.hpp"

#include <string>
#include <unordered_map>

namespace oor::analysis
{

namespace
{

// The field of the ID of a card of a category: the deck calls it EID on an element, PID on a property, MID on a
// material, CID on a coordinate system.
} // namespace

void reportIdsSharedByTypes(const ValidationContext& context, FindingSink& sink, model::CardCategory category,
                            std::string_view what, const SkipCard& skip)
{
    const model::Model& model = context.model();
    const model::CardRegistry& registry = model::CardRegistry::instance();
    std::unordered_map<base::EntityId, model::CardLocation> first; // the first card of each ID
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        const model::CardTable* table = model.findTable(type);
        if (table == nullptr || registry.entry(type).info.category != category)
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            const model::CardLocation where{type, row};
            if (skip && skip(model, where))
                continue;
            const base::EntityId id = table->idAt(row);
            const auto [owner, inserted] = first.try_emplace(id, where);
            if (inserted || owner->second.table == type)
                continue;
            sink.reportCard(where, idFieldAt(model, where),
                            "the ID " + std::to_string(id) + " is also the ID of a " +
                                std::string(registry.entry(owner->second.table).info.name) + ": the " +
                                std::string(what) + " IDs must be different",
                            "give one of the cards another ID (renumber the model)");
        }
    }
}

} // namespace oor::analysis
