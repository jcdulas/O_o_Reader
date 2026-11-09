#include "oor/model/cards/explicit/fluidFilledContainer/FluidFilledContainerTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<FluidFilledContainerCard> FluidFilledContainerFormat::read(FieldReader& reader)
{
    FluidFilledContainerCard card;
    card.id = reader.requiredInteger(1, "id");
    card.surfaceId = reader.requiredInteger(2, "surfaceId");
    card.fluidVolume = reader.requiredReal(3, "fluidVolume");
    card.atmosphericPressure = reader.requiredReal(4, "atmosphericPressure");
    card.temperatureTableId = reader.integer(5, "temperatureTableId", 0);
    card.densityTableId = reader.integer(6, "densityTableId", 0);
    card.activationTime = reader.optionalReal(7, "activationTime");
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 7 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FluidFilledContainerFormat::check(const FluidFilledContainerCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.surfaceId, "surfaceId"))
        return problem;
    if (const auto problem = checkReal(card.fluidVolume, "fluidVolume", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.atmosphericPressure, "atmosphericPressure", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalId(card.temperatureTableId, "temperatureTableId"))
        return problem;
    if (const auto problem = checkOptionalId(card.densityTableId, "densityTableId"))
        return problem;
    return checkOptionalReal(card.activationTime, "activationTime", RealRule::Finite);
}

void FluidFilledContainerFormat::write(const FluidFilledContainerCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.surfaceId);
    writer.real(3, card.fluidVolume);
    writer.real(4, card.atmosphericPressure);
    writer.integerUnlessDefault(5, card.temperatureTableId, 0);
    writer.integerUnlessDefault(6, card.densityTableId, 0);
    if (card.activationTime)
        writer.real(7, *card.activationTime);
}

std::uint32_t FluidFilledContainerTable::append(const FluidFilledContainerCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void FluidFilledContainerTable::store(std::uint32_t row, const FluidFilledContainerCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void FluidFilledContainerTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void FluidFilledContainerTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void FluidFilledContainerTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    FluidFilledContainerCard& card = m_cards.edit(row);
    visitId(visitor, "surfaceId", ReferenceTarget::ContactSurface, card.surfaceId);
    visitId(visitor, "temperatureTableId", ReferenceTarget::DynamicTable, card.temperatureTableId);
    visitId(visitor, "densityTableId", ReferenceTarget::DynamicTable, card.densityTableId);
}

} // namespace oor::model
