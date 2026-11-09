#include "oor/model/cards/dynamics/freqResponseLoad2/FreqResponseLoad2Table.hpp"

namespace oor::model
{

std::optional<FreqResponseLoad2Card> FreqResponseLoad2Format::read(FieldReader& reader)
{
    FreqResponseLoad2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.excitationId = reader.requiredInteger(2, "excitationId");
    readIdOrReal(reader, 3, "delayId", card.delayId, card.delay);
    readIdOrReal(reader, 4, "phaseId", card.phaseId, card.phase);
    readIdOrReal(reader, 5, "amplitudeTableId", card.amplitudeTableId, card.amplitude);
    readIdOrReal(reader, 6, "phaseTableId", card.phaseTableId, card.phaseAngle);
    card.kind = readExcitationType(reader, 7);
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("kind"), "kind");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FreqResponseLoad2Format::check(const FreqResponseLoad2Card& card)
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
    if (const auto problem = checkIdOrReal(card.amplitudeTableId, card.amplitude, "amplitudeTableId"))
        return problem;
    if (const auto problem = checkIdOrReal(card.phaseTableId, card.phaseAngle, "phaseTableId"))
        return problem;
    // format reference: B(f) is a table (TB) or a constant that is not zero (RB).
    if (card.amplitudeTableId == 0 && card.amplitude == 0.0)
        return CardProblem{"card.missing-field",
                           "the card needs the table " + std::string(CardProblem::FIELD) + " or a non-zero constant",
                           "amplitudeTableId"};
    if (card.kind != ExcitationType::Load && card.kind != ExcitationType::Displacement &&
        card.kind != ExcitationType::Velocity && card.kind != ExcitationType::Acceleration)
        return CardProblem{"card.bad-value",
                           "the " + std::string(CardProblem::FIELD) + " of the card is 0 to 3 (LOAD, DISP, VELO, ACCE)",
                           "kind"};
    return std::nullopt;
}

void FreqResponseLoad2Format::write(const FreqResponseLoad2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.excitationId);
    writeIdOrReal(writer, 3, card.delayId, card.delay);
    writeIdOrReal(writer, 4, card.phaseId, card.phase);
    writeIdOrReal(writer, 5, card.amplitudeTableId, card.amplitude);
    writeIdOrReal(writer, 6, card.phaseTableId, card.phaseAngle);
    writeExcitationType(writer, 7, card.kind);
}

std::uint32_t FreqResponseLoad2Table::append(const FreqResponseLoad2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_excitationId.push(card.excitationId);
    m_delayId.push(card.delayId);
    m_delay.push(card.delay);
    m_phaseId.push(card.phaseId);
    m_phase.push(card.phase);
    m_amplitudeTableId.push(card.amplitudeTableId);
    m_amplitude.push(card.amplitude);
    m_phaseTableId.push(card.phaseTableId);
    m_phaseAngle.push(card.phaseAngle);
    m_kind.push(static_cast<std::uint8_t>(card.kind));
    return row;
}

FreqResponseLoad2Card FreqResponseLoad2Table::card(std::uint32_t row) const
{
    FreqResponseLoad2Card card;
    card.id = m_id.get(row);
    card.excitationId = m_excitationId.get(row);
    card.delayId = m_delayId.get(row);
    card.delay = m_delay.get(row);
    card.phaseId = m_phaseId.get(row);
    card.phase = m_phase.get(row);
    card.amplitudeTableId = m_amplitudeTableId.get(row);
    card.amplitude = m_amplitude.get(row);
    card.phaseTableId = m_phaseTableId.get(row);
    card.phaseAngle = m_phaseAngle.get(row);
    card.kind = static_cast<ExcitationType>(m_kind.get(row));
    return card;
}

void FreqResponseLoad2Table::store(std::uint32_t row, const FreqResponseLoad2Card& card)
{
    m_id.set(row, card.id);
    m_excitationId.set(row, card.excitationId);
    m_delayId.set(row, card.delayId);
    m_delay.set(row, card.delay);
    m_phaseId.set(row, card.phaseId);
    m_phase.set(row, card.phase);
    m_amplitudeTableId.set(row, card.amplitudeTableId);
    m_amplitude.set(row, card.amplitude);
    m_phaseTableId.set(row, card.phaseTableId);
    m_phaseAngle.set(row, card.phaseAngle);
    m_kind.set(row, static_cast<std::uint8_t>(card.kind));
}

void FreqResponseLoad2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_excitationId.keepRows(rows);
    m_delayId.keepRows(rows);
    m_delay.keepRows(rows);
    m_phaseId.keepRows(rows);
    m_phase.keepRows(rows);
    m_amplitudeTableId.keepRows(rows);
    m_amplitude.keepRows(rows);
    m_phaseTableId.keepRows(rows);
    m_phaseAngle.keepRows(rows);
    m_kind.keepRows(rows);
}

void FreqResponseLoad2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    const auto type = static_cast<ExcitationType>(m_kind.get(row));
    m_excitationId.set(row, visitIfSet(visitor, "excitationId", excitationTarget(type), m_excitationId.get(row)));
    // The DELAY and DPHASE cards are not typed cards yet: their IDs are kept as plain IDs.
    m_delayId.set(row, visitIfSet(visitor, "delayId", ReferenceTarget::DelaySet, m_delayId.get(row)));
    m_phaseId.set(row, visitIfSet(visitor, "phaseId", ReferenceTarget::PhaseSet, m_phaseId.get(row)));
    m_amplitudeTableId.set(
        row, visitIfSet(visitor, "amplitudeTableId", ReferenceTarget::DynamicTable, m_amplitudeTableId.get(row)));
    m_phaseTableId.set(row,
                       visitIfSet(visitor, "phaseTableId", ReferenceTarget::DynamicTable, m_phaseTableId.get(row)));
}

} // namespace oor::model
