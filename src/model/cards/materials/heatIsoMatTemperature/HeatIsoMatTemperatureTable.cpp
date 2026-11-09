#include "oor/model/cards/materials/heatIsoMatTemperature/HeatIsoMatTemperatureTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

std::optional<HeatIsoMatTemperatureCard> HeatIsoMatTemperatureFormat::read(FieldReader& reader)
{
    HeatIsoMatTemperatureCard card;
    card.id = reader.requiredInteger(1, "id");
    card.conductivityTableId = reader.integer(2, "conductivityTableId", 0);
    card.specificHeatTableId = reader.integer(3, "specificHeatTableId", 0);
    card.convectionCoefficientTableId = reader.integer(5, "convectionCoefficientTableId", 0);
    card.viscosityTableId = reader.integer(6, "viscosityTableId", 0);
    card.heatGenerationFactorTableId = reader.integer(7, "heatGenerationFactorTableId", 0);
    requireBlankFields(reader, {4}, 7);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HeatIsoMatTemperatureFormat::check(const HeatIsoMatTemperatureCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (const auto problem = checkTableReference(card.conductivityTableId, "conductivityTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.specificHeatTableId, "specificHeatTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.convectionCoefficientTableId, "convectionCoefficientTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.viscosityTableId, "viscosityTableId"))
        return problem;
    return checkTableReference(card.heatGenerationFactorTableId, "heatGenerationFactorTableId");
}

void HeatIsoMatTemperatureFormat::write(const HeatIsoMatTemperatureCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.conductivityTableId, 0);
    writer.integerUnlessDefault(3, card.specificHeatTableId, 0);
    writer.integerUnlessDefault(5, card.convectionCoefficientTableId, 0);
    writer.integerUnlessDefault(6, card.viscosityTableId, 0);
    writer.integerUnlessDefault(7, card.heatGenerationFactorTableId, 0);
}

std::uint32_t HeatIsoMatTemperatureTable::append(const HeatIsoMatTemperatureCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_conductivityTableId.push(card.conductivityTableId);
    m_specificHeatTableId.push(card.specificHeatTableId);
    m_convectionCoefficientTableId.push(card.convectionCoefficientTableId);
    m_viscosityTableId.push(card.viscosityTableId);
    m_heatGenerationFactorTableId.push(card.heatGenerationFactorTableId);
    return row;
}

HeatIsoMatTemperatureCard HeatIsoMatTemperatureTable::card(std::uint32_t row) const
{
    HeatIsoMatTemperatureCard card;
    card.id = m_id.get(row);
    card.conductivityTableId = m_conductivityTableId.get(row);
    card.specificHeatTableId = m_specificHeatTableId.get(row);
    card.convectionCoefficientTableId = m_convectionCoefficientTableId.get(row);
    card.viscosityTableId = m_viscosityTableId.get(row);
    card.heatGenerationFactorTableId = m_heatGenerationFactorTableId.get(row);
    return card;
}

void HeatIsoMatTemperatureTable::store(std::uint32_t row, const HeatIsoMatTemperatureCard& card)
{
    m_id.set(row, card.id);
    m_conductivityTableId.set(row, card.conductivityTableId);
    m_specificHeatTableId.set(row, card.specificHeatTableId);
    m_convectionCoefficientTableId.set(row, card.convectionCoefficientTableId);
    m_viscosityTableId.set(row, card.viscosityTableId);
    m_heatGenerationFactorTableId.set(row, card.heatGenerationFactorTableId);
}

void HeatIsoMatTemperatureTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_conductivityTableId.keepRows(rows);
    m_specificHeatTableId.keepRows(rows);
    m_convectionCoefficientTableId.keepRows(rows);
    m_viscosityTableId.keepRows(rows);
    m_heatGenerationFactorTableId.keepRows(rows);
}

void HeatIsoMatTemperatureTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MAT4 that the card extends; the tables are TABLEMi cards.
    m_id.set(row, visitor.visit("id", ReferenceTarget::ThermalMaterial, m_id.get(row)));
    if (const base::EntityId id = m_conductivityTableId.get(row); id != 0)
        m_conductivityTableId.set(row, visitor.visit("conductivityTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_specificHeatTableId.get(row); id != 0)
        m_specificHeatTableId.set(row, visitor.visit("specificHeatTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_convectionCoefficientTableId.get(row); id != 0)
        m_convectionCoefficientTableId.set(
            row, visitor.visit("convectionCoefficientTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_viscosityTableId.get(row); id != 0)
        m_viscosityTableId.set(row, visitor.visit("viscosityTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_heatGenerationFactorTableId.get(row); id != 0)
        m_heatGenerationFactorTableId.set(
            row, visitor.visit("heatGenerationFactorTableId", ReferenceTarget::MaterialTable, id));
}

} // namespace oor::model
