#include "oor/model/cards/thermal/heatAnisoMat/HeatAnisoMatTable.hpp"

#include <string>

namespace oor::model
{

HeatAnisoMatTable::HeatAnisoMatTable() : m_density(1.0), m_heatGenerationFactor(1.0) {}

std::optional<HeatAnisoMatCard> HeatAnisoMatFormat::read(FieldReader& reader)
{
    HeatAnisoMatCard card;
    card.id = reader.requiredInteger(1, "id");
    card.conductivityXx = reader.real(2, "conductivityXx", 0.0);
    card.conductivityXy = reader.real(3, "conductivityXy", 0.0);
    card.conductivityXz = reader.real(4, "conductivityXz", 0.0);
    card.conductivityYy = reader.real(5, "conductivityYy", 0.0);
    card.conductivityYz = reader.real(6, "conductivityYz", 0.0);
    card.conductivityZz = reader.real(7, "conductivityZz", 0.0);
    card.specificHeat = reader.real(8, "specificHeat", 0.0);
    card.density = reader.real(9, "density", 1.0);
    card.heatGenerationFactor = reader.real(10, "heatGenerationFactor", 1.0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HeatAnisoMatFormat::check(const HeatAnisoMatCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "id"};
    if (!(card.specificHeat >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be negative", "specificHeat"};
    if (!(card.density >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be negative", "density"};
    if (!(card.heatGenerationFactor >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be negative",
                           "heatGenerationFactor"};
    return std::nullopt;
}

void HeatAnisoMatFormat::write(const HeatAnisoMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.conductivityXx, 0.0);
    writer.realUnlessDefault(3, card.conductivityXy, 0.0);
    writer.realUnlessDefault(4, card.conductivityXz, 0.0);
    writer.realUnlessDefault(5, card.conductivityYy, 0.0);
    writer.realUnlessDefault(6, card.conductivityYz, 0.0);
    writer.realUnlessDefault(7, card.conductivityZz, 0.0);
    writer.realUnlessDefault(8, card.specificHeat, 0.0);
    writer.realUnlessDefault(9, card.density, 1.0);
    writer.realUnlessDefault(10, card.heatGenerationFactor, 1.0);
}

std::uint32_t HeatAnisoMatTable::append(const HeatAnisoMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_conductivityXx.push(card.conductivityXx);
    m_conductivityXy.push(card.conductivityXy);
    m_conductivityXz.push(card.conductivityXz);
    m_conductivityYy.push(card.conductivityYy);
    m_conductivityYz.push(card.conductivityYz);
    m_conductivityZz.push(card.conductivityZz);
    m_specificHeat.push(card.specificHeat);
    m_density.push(card.density);
    m_heatGenerationFactor.push(card.heatGenerationFactor);
    return row;
}

HeatAnisoMatCard HeatAnisoMatTable::card(std::uint32_t row) const
{
    HeatAnisoMatCard card;
    card.id = m_id.get(row);
    card.conductivityXx = m_conductivityXx.get(row);
    card.conductivityXy = m_conductivityXy.get(row);
    card.conductivityXz = m_conductivityXz.get(row);
    card.conductivityYy = m_conductivityYy.get(row);
    card.conductivityYz = m_conductivityYz.get(row);
    card.conductivityZz = m_conductivityZz.get(row);
    card.specificHeat = m_specificHeat.get(row);
    card.density = m_density.get(row);
    card.heatGenerationFactor = m_heatGenerationFactor.get(row);
    return card;
}

void HeatAnisoMatTable::store(std::uint32_t row, const HeatAnisoMatCard& card)
{
    m_id.set(row, card.id);
    m_conductivityXx.set(row, card.conductivityXx);
    m_conductivityXy.set(row, card.conductivityXy);
    m_conductivityXz.set(row, card.conductivityXz);
    m_conductivityYy.set(row, card.conductivityYy);
    m_conductivityYz.set(row, card.conductivityYz);
    m_conductivityZz.set(row, card.conductivityZz);
    m_specificHeat.set(row, card.specificHeat);
    m_density.set(row, card.density);
    m_heatGenerationFactor.set(row, card.heatGenerationFactor);
}

void HeatAnisoMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_conductivityXx.keepRows(rows);
    m_conductivityXy.keepRows(rows);
    m_conductivityXz.keepRows(rows);
    m_conductivityYy.keepRows(rows);
    m_conductivityYz.keepRows(rows);
    m_conductivityZz.keepRows(rows);
    m_specificHeat.keepRows(rows);
    m_density.keepRows(rows);
    m_heatGenerationFactor.keepRows(rows);
}

} // namespace oor::model
