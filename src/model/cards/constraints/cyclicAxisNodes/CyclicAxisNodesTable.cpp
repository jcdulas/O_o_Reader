#include "oor/model/cards/constraints/cyclicAxisNodes/CyclicAxisNodesTable.hpp"

#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_GRID_FIELD = 1;

} // namespace

std::optional<CyclicAxisNodesCard> CyclicAxisNodesFormat::read(FieldReader& reader)
{
    CyclicAxisNodesCard card;
    card.grids = readIdRuns(reader, FIRST_GRID_FIELD, "nodes");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CyclicAxisNodesFormat::check(const CyclicAxisNodesCard& card)
{
    return checkIdRuns(card.grids, "nodes");
}

void CyclicAxisNodesFormat::write(const CyclicAxisNodesCard& card, FieldWriter& writer)
{
    writeIdRuns(writer, FIRST_GRID_FIELD, card.grids, writer.compressesRanges());
}

std::uint32_t CyclicAxisNodesTable::append(const CyclicAxisNodesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_itemCount.push(static_cast<std::int32_t>(card.grids.size()));
    m_cards.push(card);
    return row;
}

void CyclicAxisNodesTable::store(std::uint32_t row, const CyclicAxisNodesCard& card)
{
    m_itemCount.set(row, static_cast<std::int32_t>(card.grids.size()));
    m_cards.set(row, card);
}

void CyclicAxisNodesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_itemCount.keepRows(rows);
    m_cards.keepRows(rows);
}

void CyclicAxisNodesTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitRunList(visitor, "nodes", m_cards.edit(row).grids, ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
