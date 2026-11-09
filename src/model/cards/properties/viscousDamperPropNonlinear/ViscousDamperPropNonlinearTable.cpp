#include "oor/model/cards/properties/viscousDamperPropNonlinear/ViscousDamperPropNonlinearTable.hpp"

namespace oor::model
{

std::optional<ViscousDamperPropNonlinearCard> ViscousDamperPropNonlinearFormat::read(FieldReader& reader)
{
    ViscousDamperPropNonlinearCard card;
    card.id = reader.requiredInteger(1, "id");
    card.tableId = reader.requiredInteger(2, "tableId");
    if (reader.lastUsedField() > 2)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 2 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ViscousDamperPropNonlinearFormat::check(const ViscousDamperPropNonlinearCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.tableId <= 0)
        return CardProblem{"card.bad-id",
                           std::string(CardProblem::FIELD) + " must be the positive ID of a " +
                               CardProblem::cardNamed("TableDynamic1"),
                           "tableId"};
    return std::nullopt;
}

void ViscousDamperPropNonlinearFormat::write(const ViscousDamperPropNonlinearCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.tableId);
}

std::uint32_t ViscousDamperPropNonlinearTable::append(const ViscousDamperPropNonlinearCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_tableId.push(card.tableId);
    return row;
}

ViscousDamperPropNonlinearCard ViscousDamperPropNonlinearTable::card(std::uint32_t row) const
{
    return ViscousDamperPropNonlinearCard{m_id.get(row), m_tableId.get(row)};
}

void ViscousDamperPropNonlinearTable::store(std::uint32_t row, const ViscousDamperPropNonlinearCard& card)
{
    m_id.set(row, card.id);
    m_tableId.set(row, card.tableId);
}

void ViscousDamperPropNonlinearTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_tableId.keepRows(rows);
}

void ViscousDamperPropNonlinearTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The force-velocity curve is a TABLED1, the dynamic table space (the TABLEDi cards).
    if (const base::EntityId tableId = m_tableId.get(row); tableId > 0)
        m_tableId.set(row, visitor.visit("tableId", ReferenceTarget::DynamicTable, tableId));
}

} // namespace oor::model
