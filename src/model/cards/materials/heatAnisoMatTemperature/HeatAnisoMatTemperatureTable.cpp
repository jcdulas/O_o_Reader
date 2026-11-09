#include "oor/model/cards/materials/heatAnisoMatTemperature/HeatAnisoMatTemperatureTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

std::optional<HeatAnisoMatTemperatureCard> HeatAnisoMatTemperatureFormat::read(FieldReader& reader)
{
    HeatAnisoMatTemperatureCard card;
    card.id = reader.requiredInteger(1, "id");
    card.conductivityXxTableId = reader.integer(2, "conductivityXxTableId", 0);
    card.conductivityXyTableId = reader.integer(3, "conductivityXyTableId", 0);
    card.conductivityXzTableId = reader.integer(4, "conductivityXzTableId", 0);
    card.conductivityYyTableId = reader.integer(5, "conductivityYyTableId", 0);
    card.conductivityYzTableId = reader.integer(6, "conductivityYzTableId", 0);
    card.conductivityZzTableId = reader.integer(7, "conductivityZzTableId", 0);
    card.specificHeatTableId = reader.integer(8, "specificHeatTableId", 0);
    card.heatGenerationFactorTableId = reader.integer(10, "heatGenerationFactorTableId", 0);
    requireBlankFields(reader, {9}, 10);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HeatAnisoMatTemperatureFormat::check(const HeatAnisoMatTemperatureCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (const auto problem = checkTableReference(card.conductivityXxTableId, "conductivityXxTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.conductivityXyTableId, "conductivityXyTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.conductivityXzTableId, "conductivityXzTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.conductivityYyTableId, "conductivityYyTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.conductivityYzTableId, "conductivityYzTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.conductivityZzTableId, "conductivityZzTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.specificHeatTableId, "specificHeatTableId"))
        return problem;
    return checkTableReference(card.heatGenerationFactorTableId, "heatGenerationFactorTableId");
}

void HeatAnisoMatTemperatureFormat::write(const HeatAnisoMatTemperatureCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.conductivityXxTableId, 0);
    writer.integerUnlessDefault(3, card.conductivityXyTableId, 0);
    writer.integerUnlessDefault(4, card.conductivityXzTableId, 0);
    writer.integerUnlessDefault(5, card.conductivityYyTableId, 0);
    writer.integerUnlessDefault(6, card.conductivityYzTableId, 0);
    writer.integerUnlessDefault(7, card.conductivityZzTableId, 0);
    writer.integerUnlessDefault(8, card.specificHeatTableId, 0);
    writer.integerUnlessDefault(10, card.heatGenerationFactorTableId, 0);
}

std::uint32_t HeatAnisoMatTemperatureTable::append(const HeatAnisoMatTemperatureCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_conductivityXxTableId.push(card.conductivityXxTableId);
    m_conductivityXyTableId.push(card.conductivityXyTableId);
    m_conductivityXzTableId.push(card.conductivityXzTableId);
    m_conductivityYyTableId.push(card.conductivityYyTableId);
    m_conductivityYzTableId.push(card.conductivityYzTableId);
    m_conductivityZzTableId.push(card.conductivityZzTableId);
    m_specificHeatTableId.push(card.specificHeatTableId);
    m_heatGenerationFactorTableId.push(card.heatGenerationFactorTableId);
    return row;
}

HeatAnisoMatTemperatureCard HeatAnisoMatTemperatureTable::card(std::uint32_t row) const
{
    HeatAnisoMatTemperatureCard card;
    card.id = m_id.get(row);
    card.conductivityXxTableId = m_conductivityXxTableId.get(row);
    card.conductivityXyTableId = m_conductivityXyTableId.get(row);
    card.conductivityXzTableId = m_conductivityXzTableId.get(row);
    card.conductivityYyTableId = m_conductivityYyTableId.get(row);
    card.conductivityYzTableId = m_conductivityYzTableId.get(row);
    card.conductivityZzTableId = m_conductivityZzTableId.get(row);
    card.specificHeatTableId = m_specificHeatTableId.get(row);
    card.heatGenerationFactorTableId = m_heatGenerationFactorTableId.get(row);
    return card;
}

void HeatAnisoMatTemperatureTable::store(std::uint32_t row, const HeatAnisoMatTemperatureCard& card)
{
    m_id.set(row, card.id);
    m_conductivityXxTableId.set(row, card.conductivityXxTableId);
    m_conductivityXyTableId.set(row, card.conductivityXyTableId);
    m_conductivityXzTableId.set(row, card.conductivityXzTableId);
    m_conductivityYyTableId.set(row, card.conductivityYyTableId);
    m_conductivityYzTableId.set(row, card.conductivityYzTableId);
    m_conductivityZzTableId.set(row, card.conductivityZzTableId);
    m_specificHeatTableId.set(row, card.specificHeatTableId);
    m_heatGenerationFactorTableId.set(row, card.heatGenerationFactorTableId);
}

void HeatAnisoMatTemperatureTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_conductivityXxTableId.keepRows(rows);
    m_conductivityXyTableId.keepRows(rows);
    m_conductivityXzTableId.keepRows(rows);
    m_conductivityYyTableId.keepRows(rows);
    m_conductivityYzTableId.keepRows(rows);
    m_conductivityZzTableId.keepRows(rows);
    m_specificHeatTableId.keepRows(rows);
    m_heatGenerationFactorTableId.keepRows(rows);
}

void HeatAnisoMatTemperatureTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MAT5 that the card extends; the tables are TABLEMi cards.
    m_id.set(row, visitor.visit("id", ReferenceTarget::ThermalMaterial, m_id.get(row)));
    if (const base::EntityId id = m_conductivityXxTableId.get(row); id != 0)
        m_conductivityXxTableId.set(row, visitor.visit("conductivityXxTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_conductivityXyTableId.get(row); id != 0)
        m_conductivityXyTableId.set(row, visitor.visit("conductivityXyTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_conductivityXzTableId.get(row); id != 0)
        m_conductivityXzTableId.set(row, visitor.visit("conductivityXzTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_conductivityYyTableId.get(row); id != 0)
        m_conductivityYyTableId.set(row, visitor.visit("conductivityYyTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_conductivityYzTableId.get(row); id != 0)
        m_conductivityYzTableId.set(row, visitor.visit("conductivityYzTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_conductivityZzTableId.get(row); id != 0)
        m_conductivityZzTableId.set(row, visitor.visit("conductivityZzTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_specificHeatTableId.get(row); id != 0)
        m_specificHeatTableId.set(row, visitor.visit("specificHeatTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_heatGenerationFactorTableId.get(row); id != 0)
        m_heatGenerationFactorTableId.set(
            row, visitor.visit("heatGenerationFactorTableId", ReferenceTarget::MaterialTable, id));
}

} // namespace oor::model
