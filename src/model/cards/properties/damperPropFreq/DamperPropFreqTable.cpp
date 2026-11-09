#include "oor/model/cards/properties/damperPropFreq/DamperPropFreqTable.hpp"

namespace oor::model
{

std::optional<DamperPropFreqCard> DamperPropFreqFormat::read(FieldReader& reader)
{
    DamperPropFreqCard card;
    card.id = reader.requiredInteger(1, "id");
    card.dampingTableId = reader.integer(2, "dampingTableId", 0);
    if (reader.lastUsedField() > 2)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has two fields", "dampingTableId");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DamperPropFreqFormat::check(const DamperPropFreqCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.dampingTableId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "dampingTableId"};
    return std::nullopt;
}

void DamperPropFreqFormat::write(const DamperPropFreqCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.dampingTableId, 0);
}

std::uint32_t DamperPropFreqTable::append(const DamperPropFreqCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_dampingTableId.push(card.dampingTableId);
    return row;
}

DamperPropFreqCard DamperPropFreqTable::card(std::uint32_t row) const
{
    return DamperPropFreqCard{m_id.get(row), m_dampingTableId.get(row)};
}

void DamperPropFreqTable::store(std::uint32_t row, const DamperPropFreqCard& card)
{
    m_id.set(row, card.id);
    m_dampingTableId.set(row, card.dampingTableId);
}

void DamperPropFreqTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_dampingTableId.keepRows(rows);
}

void DamperPropFreqTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the PDAMP that the card extends.
    m_id.set(row, visitor.visit("id", ReferenceTarget::Property, m_id.get(row)));
    if (const base::EntityId dampingTableId = m_dampingTableId.get(row); dampingTableId > 0)
        m_dampingTableId.set(row, visitor.visit("dampingTableId", ReferenceTarget::DynamicTable, dampingTableId));
}

} // namespace oor::model
