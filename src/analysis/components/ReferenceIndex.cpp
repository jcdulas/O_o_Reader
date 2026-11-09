#include "oor/analysis/components/ReferenceIndex.hpp"

#include "oor/model/references/ReferenceChecker.hpp"

namespace oor::analysis
{

ReferenceIndex ReferenceIndex::build(const model::Model& model)
{
    const model::Model copy = model.copy();
    ReferenceIndex index;
    const std::size_t typeCount = model::CardRegistry::instance().size();
    index.m_start.resize(typeCount);
    index.m_referenced.resize(typeCount);
    for (std::size_t type = 0; type < typeCount; ++type)
    {
        const model::CardTable* table = copy.findTable(static_cast<std::uint16_t>(type));
        const std::size_t rows = table == nullptr ? 0 : table->rows().rowCount();
        index.m_start[type].assign(rows + 1, 0);
        index.m_referenced[type].assign(rows, 0);
    }
    // The scan visits the cards in table then row order, so the edges of a card come one after the other.
    model::ReferenceResolver resolver(copy);
    model::ReferenceChecker::scan(
        copy,
        [&](const model::ReferenceUse& use)
        {
            if (use.inCaseControl)
            {
                if (!use.resolved)
                    index.m_dangling.push_back(DanglingReference{
                        {}, std::string(use.field), use.target, use.id, true, std::string(use.scope), use.scopeNumber});
                return;
            }
            ReferenceEdge edge;
            edge.target = use.target;
            edge.id = use.id;
            if (const auto found = resolver.resolve(use.target, use.id))
            {
                edge.to = *found;
                edge.resolved = true;
            }
            if (!use.resolved)
                index.m_dangling.push_back(
                    DanglingReference{use.location, std::string(use.field), use.target, use.id, false, {}, 0});
            index.m_edges.push_back(edge);
            ++index.m_start[use.location.table][use.location.row + 1];
        });
    // The counts become global offsets: the edges of a card are [start[row], start[row + 1]).
    std::uint32_t running = 0;
    for (std::size_t type = 0; type < typeCount; ++type)
    {
        std::vector<std::uint32_t>& start = index.m_start[type];
        start[0] = running;
        for (std::size_t row = 0; row + 1 < start.size(); ++row)
        {
            running += start[row + 1];
            start[row + 1] = running;
        }
    }
    for (const ReferenceEdge& edge : index.m_edges)
    {
        if (edge.resolved && edge.to.row < index.m_referenced[edge.to.table].size())
            index.m_referenced[edge.to.table][edge.to.row] = 1;
    }
    return index;
}

std::span<const ReferenceEdge> ReferenceIndex::from(model::CardLocation location) const noexcept
{
    if (location.table >= m_start.size() || location.row + 1 >= m_start[location.table].size())
        return {};
    const std::vector<std::uint32_t>& start = m_start[location.table];
    return std::span<const ReferenceEdge>(m_edges).subspan(start[location.row],
                                                           start[location.row + 1] - start[location.row]);
}

bool ReferenceIndex::isReferenced(model::CardLocation location) const noexcept
{
    return location.table < m_referenced.size() && location.row < m_referenced[location.table].size() &&
           m_referenced[location.table][location.row] != 0;
}

} // namespace oor::analysis
