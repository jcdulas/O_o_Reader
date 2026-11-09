#include "oor/model/cards/explicit/zerilliArmstrongYield/ZerilliArmstrongYieldTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<ZerilliArmstrongYieldCard> ZerilliArmstrongYieldFormat::read(FieldReader& reader)
{
    ZerilliArmstrongYieldCard card;
    card.id = reader.requiredInteger(1, "id");
    card.coefficientA = reader.requiredReal(2, "coefficientA");
    card.coefficientB = reader.real(3, "coefficientB", 0.0);
    card.coefficientN = reader.real(4, "coefficientN", 0.0);
    card.coefficientC = reader.real(5, "coefficientC", 1.0);
    card.coefficientM = reader.real(6, "coefficientM", 1.0);
    card.referenceStrainRate = reader.real(7, "referenceStrainRate", 1.0);
    card.specificHeat = reader.real(8, "specificHeat", ZERILLI_ARMSTRONG_YIELD_NO_LIMIT);
    card.coefficientD = reader.optionalReal(9, "coefficientD");
    if (reader.lastUsedField() > 9)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 9 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ZerilliArmstrongYieldFormat::check(const ZerilliArmstrongYieldCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.coefficientA, "coefficientA", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.coefficientB, "coefficientB", RealRule::NonNegative))
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
    return checkOptionalReal(card.coefficientD, "coefficientD", RealRule::Finite);
}

void ZerilliArmstrongYieldFormat::write(const ZerilliArmstrongYieldCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.coefficientA);
    writer.realUnlessDefault(3, card.coefficientB, 0.0);
    writer.realUnlessDefault(4, card.coefficientN, 0.0);
    writer.realUnlessDefault(5, card.coefficientC, 1.0);
    writer.realUnlessDefault(6, card.coefficientM, 1.0);
    writer.realUnlessDefault(7, card.referenceStrainRate, 1.0);
    writer.realUnlessDefault(8, card.specificHeat, ZERILLI_ARMSTRONG_YIELD_NO_LIMIT);
    if (card.coefficientD)
        writer.real(9, *card.coefficientD);
}

std::uint32_t ZerilliArmstrongYieldTable::append(const ZerilliArmstrongYieldCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ZerilliArmstrongYieldTable::store(std::uint32_t row, const ZerilliArmstrongYieldCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ZerilliArmstrongYieldTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ZerilliArmstrongYieldTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
