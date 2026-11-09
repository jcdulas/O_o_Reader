#include "oor/model/cards/superelements/superelementBoundarySearch/SuperelementBoundarySearchTable.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 3;

} // namespace

std::optional<SuperelementBoundarySearchCard> SuperelementBoundarySearchFormat::read(FieldReader& reader)
{
    SuperelementBoundarySearchCard card;
    card.superelementIdA = reader.requiredInteger(1, "superelementIdA");
    card.superelementIdB = readSuperelementOrAll(reader, 2, "superelementIdB", std::nullopt);
    card.grids = readIdRuns(reader, FIRST_POINT_FIELD, "boundaryNodeIds");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementBoundarySearchFormat::check(const SuperelementBoundarySearchCard& card)
{
    if (const auto problem = checkSuperelementId(card.superelementIdA, "superelementIdA", false))
        return problem;
    if (card.superelementIdB)
    {
        if (const auto problem = checkSuperelementId(*card.superelementIdB, "superelementIdB", false))
            return problem;
    }
    return checkIdRuns(card.grids, "boundaryNodeIds");
}

void SuperelementBoundarySearchFormat::write(const SuperelementBoundarySearchCard& card, FieldWriter& writer)
{
    writer.integer(1, card.superelementIdA);
    writeSuperelementOrAll(writer, 2, card.superelementIdB, true);
    writeIdRuns(writer, FIRST_POINT_FIELD, card.grids, writer.compressesRanges());
}

std::uint32_t SuperelementBoundarySearchTable::append(const SuperelementBoundarySearchCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_superelementIdA.push(card.superelementIdA);
    m_cards.push(card);
    return row;
}

void SuperelementBoundarySearchTable::store(std::uint32_t row, const SuperelementBoundarySearchCard& card)
{
    m_superelementIdA.set(row, card.superelementIdA);
    m_cards.set(row, card);
}

void SuperelementBoundarySearchTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_superelementIdA.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementBoundarySearchTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    SuperelementBoundarySearchCard& card = m_cards.edit(row);
    visitSuperelement(visitor, "superelementIdA", card.superelementIdA);
    m_superelementIdA.set(row, card.superelementIdA);
    if (card.superelementIdB)
    {
        base::EntityId superelementIdB = *card.superelementIdB;
        visitSuperelement(visitor, "superelementIdB", superelementIdB);
        card.superelementIdB = superelementIdB;
    }
    SuperelementScope scope(visitor, card.superelementIdA);
    visitRunList(scope, "boundaryNodeIds", card.grids, ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
