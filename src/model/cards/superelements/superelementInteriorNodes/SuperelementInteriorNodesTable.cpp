#include "oor/model/cards/superelements/superelementInteriorNodes/SuperelementInteriorNodesTable.hpp"

#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 2;

} // namespace

std::optional<SuperelementInteriorNodesCard> SuperelementInteriorNodesFormat::read(FieldReader& reader)
{
    SuperelementInteriorNodesCard card;
    card.superelementId = reader.requiredInteger(1, "superelementId");
    card.grids = readIdRuns(reader, FIRST_POINT_FIELD, "nodes");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementInteriorNodesFormat::check(const SuperelementInteriorNodesCard& card)
{
    if (const auto problem = checkSuperelementId(card.superelementId, "superelementId", false))
        return problem;
    return checkIdRuns(card.grids, "nodes");
}

void SuperelementInteriorNodesFormat::write(const SuperelementInteriorNodesCard& card, FieldWriter& writer)
{
    writer.integer(1, card.superelementId);
    writeIdRuns(writer, FIRST_POINT_FIELD, card.grids, writer.compressesRanges());
}

std::uint32_t SuperelementInteriorNodesTable::append(const SuperelementInteriorNodesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_superelementId.push(card.superelementId);
    m_cards.push(card);
    return row;
}

void SuperelementInteriorNodesTable::store(std::uint32_t row, const SuperelementInteriorNodesCard& card)
{
    m_superelementId.set(row, card.superelementId);
    m_cards.set(row, card);
}

void SuperelementInteriorNodesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_superelementId.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementInteriorNodesTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_superelementId.set(row, id);
    m_cards.edit(row).superelementId = id;
}

void SuperelementInteriorNodesTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitRunList(visitor, "nodes", m_cards.edit(row).grids, ReferenceTarget::NodeInRange, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
