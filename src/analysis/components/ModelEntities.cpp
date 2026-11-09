#include "oor/analysis/components/ModelEntities.hpp"

#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/base/threading/ParallelFor.hpp"
#include "oor/model/cards/constraints/multiPointConstr/MultiPointConstrTable.hpp"

#include <algorithm>

namespace oor::analysis
{

ModelEntities ModelEntities::build(const model::Model& model, const FastenerPiercings* piercings,
                                   base::ThreadPool* pool)
{
    ModelEntities result;
    const model::CardRegistry& registry = model::CardRegistry::instance();
    result.m_indexOfRow.resize(registry.size());
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        const ElementRole role = roleOfType(type);
        if (role == ElementRole::None)
            continue;
        const model::CardTable* table = model.findTable(type);
        if (table == nullptr)
            continue;
        result.m_indexOfRow[type].assign(table->rows().rowCount(), 0);
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            result.m_entities.push_back(ModelEntity{model::CardLocation{type, row}, table->idAt(row), role, false});
            result.m_indexOfRow[type][row] = static_cast<std::uint32_t>(result.m_entities.size());
        }
    }
    const std::size_t elementCount = result.m_entities.size();

    // The grid points of the elements, by chunks over the pool, then joined in order.
    constexpr std::size_t CHUNK = 16384;
    const std::size_t chunks = (elementCount + CHUNK - 1) / CHUNK;
    std::vector<std::vector<base::EntityId>> parts(chunks);
    std::vector<std::vector<std::uint32_t>> sizes(chunks);
    const auto work = [&](std::size_t chunk)
    {
        std::vector<base::EntityId> nodes;
        const std::size_t begin = chunk * CHUNK;
        const std::size_t end = std::min(begin + CHUNK, elementCount);
        for (std::size_t index = begin; index < end; ++index)
        {
            const ModelEntity& entity = result.m_entities[index];
            const std::size_t before = parts[chunk].size();
            const PiercedFastener* fastener = piercings != nullptr ? piercings->find(entity.card) : nullptr;
            nodes.clear();
            if (fastener != nullptr)
                nodes = piercings->connectionNodes(*fastener);
            if (nodes.empty() && ElementTopology::supports(entity.card.table))
                ElementTopology::nodesOf(*model.findTable(entity.card.table), entity.card.table, entity.card.row,
                                         nodes);
            parts[chunk].insert(parts[chunk].end(), nodes.begin(), nodes.end());
            sizes[chunk].push_back(static_cast<std::uint32_t>(parts[chunk].size() - before));
        }
    };
    if (pool != nullptr && chunks > 1)
        base::parallelFor(*pool, chunks, work);
    else
    {
        for (std::size_t chunk = 0; chunk < chunks; ++chunk)
            work(chunk);
    }
    result.m_start.reserve(elementCount + 1);
    result.m_start.push_back(0);
    for (std::size_t chunk = 0; chunk < chunks; ++chunk)
    {
        for (const std::uint32_t size : sizes[chunk])
            result.m_start.push_back(result.m_start.back() + size);
        result.m_nodes.insert(result.m_nodes.end(), parts[chunk].begin(), parts[chunk].end());
    }

    // The MPC equations: one per MPC card, on the grid points of its terms.
    if (const model::MultiPointConstrTable* table = model.findTable<model::MultiPointConstrTable>())
    {
        const std::uint16_t type = model::typeIndexOf<model::MultiPointConstrTable>();
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            const model::MultiPointConstrCard card = table->card(row);
            result.m_entities.push_back(ModelEntity{model::CardLocation{type, row}, card.id, ElementRole::None, true});
            for (const model::MultiPointConstrTerm& term : card.terms)
                result.m_nodes.push_back(term.node);
            result.m_start.push_back(static_cast<std::uint32_t>(result.m_nodes.size()));
        }
    }
    return result;
}

std::optional<std::size_t> ModelEntities::indexOf(model::CardLocation card) const noexcept
{
    if (card.table >= m_indexOfRow.size() || card.row >= m_indexOfRow[card.table].size())
        return std::nullopt;
    const std::uint32_t position = m_indexOfRow[card.table][card.row];
    if (position == 0)
        return std::nullopt;
    return position - 1;
}

} // namespace oor::analysis
