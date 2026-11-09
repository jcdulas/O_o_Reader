#include "oor/model/cards/explicit/steinbergGuinanYield/SteinbergGuinanYieldTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<SteinbergGuinanYieldCard> SteinbergGuinanYieldFormat::read(FieldReader& reader)
{
    SteinbergGuinanYieldCard card;
    card.id = reader.requiredInteger(1, "id");
    card.coefficientA1 = reader.requiredReal(2, "coefficientA1");
    card.coefficientA2 = reader.requiredReal(3, "coefficientA2");
    card.coefficientA3 = reader.requiredReal(4, "coefficientA3");
    card.coefficientA4 = reader.requiredReal(5, "coefficientA4");
    card.coefficientH = reader.real(6, "coefficientH", 0.0);
    card.coefficientB = reader.real(7, "coefficientB", 0.0);
    card.specificHeat = reader.real(8, "specificHeat", STEINBERG_GUINAN_YIELD_NO_LIMIT);
    card.meltTemperature = reader.real(9, "meltTemperature", STEINBERG_GUINAN_YIELD_NO_LIMIT);
    card.roomTemperature = reader.real(10, "roomTemperature", STEINBERG_GUINAN_YIELD_ROOM_TEMPERATURE);
    if (reader.lastUsedField() > 10)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 10 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SteinbergGuinanYieldFormat::check(const SteinbergGuinanYieldCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.coefficientA1, "coefficientA1", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.coefficientA2, "coefficientA2", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.coefficientA3, "coefficientA3", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.coefficientA4, "coefficientA4", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.coefficientH, "coefficientH", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientB, "coefficientB", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.specificHeat, "specificHeat", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.meltTemperature, "meltTemperature", RealRule::Finite))
        return problem;
    return checkReal(card.roomTemperature, "roomTemperature", RealRule::Finite);
}

void SteinbergGuinanYieldFormat::write(const SteinbergGuinanYieldCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.coefficientA1);
    writer.real(3, card.coefficientA2);
    writer.real(4, card.coefficientA3);
    writer.real(5, card.coefficientA4);
    writer.realUnlessDefault(6, card.coefficientH, 0.0);
    writer.realUnlessDefault(7, card.coefficientB, 0.0);
    writer.realUnlessDefault(8, card.specificHeat, STEINBERG_GUINAN_YIELD_NO_LIMIT);
    writer.realUnlessDefault(9, card.meltTemperature, STEINBERG_GUINAN_YIELD_NO_LIMIT);
    writer.realUnlessDefault(10, card.roomTemperature, STEINBERG_GUINAN_YIELD_ROOM_TEMPERATURE);
}

std::uint32_t SteinbergGuinanYieldTable::append(const SteinbergGuinanYieldCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void SteinbergGuinanYieldTable::store(std::uint32_t row, const SteinbergGuinanYieldCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void SteinbergGuinanYieldTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void SteinbergGuinanYieldTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
