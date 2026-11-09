#include "oor/model/cards/explicit/jonesWilkinsLeeStateEquation/JonesWilkinsLeeStateEquationTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<JonesWilkinsLeeStateEquationCard> JonesWilkinsLeeStateEquationFormat::read(FieldReader& reader)
{
    JonesWilkinsLeeStateEquationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.coefficientA = reader.real(2, "coefficientA", 0.0);
    card.coefficientB = reader.real(3, "coefficientB", 0.0);
    card.coefficientR1 = reader.real(4, "coefficientR1", 0.0);
    card.coefficientR2 = reader.real(5, "coefficientR2", 0.0);
    card.omega = reader.real(6, "omega", 0.0);
    card.initialPressure = reader.real(7, "initialPressure", 0.0);
    card.afterburnEnergy = reader.real(8, "afterburnEnergy", 0.0);
    card.afterburnRate = reader.real(9, "afterburnRate", 0.0);
    card.coefficientM = reader.real(10, "coefficientM", 0.0);
    card.coefficientN = reader.real(11, "coefficientN", 0.0);
    if (reader.lastUsedField() > 11)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 11 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> JonesWilkinsLeeStateEquationFormat::check(const JonesWilkinsLeeStateEquationCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.coefficientA, "coefficientA", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientB, "coefficientB", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientR1, "coefficientR1", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientR2, "coefficientR2", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.omega, "omega", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.initialPressure, "initialPressure", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.afterburnEnergy, "afterburnEnergy", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.afterburnRate, "afterburnRate", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coefficientM, "coefficientM", RealRule::Finite))
        return problem;
    return checkReal(card.coefficientN, "coefficientN", RealRule::Finite);
}

void JonesWilkinsLeeStateEquationFormat::write(const JonesWilkinsLeeStateEquationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.coefficientA, 0.0);
    writer.realUnlessDefault(3, card.coefficientB, 0.0);
    writer.realUnlessDefault(4, card.coefficientR1, 0.0);
    writer.realUnlessDefault(5, card.coefficientR2, 0.0);
    writer.realUnlessDefault(6, card.omega, 0.0);
    writer.realUnlessDefault(7, card.initialPressure, 0.0);
    writer.realUnlessDefault(8, card.afterburnEnergy, 0.0);
    writer.realUnlessDefault(9, card.afterburnRate, 0.0);
    writer.realUnlessDefault(10, card.coefficientM, 0.0);
    writer.realUnlessDefault(11, card.coefficientN, 0.0);
}

std::uint32_t JonesWilkinsLeeStateEquationTable::append(const JonesWilkinsLeeStateEquationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void JonesWilkinsLeeStateEquationTable::store(std::uint32_t row, const JonesWilkinsLeeStateEquationCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void JonesWilkinsLeeStateEquationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void JonesWilkinsLeeStateEquationTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
