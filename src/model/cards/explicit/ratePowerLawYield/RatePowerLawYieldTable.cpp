#include "oor/model/cards/explicit/ratePowerLawYield/RatePowerLawYieldTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<RatePowerLawYieldCard> RatePowerLawYieldFormat::read(FieldReader& reader)
{
    RatePowerLawYieldCard card;
    card.id = reader.requiredInteger(1, "id");
    card.coefficientA = reader.requiredReal(2, "coefficientA");
    card.coefficientB = reader.real(3, "coefficientB", 0.0);
    card.coefficientN = reader.real(4, "coefficientN", 1.0);
    card.coefficientM = reader.real(5, "coefficientM", 1.0);
    card.coefficientC = reader.real(6, "coefficientC", RATE_POWER_LAW_YIELD_DEFAULT_MINIMUM);
    if (reader.lastUsedField() > 6)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 6 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RatePowerLawYieldFormat::check(const RatePowerLawYieldCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.coefficientA, "coefficientA", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.coefficientB, "coefficientB", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientN, "coefficientN", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientM, "coefficientM", RealRule::Finite))
        return problem;
    return checkReal(card.coefficientC, "coefficientC", RealRule::Finite);
}

void RatePowerLawYieldFormat::write(const RatePowerLawYieldCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.coefficientA);
    writer.realUnlessDefault(3, card.coefficientB, 0.0);
    writer.realUnlessDefault(4, card.coefficientN, 1.0);
    writer.realUnlessDefault(5, card.coefficientM, 1.0);
    writer.realUnlessDefault(6, card.coefficientC, RATE_POWER_LAW_YIELD_DEFAULT_MINIMUM);
}

std::uint32_t RatePowerLawYieldTable::append(const RatePowerLawYieldCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void RatePowerLawYieldTable::store(std::uint32_t row, const RatePowerLawYieldCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void RatePowerLawYieldTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void RatePowerLawYieldTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
