#include "oor/model/cards/superelements/superelementExteriorPoints/SuperelementExteriorPointsTable.hpp"

#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 2;

} // namespace

std::optional<SuperelementExteriorPointsCard> SuperelementExteriorPointsFormat::read(FieldReader& reader)
{
    SuperelementExteriorPointsCard card;
    card.superelementId = reader.requiredInteger(1, "superelementId");
    card.points = readIdRuns(reader, FIRST_POINT_FIELD, "nodes");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementExteriorPointsFormat::check(const SuperelementExteriorPointsCard& card)
{
    if (const auto problem = checkSuperelementId(card.superelementId, "superelementId", true))
        return problem;
    return checkIdRuns(card.points, "nodes");
}

void SuperelementExteriorPointsFormat::write(const SuperelementExteriorPointsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.superelementId);
    writeIdRuns(writer, FIRST_POINT_FIELD, card.points, writer.compressesRanges());
}

std::uint32_t SuperelementExteriorPointsTable::append(const SuperelementExteriorPointsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_superelementId.push(card.superelementId);
    m_cards.push(card);
    return row;
}

void SuperelementExteriorPointsTable::store(std::uint32_t row, const SuperelementExteriorPointsCard& card)
{
    m_superelementId.set(row, card.superelementId);
    m_cards.set(row, card);
}

void SuperelementExteriorPointsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_superelementId.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementExteriorPointsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_superelementId.set(row, id);
    m_cards.edit(row).superelementId = id;
}

void SuperelementExteriorPointsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitRunList(visitor, "nodes", m_cards.edit(row).points, ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
