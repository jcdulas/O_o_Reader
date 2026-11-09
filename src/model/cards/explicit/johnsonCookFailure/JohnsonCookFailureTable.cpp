#include "oor/model/cards/explicit/johnsonCookFailure/JohnsonCookFailureTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<JohnsonCookFailureCard> JohnsonCookFailureFormat::read(FieldReader& reader)
{
    JohnsonCookFailureCard card;
    card.id = reader.requiredInteger(1, "id");
    card.coefficientD1 = reader.real(2, "coefficientD1", 0.0);
    card.coefficientD2 = reader.real(3, "coefficientD2", 0.0);
    card.coefficientD3 = reader.real(4, "coefficientD3", 0.0);
    card.coefficientD4 = reader.real(5, "coefficientD4", 0.0);
    card.coefficientD5 = reader.real(6, "coefficientD5", 0.0);
    card.referenceStrainRate = reader.real(7, "referenceStrainRate", 1.0);
    card.roomTemperature = reader.real(8, "roomTemperature", 0.0);
    card.meltTemperature = reader.real(9, "meltTemperature", JOHNSON_COOK_FAILURE_NO_LIMIT);
    card.specificHeat = reader.real(10, "specificHeat", JOHNSON_COOK_FAILURE_NO_LIMIT);
    card.method =
        readKeyword(reader, 11, "method", JOHNSON_COOK_FAILURE_METHOD_WORDS, JohnsonCookFailureMethod::Continuous);
    if (reader.lastUsedField() > 11)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 11 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> JohnsonCookFailureFormat::check(const JohnsonCookFailureCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.coefficientD1, "coefficientD1", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientD2, "coefficientD2", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientD3, "coefficientD3", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientD4, "coefficientD4", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientD5, "coefficientD5", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.referenceStrainRate, "referenceStrainRate", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.roomTemperature, "roomTemperature", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.meltTemperature, "meltTemperature", RealRule::Finite))
        return problem;
    return checkReal(card.specificHeat, "specificHeat", RealRule::Finite);
}

void JohnsonCookFailureFormat::write(const JohnsonCookFailureCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.coefficientD1, 0.0);
    writer.realUnlessDefault(3, card.coefficientD2, 0.0);
    writer.realUnlessDefault(4, card.coefficientD3, 0.0);
    writer.realUnlessDefault(5, card.coefficientD4, 0.0);
    writer.realUnlessDefault(6, card.coefficientD5, 0.0);
    writer.realUnlessDefault(7, card.referenceStrainRate, 1.0);
    writer.realUnlessDefault(8, card.roomTemperature, 0.0);
    writer.realUnlessDefault(9, card.meltTemperature, JOHNSON_COOK_FAILURE_NO_LIMIT);
    writer.realUnlessDefault(10, card.specificHeat, JOHNSON_COOK_FAILURE_NO_LIMIT);
    writeKeywordUnlessDefault(writer, 11, JOHNSON_COOK_FAILURE_METHOD_WORDS, card.method,
                              JohnsonCookFailureMethod::Continuous);
}

std::uint32_t JohnsonCookFailureTable::append(const JohnsonCookFailureCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void JohnsonCookFailureTable::store(std::uint32_t row, const JohnsonCookFailureCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void JohnsonCookFailureTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void JohnsonCookFailureTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
