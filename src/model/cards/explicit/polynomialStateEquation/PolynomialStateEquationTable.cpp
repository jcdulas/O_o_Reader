#include "oor/model/cards/explicit/polynomialStateEquation/PolynomialStateEquationTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <cmath>

namespace oor::model
{

std::optional<PolynomialStateEquationCard> PolynomialStateEquationFormat::read(FieldReader& reader)
{
    PolynomialStateEquationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.coefficientA1 = reader.real(2, "coefficientA1", 0.0);
    card.coefficientA2 = reader.real(3, "coefficientA2", 0.0);
    card.coefficientA3 = reader.real(4, "coefficientA3", 0.0);
    card.coefficientB0 = reader.real(5, "coefficientB0", 0.0);
    card.coefficientB1 = reader.real(6, "coefficientB1", 0.0);
    card.coefficientB2 = reader.real(7, "coefficientB2", 0.0);
    card.coefficientB3 = reader.real(8, "coefficientB3", 0.0);
    card.volumeLimit = reader.real(9, "volumeLimit", POLYNOMIAL_STATE_EQUATION_VOLUME_LIMIT_DEFAULT);
    card.viscosity = reader.real(10, "viscosity", 0.0);
    if (reader.lastUsedField() > 10)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 10 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PolynomialStateEquationFormat::check(const PolynomialStateEquationCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.coefficientA1, "coefficientA1", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientA2, "coefficientA2", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientA3, "coefficientA3", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientB0, "coefficientB0", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientB1, "coefficientB1", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientB2, "coefficientB2", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientB3, "coefficientB3", RealRule::Finite))
        return problem;
    if (!std::isfinite(card.volumeLimit) || !(card.volumeLimit > 1.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 1",
                           "volumeLimit"};
    return checkReal(card.viscosity, "viscosity", RealRule::Finite);
}

void PolynomialStateEquationFormat::write(const PolynomialStateEquationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.coefficientA1, 0.0);
    writer.realUnlessDefault(3, card.coefficientA2, 0.0);
    writer.realUnlessDefault(4, card.coefficientA3, 0.0);
    writer.realUnlessDefault(5, card.coefficientB0, 0.0);
    writer.realUnlessDefault(6, card.coefficientB1, 0.0);
    writer.realUnlessDefault(7, card.coefficientB2, 0.0);
    writer.realUnlessDefault(8, card.coefficientB3, 0.0);
    writer.realUnlessDefault(9, card.volumeLimit, POLYNOMIAL_STATE_EQUATION_VOLUME_LIMIT_DEFAULT);
    writer.realUnlessDefault(10, card.viscosity, 0.0);
}

std::uint32_t PolynomialStateEquationTable::append(const PolynomialStateEquationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void PolynomialStateEquationTable::store(std::uint32_t row, const PolynomialStateEquationCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void PolynomialStateEquationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void PolynomialStateEquationTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
