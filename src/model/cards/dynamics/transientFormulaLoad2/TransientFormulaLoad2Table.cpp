#include "oor/model/cards/dynamics/transientFormulaLoad2/TransientFormulaLoad2Table.hpp"

namespace oor::model
{

std::optional<TransientFormulaLoad2Card> TransientFormulaLoad2Format::read(FieldReader& reader)
{
    TransientFormulaLoad2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.excitationId = reader.requiredInteger(2, "excitationId");
    readIdOrReal(reader, 3, "delayId", card.delayId, card.delay);
    card.kind = readExcitationType(reader, 4);
    card.time1 = reader.real(5, "time1", 0.0);
    card.time2 = reader.real(6, "time2", card.time1); // blank: T1
    card.frequency = reader.real(7, "frequency", 0.0);
    card.phase = reader.real(8, "phase", 0.0);
    card.coefficientC = reader.real(9, "coefficientC", 0.0);
    card.exponentB = reader.real(10, "exponentB", 0.0);
    card.initialDisplacementFactor = reader.real(11, "initialDisplacementFactor", 0.0);
    card.initialVelocityFactor = reader.real(12, "initialVelocityFactor", 0.0);
    if (reader.lastUsedField() > 12)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("initialVelocityFactor"),
                     "initialVelocityFactor");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TransientFormulaLoad2Format::check(const TransientFormulaLoad2Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the dynamic load set ID must be a positive integer", "id"};
    if (card.excitationId <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "excitationId"};
    if (const auto problem = checkIdOrReal(card.delayId, card.delay, "delayId"))
        return problem;
    if (!(card.time1 >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "time1"};
    if (!(card.time2 >= card.time1))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must not be below " + CardProblem::fieldNamed("time1"),
                           "time2"};
    return std::nullopt;
}

void TransientFormulaLoad2Format::write(const TransientFormulaLoad2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.excitationId);
    writeIdOrReal(writer, 3, card.delayId, card.delay);
    writeExcitationType(writer, 4, card.kind);
    writer.realUnlessDefault(5, card.time1, 0.0);
    writer.real(6, card.time2); // depends on T1: always written
    writer.realUnlessDefault(7, card.frequency, 0.0);
    writer.realUnlessDefault(8, card.phase, 0.0);
    writer.realUnlessDefault(9, card.coefficientC, 0.0);
    writer.realUnlessDefault(10, card.exponentB, 0.0);
    writer.realUnlessDefault(11, card.initialDisplacementFactor, 0.0);
    writer.realUnlessDefault(12, card.initialVelocityFactor, 0.0);
}

std::uint32_t TransientFormulaLoad2Table::append(const TransientFormulaLoad2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_excitationId.push(card.excitationId);
    m_delayId.push(card.delayId);
    m_delay.push(card.delay);
    m_kind.push(static_cast<std::uint8_t>(card.kind));
    m_time1.push(card.time1);
    m_time2.push(card.time2);
    m_frequency.push(card.frequency);
    m_phase.push(card.phase);
    m_coefficientC.push(card.coefficientC);
    m_exponentB.push(card.exponentB);
    m_initialDisplacementFactor.push(card.initialDisplacementFactor);
    m_initialVelocityFactor.push(card.initialVelocityFactor);
    return row;
}

TransientFormulaLoad2Card TransientFormulaLoad2Table::card(std::uint32_t row) const
{
    TransientFormulaLoad2Card card;
    card.id = m_id.get(row);
    card.excitationId = m_excitationId.get(row);
    card.delayId = m_delayId.get(row);
    card.delay = m_delay.get(row);
    card.kind = static_cast<ExcitationType>(m_kind.get(row));
    card.time1 = m_time1.get(row);
    card.time2 = m_time2.get(row);
    card.frequency = m_frequency.get(row);
    card.phase = m_phase.get(row);
    card.coefficientC = m_coefficientC.get(row);
    card.exponentB = m_exponentB.get(row);
    card.initialDisplacementFactor = m_initialDisplacementFactor.get(row);
    card.initialVelocityFactor = m_initialVelocityFactor.get(row);
    return card;
}

void TransientFormulaLoad2Table::store(std::uint32_t row, const TransientFormulaLoad2Card& card)
{
    m_id.set(row, card.id);
    m_excitationId.set(row, card.excitationId);
    m_delayId.set(row, card.delayId);
    m_delay.set(row, card.delay);
    m_kind.set(row, static_cast<std::uint8_t>(card.kind));
    m_time1.set(row, card.time1);
    m_time2.set(row, card.time2);
    m_frequency.set(row, card.frequency);
    m_phase.set(row, card.phase);
    m_coefficientC.set(row, card.coefficientC);
    m_exponentB.set(row, card.exponentB);
    m_initialDisplacementFactor.set(row, card.initialDisplacementFactor);
    m_initialVelocityFactor.set(row, card.initialVelocityFactor);
}

void TransientFormulaLoad2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_excitationId.keepRows(rows);
    m_delayId.keepRows(rows);
    m_delay.keepRows(rows);
    m_kind.keepRows(rows);
    m_time1.keepRows(rows);
    m_time2.keepRows(rows);
    m_frequency.keepRows(rows);
    m_phase.keepRows(rows);
    m_coefficientC.keepRows(rows);
    m_exponentB.keepRows(rows);
    m_initialDisplacementFactor.keepRows(rows);
    m_initialVelocityFactor.keepRows(rows);
}

void TransientFormulaLoad2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    const auto type = static_cast<ExcitationType>(m_kind.get(row));
    m_excitationId.set(row, visitIfSet(visitor, "excitationId", excitationTarget(type), m_excitationId.get(row)));
    // The DELAY card is not a typed card yet: its ID is kept as a plain ID.
    m_delayId.set(row, visitIfSet(visitor, "delayId", ReferenceTarget::DelaySet, m_delayId.get(row)));
}

} // namespace oor::model
