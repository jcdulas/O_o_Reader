#include "oor/model/cards/thermal/heatIsoMat/HeatIsoMatTable.hpp"

#include <cmath>
#include <limits>

namespace oor::model
{

namespace
{

double notGiven()
{
    return std::numeric_limits<double>::quiet_NaN();
}

std::optional<double> given(double value)
{
    if (std::isnan(value))
        return std::nullopt;
    return value;
}

} // namespace

HeatIsoMatTable::HeatIsoMatTable()
    : m_conductivity(notGiven()),
      m_density(1.0),
      m_convectionCoefficient(notGiven()),
      m_viscosity(notGiven()),
      m_heatGenerationFactor(1.0),
      m_referenceEnthalpy(notGiven()),
      m_phaseChangeTemperature(notGiven()),
      m_phaseChangeRange(notGiven()),
      m_latentHeat(notGiven())
{
}

std::optional<HeatIsoMatCard> HeatIsoMatFormat::read(FieldReader& reader)
{
    HeatIsoMatCard card;
    card.id = reader.requiredInteger(1, "id");
    card.conductivity = reader.optionalReal(2, "conductivity");
    card.specificHeat = reader.real(3, "specificHeat", 0.0);
    card.density = reader.real(4, "density", 1.0);
    card.convectionCoefficient = reader.optionalReal(5, "convectionCoefficient");
    card.viscosity = reader.optionalReal(6, "viscosity");
    card.heatGenerationFactor = reader.real(7, "heatGenerationFactor", 1.0);
    card.referenceEnthalpy = reader.optionalReal(8, "referenceEnthalpy");
    card.phaseChangeTemperature = reader.optionalReal(9, "phaseChangeTemperature");
    card.phaseChangeRange = reader.optionalReal(10, "phaseChangeRange");
    card.latentHeat = reader.optionalReal(11, "latentHeat");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HeatIsoMatFormat::check(const HeatIsoMatCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "id"};
    if (card.conductivity && !(*card.conductivity > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0 (or blank)",
                           "conductivity"};
    if (!(card.specificHeat >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be negative", "specificHeat"};
    if (!(card.density >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be negative", "density"};
    if (card.viscosity && !(*card.viscosity > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0 (or blank)",
                           "viscosity"};
    if (!(card.heatGenerationFactor >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be negative",
                           "heatGenerationFactor"};
    if (card.phaseChangeRange && !(*card.phaseChangeRange >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be negative",
                           "phaseChangeRange"};
    if (card.latentHeat && !(*card.latentHeat > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0 (or blank)",
                           "latentHeat"};
    return std::nullopt;
}

void HeatIsoMatFormat::write(const HeatIsoMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.conductivity)
        writer.real(2, *card.conductivity);
    writer.realUnlessDefault(3, card.specificHeat, 0.0);
    writer.realUnlessDefault(4, card.density, 1.0);
    if (card.convectionCoefficient)
        writer.real(5, *card.convectionCoefficient);
    if (card.viscosity)
        writer.real(6, *card.viscosity);
    writer.realUnlessDefault(7, card.heatGenerationFactor, 1.0);
    if (card.referenceEnthalpy)
        writer.real(8, *card.referenceEnthalpy);
    // The phase change group is on the continuation line.
    if (card.phaseChangeTemperature)
        writer.real(9, *card.phaseChangeTemperature);
    if (card.phaseChangeRange)
        writer.real(10, *card.phaseChangeRange);
    if (card.latentHeat)
        writer.real(11, *card.latentHeat);
}

std::uint32_t HeatIsoMatTable::append(const HeatIsoMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_conductivity.push(card.conductivity.value_or(notGiven()));
    m_specificHeat.push(card.specificHeat);
    m_density.push(card.density);
    m_convectionCoefficient.push(card.convectionCoefficient.value_or(notGiven()));
    m_viscosity.push(card.viscosity.value_or(notGiven()));
    m_heatGenerationFactor.push(card.heatGenerationFactor);
    m_referenceEnthalpy.push(card.referenceEnthalpy.value_or(notGiven()));
    m_phaseChangeTemperature.push(card.phaseChangeTemperature.value_or(notGiven()));
    m_phaseChangeRange.push(card.phaseChangeRange.value_or(notGiven()));
    m_latentHeat.push(card.latentHeat.value_or(notGiven()));
    return row;
}

HeatIsoMatCard HeatIsoMatTable::card(std::uint32_t row) const
{
    HeatIsoMatCard card;
    card.id = m_id.get(row);
    card.conductivity = given(m_conductivity.get(row));
    card.specificHeat = m_specificHeat.get(row);
    card.density = m_density.get(row);
    card.convectionCoefficient = given(m_convectionCoefficient.get(row));
    card.viscosity = given(m_viscosity.get(row));
    card.heatGenerationFactor = m_heatGenerationFactor.get(row);
    card.referenceEnthalpy = given(m_referenceEnthalpy.get(row));
    card.phaseChangeTemperature = given(m_phaseChangeTemperature.get(row));
    card.phaseChangeRange = given(m_phaseChangeRange.get(row));
    card.latentHeat = given(m_latentHeat.get(row));
    return card;
}

void HeatIsoMatTable::store(std::uint32_t row, const HeatIsoMatCard& card)
{
    m_id.set(row, card.id);
    m_conductivity.set(row, card.conductivity.value_or(notGiven()));
    m_specificHeat.set(row, card.specificHeat);
    m_density.set(row, card.density);
    m_convectionCoefficient.set(row, card.convectionCoefficient.value_or(notGiven()));
    m_viscosity.set(row, card.viscosity.value_or(notGiven()));
    m_heatGenerationFactor.set(row, card.heatGenerationFactor);
    m_referenceEnthalpy.set(row, card.referenceEnthalpy.value_or(notGiven()));
    m_phaseChangeTemperature.set(row, card.phaseChangeTemperature.value_or(notGiven()));
    m_phaseChangeRange.set(row, card.phaseChangeRange.value_or(notGiven()));
    m_latentHeat.set(row, card.latentHeat.value_or(notGiven()));
}

void HeatIsoMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_conductivity.keepRows(rows);
    m_specificHeat.keepRows(rows);
    m_density.keepRows(rows);
    m_convectionCoefficient.keepRows(rows);
    m_viscosity.keepRows(rows);
    m_heatGenerationFactor.keepRows(rows);
    m_referenceEnthalpy.keepRows(rows);
    m_phaseChangeTemperature.keepRows(rows);
    m_phaseChangeRange.keepRows(rows);
    m_latentHeat.keepRows(rows);
}

} // namespace oor::model
