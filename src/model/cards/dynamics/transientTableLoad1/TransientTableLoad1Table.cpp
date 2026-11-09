#include "oor/model/cards/dynamics/transientTableLoad1/TransientTableLoad1Table.hpp"

namespace oor::model
{

std::optional<TransientTableLoad1Card> TransientTableLoad1Format::read(FieldReader& reader)
{
    TransientTableLoad1Card card;
    card.id = reader.requiredInteger(1, "id");
    card.excitationId = reader.requiredInteger(2, "excitationId");
    readIdOrReal(reader, 3, "delayId", card.delayId, card.delay);
    card.kind = readExcitationType(reader, 4);
    readIdOrReal(reader, 5, "tableId", card.tableId, card.value);
    card.initialDisplacementFactor = reader.real(6, "initialDisplacementFactor", 0.0);
    card.initialVelocityFactor = reader.real(7, "initialVelocityFactor", 0.0);
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("initialVelocityFactor"),
                     "initialVelocityFactor");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TransientTableLoad1Format::check(const TransientTableLoad1Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the dynamic load set ID must be a positive integer", "id"};
    if (card.excitationId <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "excitationId"};
    if (const auto problem = checkIdOrReal(card.delayId, card.delay, "delayId"))
        return problem;
    if (const auto problem = checkIdOrReal(card.tableId, card.value, "tableId"))
        return problem;
    // The function of time is a table (TID) or a constant that is not zero (F), except for the excitations of SOL 700
    // that name a flow boundary or a user-defined load (FLOW, FLOWT, FLOWUDS, FORCUDS): their TLOAD1 has neither, and a
    // card cannot tell them apart. The check `load.transient-table-load1-no-function` looks at what EXCITEID names.
    return std::nullopt;
}

void TransientTableLoad1Format::write(const TransientTableLoad1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.excitationId);
    writeIdOrReal(writer, 3, card.delayId, card.delay);
    writeExcitationType(writer, 4, card.kind);
    writeIdOrReal(writer, 5, card.tableId, card.value);
    writer.realUnlessDefault(6, card.initialDisplacementFactor, 0.0);
    writer.realUnlessDefault(7, card.initialVelocityFactor, 0.0);
}

std::uint32_t TransientTableLoad1Table::append(const TransientTableLoad1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_excitationId.push(card.excitationId);
    m_delayId.push(card.delayId);
    m_delay.push(card.delay);
    m_kind.push(static_cast<std::uint8_t>(card.kind));
    m_tableId.push(card.tableId);
    m_value.push(card.value);
    m_initialDisplacementFactor.push(card.initialDisplacementFactor);
    m_initialVelocityFactor.push(card.initialVelocityFactor);
    return row;
}

TransientTableLoad1Card TransientTableLoad1Table::card(std::uint32_t row) const
{
    TransientTableLoad1Card card;
    card.id = m_id.get(row);
    card.excitationId = m_excitationId.get(row);
    card.delayId = m_delayId.get(row);
    card.delay = m_delay.get(row);
    card.kind = static_cast<ExcitationType>(m_kind.get(row));
    card.tableId = m_tableId.get(row);
    card.value = m_value.get(row);
    card.initialDisplacementFactor = m_initialDisplacementFactor.get(row);
    card.initialVelocityFactor = m_initialVelocityFactor.get(row);
    return card;
}

void TransientTableLoad1Table::store(std::uint32_t row, const TransientTableLoad1Card& card)
{
    m_id.set(row, card.id);
    m_excitationId.set(row, card.excitationId);
    m_delayId.set(row, card.delayId);
    m_delay.set(row, card.delay);
    m_kind.set(row, static_cast<std::uint8_t>(card.kind));
    m_tableId.set(row, card.tableId);
    m_value.set(row, card.value);
    m_initialDisplacementFactor.set(row, card.initialDisplacementFactor);
    m_initialVelocityFactor.set(row, card.initialVelocityFactor);
}

void TransientTableLoad1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_excitationId.keepRows(rows);
    m_delayId.keepRows(rows);
    m_delay.keepRows(rows);
    m_kind.keepRows(rows);
    m_tableId.keepRows(rows);
    m_value.keepRows(rows);
    m_initialDisplacementFactor.keepRows(rows);
    m_initialVelocityFactor.keepRows(rows);
}

void TransientTableLoad1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    const auto type = static_cast<ExcitationType>(m_kind.get(row));
    m_excitationId.set(row, visitIfSet(visitor, "excitationId", excitationTarget(type), m_excitationId.get(row)));
    // The DELAY card is not a typed card yet: its ID is kept as a plain ID.
    m_delayId.set(row, visitIfSet(visitor, "delayId", ReferenceTarget::DelaySet, m_delayId.get(row)));
    m_tableId.set(row, visitIfSet(visitor, "tableId", ReferenceTarget::DynamicTable, m_tableId.get(row)));
}

} // namespace oor::model
