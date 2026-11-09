#include "oor/model/cards/explicit/deflagrationStateEquation/DeflagrationStateEquationTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<DeflagrationStateEquationCard> DeflagrationStateEquationFormat::read(FieldReader& reader)
{
    DeflagrationStateEquationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.gammaCoefficient = reader.requiredReal(2, "gammaCoefficient");
    card.coVolume = reader.requiredReal(3, "coVolume");
    card.gasConstant = reader.optionalReal(4, "gasConstant");
    card.specificHeatVolume = reader.optionalReal(5, "specificHeatVolume");
    card.specificHeatPressure = reader.optionalReal(6, "specificHeatPressure");
    card.chemicalEnergy = reader.requiredReal(7, "chemicalEnergy");
    card.referenceDensity = reader.requiredReal(8, "referenceDensity");
    card.powderDensityRatio = reader.optionalReal(9, "powderDensityRatio");
    card.burnRateCoefficient = reader.requiredReal(10, "burnRateCoefficient");
    card.burnRateExponent = reader.requiredReal(11, "burnRateExponent");
    card.surfaceToVolumeRatio = reader.requiredReal(12, "surfaceToVolumeRatio");
    card.formParameterX = reader.requiredReal(13, "formParameterX");
    card.formParameterY = reader.requiredReal(14, "formParameterY");
    if (reader.lastUsedField() > 14)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 14 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DeflagrationStateEquationFormat::check(const DeflagrationStateEquationCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.gammaCoefficient, "gammaCoefficient", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.coVolume, "coVolume", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkOptionalReal(card.gasConstant, "gasConstant", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.specificHeatVolume, "specificHeatVolume", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.specificHeatPressure, "specificHeatPressure", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.chemicalEnergy, "chemicalEnergy", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.referenceDensity, "referenceDensity", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.powderDensityRatio, "powderDensityRatio", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.burnRateCoefficient, "burnRateCoefficient", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.burnRateExponent, "burnRateExponent", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.surfaceToVolumeRatio, "surfaceToVolumeRatio", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.formParameterX, "formParameterX", RealRule::Positive))
        return problem;
    return checkReal(card.formParameterY, "formParameterY", RealRule::NonNegative);
}

void DeflagrationStateEquationFormat::write(const DeflagrationStateEquationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.gammaCoefficient);
    writer.real(3, card.coVolume);
    if (card.gasConstant)
        writer.real(4, *card.gasConstant);
    if (card.specificHeatVolume)
        writer.real(5, *card.specificHeatVolume);
    if (card.specificHeatPressure)
        writer.real(6, *card.specificHeatPressure);
    writer.real(7, card.chemicalEnergy);
    writer.real(8, card.referenceDensity);
    if (card.powderDensityRatio)
        writer.real(9, *card.powderDensityRatio);
    writer.real(10, card.burnRateCoefficient);
    writer.real(11, card.burnRateExponent);
    writer.real(12, card.surfaceToVolumeRatio);
    writer.real(13, card.formParameterX);
    writer.real(14, card.formParameterY);
}

std::uint32_t DeflagrationStateEquationTable::append(const DeflagrationStateEquationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void DeflagrationStateEquationTable::store(std::uint32_t row, const DeflagrationStateEquationCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void DeflagrationStateEquationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void DeflagrationStateEquationTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
