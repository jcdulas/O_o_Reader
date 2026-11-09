#include "oor/model/cards/superelements/superelementElemTransfer/SuperelementElemTransferTable.hpp"

#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_ELEMENT_FIELD = 2;

} // namespace

std::optional<SuperelementElemTransferCard> SuperelementElemTransferFormat::read(FieldReader& reader)
{
    SuperelementElemTransferCard card;
    card.superelementId = reader.requiredInteger(1, "superelementId");
    card.elements = readIdRuns(reader, FIRST_ELEMENT_FIELD, "elements");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementElemTransferFormat::check(const SuperelementElemTransferCard& card)
{
    if (const auto problem = checkSuperelementId(card.superelementId, "superelementId", true))
        return problem;
    return checkIdRuns(card.elements, "elements");
}

void SuperelementElemTransferFormat::write(const SuperelementElemTransferCard& card, FieldWriter& writer)
{
    writer.integer(1, card.superelementId);
    writeIdRuns(writer, FIRST_ELEMENT_FIELD, card.elements, writer.compressesRanges());
}

std::uint32_t SuperelementElemTransferTable::append(const SuperelementElemTransferCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_superelementId.push(card.superelementId);
    m_cards.push(card);
    return row;
}

void SuperelementElemTransferTable::store(std::uint32_t row, const SuperelementElemTransferCard& card)
{
    m_superelementId.set(row, card.superelementId);
    m_cards.set(row, card);
}

void SuperelementElemTransferTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_superelementId.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementElemTransferTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_superelementId.set(row, id);
    m_cards.edit(row).superelementId = id;
}

void SuperelementElemTransferTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitRunList(visitor, "elements", m_cards.edit(row).elements, ReferenceTarget::Element,
                 ReferenceTarget::ElementInRange);
}

} // namespace oor::model
