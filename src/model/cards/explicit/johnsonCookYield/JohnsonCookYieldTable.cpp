#include "oor/model/cards/explicit/johnsonCookYield/JohnsonCookYieldTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<JohnsonCookYieldCard> JohnsonCookYieldFormat::read(FieldReader& reader)
{
    JohnsonCookYieldCard card;
    card.id = reader.requiredInteger(1, "id");
    card.coefficientA = reader.requiredReal(2, "coefficientA");
    card.coefficientB = reader.real(3, "coefficientB", 0.0);
    card.coefficientN = reader.real(4, "coefficientN", 1.0);
    card.coefficientC = reader.real(5, "coefficientC", 0.0);
    card.coefficientM = reader.real(6, "coefficientM", 1.0);
    card.referenceStrainRate = reader.real(7, "referenceStrainRate", 1.0);
    card.specificHeat = reader.real(8, "specificHeat", JOHNSON_COOK_YIELD_NO_LIMIT);
    card.meltTemperature = reader.real(9, "meltTemperature", JOHNSON_COOK_YIELD_NO_LIMIT);
    card.roomTemperature = reader.real(10, "roomTemperature", JOHNSON_COOK_YIELD_ROOM_TEMPERATURE);
    if (reader.lastUsedField() > 10)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 10 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> JohnsonCookYieldFormat::check(const JohnsonCookYieldCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.coefficientA, "coefficientA", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.coefficientB, "coefficientB", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientN, "coefficientN", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientC, "coefficientC", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientM, "coefficientM", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.referenceStrainRate, "referenceStrainRate", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.specificHeat, "specificHeat", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.meltTemperature, "meltTemperature", RealRule::Finite))
        return problem;
    return checkReal(card.roomTemperature, "roomTemperature", RealRule::Finite);
}

void JohnsonCookYieldFormat::write(const JohnsonCookYieldCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.coefficientA);
    writer.realUnlessDefault(3, card.coefficientB, 0.0);
    writer.realUnlessDefault(4, card.coefficientN, 1.0);
    writer.realUnlessDefault(5, card.coefficientC, 0.0);
    writer.realUnlessDefault(6, card.coefficientM, 1.0);
    writer.realUnlessDefault(7, card.referenceStrainRate, 1.0);
    writer.realUnlessDefault(8, card.specificHeat, JOHNSON_COOK_YIELD_NO_LIMIT);
    writer.realUnlessDefault(9, card.meltTemperature, JOHNSON_COOK_YIELD_NO_LIMIT);
    writer.realUnlessDefault(10, card.roomTemperature, JOHNSON_COOK_YIELD_ROOM_TEMPERATURE);
}

std::uint32_t JohnsonCookYieldTable::append(const JohnsonCookYieldCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void JohnsonCookYieldTable::store(std::uint32_t row, const JohnsonCookYieldCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void JohnsonCookYieldTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void JohnsonCookYieldTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
