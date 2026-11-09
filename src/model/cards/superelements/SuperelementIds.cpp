#include "oor/model/cards/superelements/SuperelementIds.hpp"

#include "oor/model/cards/nodes/node/NodeTable.hpp"

#include <algorithm>

namespace oor::model
{

SuperelementIds::SuperelementIds(const Model& model)
{
    const Model& root = model.root();
    for (const std::unique_ptr<Model>& part : root.parts())
    {
        const PartInfo& info = *part->partInfo();
        if (info.kind == "SuperelementPart" && info.massId == 0)
            m_set.insert(static_cast<base::EntityId>(info.id));
    }
    // The cards that define a superelement: the category has a card of every kind that mentions one, and only those
    // that define it have a set ID (IdSpace::Group).
    const CardRegistry& registry = CardRegistry::instance();
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        const CardTypeInfo& info = registry.entry(type).info;
        const CardTable* table = root.findTable(type);
        if (table == nullptr || info.category != CardCategory::Superelement || info.idSpace != base::IdSpace::Group)
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (table->rows().isAlive(row) && table->idAt(row) > 0)
                m_set.insert(table->idAt(row));
        }
    }
    if (const NodeTable* grids = root.findTable<NodeTable>())
    {
        for (std::uint32_t row = 0; row < grids->rows().rowCount(); ++row)
        {
            if (grids->rows().isAlive(row) && grids->superelementId(row) > 0)
                m_set.insert(grids->superelementId(row));
        }
    }
    m_sorted.assign(m_set.begin(), m_set.end());
    std::sort(m_sorted.begin(), m_sorted.end());
}

std::unordered_set<base::EntityId> massIncrementsOf(const Model& model)
{
    std::unordered_set<base::EntityId> increments;
    for (const std::unique_ptr<Model>& part : model.root().parts())
    {
        const PartInfo& info = *part->partInfo();
        if (info.kind == "MassModel" && info.id > 0)
            increments.insert(static_cast<base::EntityId>(info.id));
        else if (info.massId > 0)
            increments.insert(static_cast<base::EntityId>(info.massId));
    }
    return increments;
}

} // namespace oor::model
