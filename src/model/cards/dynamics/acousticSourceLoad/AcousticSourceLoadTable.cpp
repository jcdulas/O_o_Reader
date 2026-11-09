#include "oor/model/cards/dynamics/acousticSourceLoad/AcousticSourceLoadTable.hpp"

#include "oor/model/cards/dynamics/DynamicLoadFields.hpp"

namespace oor::model
{

std::optional<AcousticSourceLoadCard> AcousticSourceLoadFormat::read(FieldReader& reader)
{
    AcousticSourceLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.excitationId = reader.requiredInteger(2, "excitationId");
    readIdOrReal(reader, 3, "delayId", card.delayId, card.delay);
    readIdOrReal(reader, 4, "phaseId", card.phaseId, card.phase);
    readIdOrReal(reader, 5, "powerTableId", card.powerTableId, card.power);
    card.density = reader.requiredReal(6, "density");
    card.bulkModulus = reader.requiredReal(7, "bulkModulus");
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("bulkModulus"), "bulkModulus");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AcousticSourceLoadFormat::check(const AcousticSourceLoadCard& card)
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
    if (const auto problem = checkIdOrReal(card.powerTableId, card.power, "powerTableId"))
        return problem;
    // TP is a table or RP a power that is not zero.
    if (card.powerTableId == 0 && card.power == 0.0)
        return CardProblem{
            "card.missing-field",
            "the card needs a power table " + std::string(CardProblem::FIELD) + " or a power that is not zero",
            "powerTableId"};
    if (!(card.density > 0.0))
        return CardProblem{"card.bad-value", "the density " + std::string(CardProblem::FIELD) + " must be positive",
                           "density"};
    if (!(card.bulkModulus > 0.0))
        return CardProblem{"card.bad-value",
                           "the bulk modulus " + std::string(CardProblem::FIELD) + " must be positive", "bulkModulus"};
    return std::nullopt;
}

void AcousticSourceLoadFormat::write(const AcousticSourceLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.excitationId);
    writeIdOrReal(writer, 3, card.delayId, card.delay);
    writeIdOrReal(writer, 4, card.phaseId, card.phase);
    writeIdOrReal(writer, 5, card.powerTableId, card.power);
    writer.real(6, card.density);
    writer.real(7, card.bulkModulus);
}

std::uint32_t AcousticSourceLoadTable::append(const AcousticSourceLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_excitationId.push(card.excitationId);
    m_delayId.push(card.delayId);
    m_delay.push(card.delay);
    m_phaseId.push(card.phaseId);
    m_phase.push(card.phase);
    m_powerTableId.push(card.powerTableId);
    m_power.push(card.power);
    m_density.push(card.density);
    m_bulkModulus.push(card.bulkModulus);
    return row;
}

AcousticSourceLoadCard AcousticSourceLoadTable::card(std::uint32_t row) const
{
    AcousticSourceLoadCard card;
    card.id = m_id.get(row);
    card.excitationId = m_excitationId.get(row);
    card.delayId = m_delayId.get(row);
    card.delay = m_delay.get(row);
    card.phaseId = m_phaseId.get(row);
    card.phase = m_phase.get(row);
    card.powerTableId = m_powerTableId.get(row);
    card.power = m_power.get(row);
    card.density = m_density.get(row);
    card.bulkModulus = m_bulkModulus.get(row);
    return card;
}

void AcousticSourceLoadTable::store(std::uint32_t row, const AcousticSourceLoadCard& card)
{
    m_id.set(row, card.id);
    m_excitationId.set(row, card.excitationId);
    m_delayId.set(row, card.delayId);
    m_delay.set(row, card.delay);
    m_phaseId.set(row, card.phaseId);
    m_phase.set(row, card.phase);
    m_powerTableId.set(row, card.powerTableId);
    m_power.set(row, card.power);
    m_density.set(row, card.density);
    m_bulkModulus.set(row, card.bulkModulus);
}

void AcousticSourceLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_excitationId.keepRows(rows);
    m_delayId.keepRows(rows);
    m_delay.keepRows(rows);
    m_phaseId.keepRows(rows);
    m_phase.keepRows(rows);
    m_powerTableId.keepRows(rows);
    m_power.keepRows(rows);
    m_density.keepRows(rows);
    m_bulkModulus.keepRows(rows);
}

void AcousticSourceLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // the EXCITEID names a DAREA set, or an SLOAD set (directly, or through the LSEQ that picks it).
    m_excitationId.set(row, visitor.visit("excitationId", ReferenceTarget::ExcitationSet, m_excitationId.get(row)));
    // The DELAY and DPHASE cards are not typed cards yet: their IDs are kept as plain IDs.
    m_delayId.set(row, visitIfSet(visitor, "delayId", ReferenceTarget::DelaySet, m_delayId.get(row)));
    m_phaseId.set(row, visitIfSet(visitor, "phaseId", ReferenceTarget::PhaseSet, m_phaseId.get(row)));
    m_powerTableId.set(row,
                       visitIfSet(visitor, "powerTableId", ReferenceTarget::DynamicTable, m_powerTableId.get(row)));
}

} // namespace oor::model
