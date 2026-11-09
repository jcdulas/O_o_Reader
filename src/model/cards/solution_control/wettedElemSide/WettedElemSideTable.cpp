#include "oor/model/cards/solution_control/wettedElemSide/WettedElemSideTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 3;

} // namespace

std::optional<WettedElemSideCard> WettedElemSideFormat::read(FieldReader& reader)
{
    WettedElemSideCard card;
    card.id = reader.requiredInteger(1, "id");
    card.elemId = reader.requiredInteger(2, "elemId");
    card.sideCode = reader.requiredInteger(3, "sideCode");
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> WettedElemSideFormat::check(const WettedElemSideCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.elemId, "elemId"))
        return problem;
    if (card.sideCode < WettedElemSideCard::FIRST_SIDE || card.sideCode > WettedElemSideCard::LAST_SIDE)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be between 1 and 6", "sideCode"};
    return std::nullopt;
}

void WettedElemSideFormat::write(const WettedElemSideCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.elemId);
    writer.integer(3, card.sideCode);
}

std::uint32_t WettedElemSideTable::append(const WettedElemSideCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void WettedElemSideTable::store(std::uint32_t row, const WettedElemSideCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void WettedElemSideTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void WettedElemSideTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void WettedElemSideTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The WEID is the card's own number; only the structural element is a reference.
    visitId(visitor, "elemId", ReferenceTarget::Element, m_cards.edit(row).elemId);
}

} // namespace oor::model
