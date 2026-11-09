#include "oor/model/cards/dynamics/freqResponseLoad1/FreqResponseLoad1Table.hpp"

namespace oor::model
{

std::optional<FreqResponseLoad1Card> FreqResponseLoad1Format::read(FieldReader& reader)
{
    FreqResponseLoad1Card card;
    card.id = reader.requiredInteger(1, "id");
    card.excitationId = reader.requiredInteger(2, "excitationId");
    readIdOrReal(reader, 3, "delayId", card.delayId, card.delay);
    readIdOrReal(reader, 4, "phaseId", card.phaseId, card.phase);
    readIdOrReal(reader, 5, "realTableId", card.realTableId, card.realValue);
    readIdOrReal(reader, 6, "imaginaryTableId", card.imaginaryTableId, card.imaginaryValue);
    card.kind = readExcitationType(reader, 7);
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("kind"), "kind");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FreqResponseLoad1Format::check(const FreqResponseLoad1Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the dynamic load set ID must be a positive integer", "id"};
    if (card.excitationId <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "excitationId"};
    if (const auto problem = checkIdOrReal(card.delayId, card.delay, "delayId"))
        return problem;
    if (const auto problem = checkIdOrReal(card.phaseId, card.phase, "phaseId"))
        return problem;
    if (const auto problem = checkIdOrReal(card.realTableId, card.realValue, "realTableId"))
        return problem;
    if (const auto problem = checkIdOrReal(card.imaginaryTableId, card.imaginaryValue, "imaginaryTableId"))
        return problem;
    // either TC/RC or TD/RD may be blank or zero, but not both.
    if (card.realTableId == 0 && card.realValue == 0.0 && card.imaginaryTableId == 0 && card.imaginaryValue == 0.0)
        return CardProblem{"card.missing-field",
                           "the card needs " + std::string(CardProblem::FIELD) + " or " +
                               CardProblem::fieldNamed("imaginaryTableId") + " (a table or a number)",
                           "realTableId"};
    if (card.kind != ExcitationType::Load && card.kind != ExcitationType::Displacement &&
        card.kind != ExcitationType::Velocity && card.kind != ExcitationType::Acceleration)
        return CardProblem{"card.bad-value",
                           "the " + std::string(CardProblem::FIELD) + " of the card is 0 to 3 (LOAD, DISP, VELO, ACCE)",
                           "kind"};
    return std::nullopt;
}

void FreqResponseLoad1Format::write(const FreqResponseLoad1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.excitationId);
    writeIdOrReal(writer, 3, card.delayId, card.delay);
    writeIdOrReal(writer, 4, card.phaseId, card.phase);
    writeIdOrReal(writer, 5, card.realTableId, card.realValue);
    writeIdOrReal(writer, 6, card.imaginaryTableId, card.imaginaryValue);
    writeExcitationType(writer, 7, card.kind);
}

std::uint32_t FreqResponseLoad1Table::append(const FreqResponseLoad1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_excitationId.push(card.excitationId);
    m_delayId.push(card.delayId);
    m_delay.push(card.delay);
    m_phaseId.push(card.phaseId);
    m_phase.push(card.phase);
    m_realTableId.push(card.realTableId);
    m_realValue.push(card.realValue);
    m_imaginaryTableId.push(card.imaginaryTableId);
    m_imaginaryValue.push(card.imaginaryValue);
    m_kind.push(static_cast<std::uint8_t>(card.kind));
    return row;
}

FreqResponseLoad1Card FreqResponseLoad1Table::card(std::uint32_t row) const
{
    FreqResponseLoad1Card card;
    card.id = m_id.get(row);
    card.excitationId = m_excitationId.get(row);
    card.delayId = m_delayId.get(row);
    card.delay = m_delay.get(row);
    card.phaseId = m_phaseId.get(row);
    card.phase = m_phase.get(row);
    card.realTableId = m_realTableId.get(row);
    card.realValue = m_realValue.get(row);
    card.imaginaryTableId = m_imaginaryTableId.get(row);
    card.imaginaryValue = m_imaginaryValue.get(row);
    card.kind = static_cast<ExcitationType>(m_kind.get(row));
    return card;
}

void FreqResponseLoad1Table::store(std::uint32_t row, const FreqResponseLoad1Card& card)
{
    m_id.set(row, card.id);
    m_excitationId.set(row, card.excitationId);
    m_delayId.set(row, card.delayId);
    m_delay.set(row, card.delay);
    m_phaseId.set(row, card.phaseId);
    m_phase.set(row, card.phase);
    m_realTableId.set(row, card.realTableId);
    m_realValue.set(row, card.realValue);
    m_imaginaryTableId.set(row, card.imaginaryTableId);
    m_imaginaryValue.set(row, card.imaginaryValue);
    m_kind.set(row, static_cast<std::uint8_t>(card.kind));
}

void FreqResponseLoad1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_excitationId.keepRows(rows);
    m_delayId.keepRows(rows);
    m_delay.keepRows(rows);
    m_phaseId.keepRows(rows);
    m_phase.keepRows(rows);
    m_realTableId.keepRows(rows);
    m_realValue.keepRows(rows);
    m_imaginaryTableId.keepRows(rows);
    m_imaginaryValue.keepRows(rows);
    m_kind.keepRows(rows);
}

void FreqResponseLoad1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    const auto type = static_cast<ExcitationType>(m_kind.get(row));
    m_excitationId.set(row, visitIfSet(visitor, "excitationId", excitationTarget(type), m_excitationId.get(row)));
    // The DELAY and DPHASE cards are not typed cards yet: their IDs are kept as plain IDs.
    m_delayId.set(row, visitIfSet(visitor, "delayId", ReferenceTarget::DelaySet, m_delayId.get(row)));
    m_phaseId.set(row, visitIfSet(visitor, "phaseId", ReferenceTarget::PhaseSet, m_phaseId.get(row)));
    m_realTableId.set(row, visitIfSet(visitor, "realTableId", ReferenceTarget::DynamicTable, m_realTableId.get(row)));
    m_imaginaryTableId.set(
        row, visitIfSet(visitor, "imaginaryTableId", ReferenceTarget::DynamicTable, m_imaginaryTableId.get(row)));
}

} // namespace oor::model
