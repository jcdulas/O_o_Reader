#include "oor/model/cards/materials/axisymOrthoMatTemperature/AxisymOrthoMatTemperatureTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

std::optional<AxisymOrthoMatTemperatureCard> AxisymOrthoMatTemperatureFormat::read(FieldReader& reader)
{
    AxisymOrthoMatTemperatureCard card;
    card.id = reader.requiredInteger(1, "id");
    card.youngsModulusXTableId = reader.integer(2, "youngsModulusXTableId", 0);
    card.youngsModulusThetaTableId = reader.integer(3, "youngsModulusThetaTableId", 0);
    card.youngsModulusZTableId = reader.integer(4, "youngsModulusZTableId", 0);
    card.poissonRatioXthTableId = reader.integer(5, "poissonRatioXthTableId", 0);
    card.poissonRatioThzTableId = reader.integer(6, "poissonRatioThzTableId", 0);
    card.poissonRatioZxTableId = reader.integer(7, "poissonRatioZxTableId", 0);
    card.densityTableId = reader.integer(8, "densityTableId", 0);
    card.shearModulusZxTableId = reader.integer(11, "shearModulusZxTableId", 0);
    card.thermalExpansionXTableId = reader.integer(12, "thermalExpansionXTableId", 0);
    card.thermalExpansionThetaTableId = reader.integer(13, "thermalExpansionThetaTableId", 0);
    card.thermalExpansionZTableId = reader.integer(14, "thermalExpansionZTableId", 0);
    card.structuralDampingTableId = reader.integer(16, "structuralDampingTableId", 0);
    requireBlankFields(reader, {9, 10, 15}, 16);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymOrthoMatTemperatureFormat::check(const AxisymOrthoMatTemperatureCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (const auto problem = checkTableReference(card.youngsModulusXTableId, "youngsModulusXTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.youngsModulusThetaTableId, "youngsModulusThetaTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.youngsModulusZTableId, "youngsModulusZTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.poissonRatioXthTableId, "poissonRatioXthTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.poissonRatioThzTableId, "poissonRatioThzTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.poissonRatioZxTableId, "poissonRatioZxTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.densityTableId, "densityTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.shearModulusZxTableId, "shearModulusZxTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.thermalExpansionXTableId, "thermalExpansionXTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.thermalExpansionThetaTableId, "thermalExpansionThetaTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.thermalExpansionZTableId, "thermalExpansionZTableId"))
        return problem;
    return checkTableReference(card.structuralDampingTableId, "structuralDampingTableId");
}

void AxisymOrthoMatTemperatureFormat::write(const AxisymOrthoMatTemperatureCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.youngsModulusXTableId, 0);
    writer.integerUnlessDefault(3, card.youngsModulusThetaTableId, 0);
    writer.integerUnlessDefault(4, card.youngsModulusZTableId, 0);
    writer.integerUnlessDefault(5, card.poissonRatioXthTableId, 0);
    writer.integerUnlessDefault(6, card.poissonRatioThzTableId, 0);
    writer.integerUnlessDefault(7, card.poissonRatioZxTableId, 0);
    writer.integerUnlessDefault(8, card.densityTableId, 0);
    writer.integerUnlessDefault(11, card.shearModulusZxTableId, 0);
    writer.integerUnlessDefault(12, card.thermalExpansionXTableId, 0);
    writer.integerUnlessDefault(13, card.thermalExpansionThetaTableId, 0);
    writer.integerUnlessDefault(14, card.thermalExpansionZTableId, 0);
    writer.integerUnlessDefault(16, card.structuralDampingTableId, 0);
}

std::uint32_t AxisymOrthoMatTemperatureTable::append(const AxisymOrthoMatTemperatureCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_youngsModulusXTableId.push(card.youngsModulusXTableId);
    m_youngsModulusThetaTableId.push(card.youngsModulusThetaTableId);
    m_youngsModulusZTableId.push(card.youngsModulusZTableId);
    m_poissonRatioXthTableId.push(card.poissonRatioXthTableId);
    m_poissonRatioThzTableId.push(card.poissonRatioThzTableId);
    m_poissonRatioZxTableId.push(card.poissonRatioZxTableId);
    m_densityTableId.push(card.densityTableId);
    m_shearModulusZxTableId.push(card.shearModulusZxTableId);
    m_thermalExpansionXTableId.push(card.thermalExpansionXTableId);
    m_thermalExpansionThetaTableId.push(card.thermalExpansionThetaTableId);
    m_thermalExpansionZTableId.push(card.thermalExpansionZTableId);
    m_structuralDampingTableId.push(card.structuralDampingTableId);
    return row;
}

AxisymOrthoMatTemperatureCard AxisymOrthoMatTemperatureTable::card(std::uint32_t row) const
{
    AxisymOrthoMatTemperatureCard card;
    card.id = m_id.get(row);
    card.youngsModulusXTableId = m_youngsModulusXTableId.get(row);
    card.youngsModulusThetaTableId = m_youngsModulusThetaTableId.get(row);
    card.youngsModulusZTableId = m_youngsModulusZTableId.get(row);
    card.poissonRatioXthTableId = m_poissonRatioXthTableId.get(row);
    card.poissonRatioThzTableId = m_poissonRatioThzTableId.get(row);
    card.poissonRatioZxTableId = m_poissonRatioZxTableId.get(row);
    card.densityTableId = m_densityTableId.get(row);
    card.shearModulusZxTableId = m_shearModulusZxTableId.get(row);
    card.thermalExpansionXTableId = m_thermalExpansionXTableId.get(row);
    card.thermalExpansionThetaTableId = m_thermalExpansionThetaTableId.get(row);
    card.thermalExpansionZTableId = m_thermalExpansionZTableId.get(row);
    card.structuralDampingTableId = m_structuralDampingTableId.get(row);
    return card;
}

void AxisymOrthoMatTemperatureTable::store(std::uint32_t row, const AxisymOrthoMatTemperatureCard& card)
{
    m_id.set(row, card.id);
    m_youngsModulusXTableId.set(row, card.youngsModulusXTableId);
    m_youngsModulusThetaTableId.set(row, card.youngsModulusThetaTableId);
    m_youngsModulusZTableId.set(row, card.youngsModulusZTableId);
    m_poissonRatioXthTableId.set(row, card.poissonRatioXthTableId);
    m_poissonRatioThzTableId.set(row, card.poissonRatioThzTableId);
    m_poissonRatioZxTableId.set(row, card.poissonRatioZxTableId);
    m_densityTableId.set(row, card.densityTableId);
    m_shearModulusZxTableId.set(row, card.shearModulusZxTableId);
    m_thermalExpansionXTableId.set(row, card.thermalExpansionXTableId);
    m_thermalExpansionThetaTableId.set(row, card.thermalExpansionThetaTableId);
    m_thermalExpansionZTableId.set(row, card.thermalExpansionZTableId);
    m_structuralDampingTableId.set(row, card.structuralDampingTableId);
}

void AxisymOrthoMatTemperatureTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_youngsModulusXTableId.keepRows(rows);
    m_youngsModulusThetaTableId.keepRows(rows);
    m_youngsModulusZTableId.keepRows(rows);
    m_poissonRatioXthTableId.keepRows(rows);
    m_poissonRatioThzTableId.keepRows(rows);
    m_poissonRatioZxTableId.keepRows(rows);
    m_densityTableId.keepRows(rows);
    m_shearModulusZxTableId.keepRows(rows);
    m_thermalExpansionXTableId.keepRows(rows);
    m_thermalExpansionThetaTableId.keepRows(rows);
    m_thermalExpansionZTableId.keepRows(rows);
    m_structuralDampingTableId.keepRows(rows);
}

void AxisymOrthoMatTemperatureTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MAT3 that the card extends; the tables are TABLEMi cards.
    m_id.set(row, visitor.visit("id", ReferenceTarget::Material, m_id.get(row)));
    if (const base::EntityId id = m_youngsModulusXTableId.get(row); id != 0)
        m_youngsModulusXTableId.set(row, visitor.visit("youngsModulusXTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_youngsModulusThetaTableId.get(row); id != 0)
        m_youngsModulusThetaTableId.set(row,
                                        visitor.visit("youngsModulusThetaTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_youngsModulusZTableId.get(row); id != 0)
        m_youngsModulusZTableId.set(row, visitor.visit("youngsModulusZTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_poissonRatioXthTableId.get(row); id != 0)
        m_poissonRatioXthTableId.set(row, visitor.visit("poissonRatioXthTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_poissonRatioThzTableId.get(row); id != 0)
        m_poissonRatioThzTableId.set(row, visitor.visit("poissonRatioThzTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_poissonRatioZxTableId.get(row); id != 0)
        m_poissonRatioZxTableId.set(row, visitor.visit("poissonRatioZxTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_densityTableId.get(row); id != 0)
        m_densityTableId.set(row, visitor.visit("densityTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_shearModulusZxTableId.get(row); id != 0)
        m_shearModulusZxTableId.set(row, visitor.visit("shearModulusZxTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_thermalExpansionXTableId.get(row); id != 0)
        m_thermalExpansionXTableId.set(row,
                                       visitor.visit("thermalExpansionXTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_thermalExpansionThetaTableId.get(row); id != 0)
        m_thermalExpansionThetaTableId.set(
            row, visitor.visit("thermalExpansionThetaTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_thermalExpansionZTableId.get(row); id != 0)
        m_thermalExpansionZTableId.set(row,
                                       visitor.visit("thermalExpansionZTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_structuralDampingTableId.get(row); id != 0)
        m_structuralDampingTableId.set(row,
                                       visitor.visit("structuralDampingTableId", ReferenceTarget::MaterialTable, id));
}

} // namespace oor::model
