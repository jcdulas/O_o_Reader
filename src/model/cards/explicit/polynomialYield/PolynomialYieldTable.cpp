#include "oor/model/cards/explicit/polynomialYield/PolynomialYieldTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<PolynomialYieldCard> PolynomialYieldFormat::read(FieldReader& reader)
{
    PolynomialYieldCard card;
    card.id = reader.requiredInteger(1, "id");
    card.coefficientA = reader.requiredReal(2, "coefficientA");
    card.coefficientB = reader.real(3, "coefficientB", 0.0);
    card.coefficientC = reader.real(4, "coefficientC", 0.0);
    card.coefficientD = reader.real(5, "coefficientD", 0.0);
    card.coefficientE = reader.real(6, "coefficientE", 0.0);
    card.coefficientF = reader.real(7, "coefficientF", 0.0);
    card.maxYieldStress = reader.real(8, "maxYieldStress", POLYNOMIAL_YIELD_NO_LIMIT);
    if (reader.lastUsedField() > 8)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 8 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PolynomialYieldFormat::check(const PolynomialYieldCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.coefficientA, "coefficientA", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.coefficientB, "coefficientB", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientC, "coefficientC", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientD, "coefficientD", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientE, "coefficientE", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientF, "coefficientF", RealRule::Finite))
        return problem;
    return checkReal(card.maxYieldStress, "maxYieldStress", RealRule::Finite);
}

void PolynomialYieldFormat::write(const PolynomialYieldCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.coefficientA);
    writer.realUnlessDefault(3, card.coefficientB, 0.0);
    writer.realUnlessDefault(4, card.coefficientC, 0.0);
    writer.realUnlessDefault(5, card.coefficientD, 0.0);
    writer.realUnlessDefault(6, card.coefficientE, 0.0);
    writer.realUnlessDefault(7, card.coefficientF, 0.0);
    writer.realUnlessDefault(8, card.maxYieldStress, POLYNOMIAL_YIELD_NO_LIMIT);
}

std::uint32_t PolynomialYieldTable::append(const PolynomialYieldCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void PolynomialYieldTable::store(std::uint32_t row, const PolynomialYieldCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void PolynomialYieldTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void PolynomialYieldTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
