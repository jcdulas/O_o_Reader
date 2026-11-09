#include "oor/model/cards/explicit/gammaLawStateEquation/GammaLawStateEquationTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<GammaLawStateEquationCard> GammaLawStateEquationFormat::read(FieldReader& reader)
{
    GammaLawStateEquationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.gammaCoefficient = reader.optionalReal(2, "gammaCoefficient");
    card.gasConstant = reader.optionalReal(3, "gasConstant");
    card.specificHeatVolume = reader.optionalReal(4, "specificHeatVolume");
    card.specificHeatPressure = reader.optionalReal(5, "specificHeatPressure");
    card.viscosity = reader.optionalReal(6, "viscosity");
    if (reader.lastUsedField() > 6)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 6 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> GammaLawStateEquationFormat::check(const GammaLawStateEquationCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalReal(card.gammaCoefficient, "gammaCoefficient", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.gasConstant, "gasConstant", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.specificHeatVolume, "specificHeatVolume", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.specificHeatPressure, "specificHeatPressure", RealRule::Positive))
        return problem;
    return checkOptionalReal(card.viscosity, "viscosity", RealRule::NonNegative);
}

void GammaLawStateEquationFormat::write(const GammaLawStateEquationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.gammaCoefficient)
        writer.real(2, *card.gammaCoefficient);
    if (card.gasConstant)
        writer.real(3, *card.gasConstant);
    if (card.specificHeatVolume)
        writer.real(4, *card.specificHeatVolume);
    if (card.specificHeatPressure)
        writer.real(5, *card.specificHeatPressure);
    if (card.viscosity)
        writer.real(6, *card.viscosity);
}

std::uint32_t GammaLawStateEquationTable::append(const GammaLawStateEquationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void GammaLawStateEquationTable::store(std::uint32_t row, const GammaLawStateEquationCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void GammaLawStateEquationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void GammaLawStateEquationTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
