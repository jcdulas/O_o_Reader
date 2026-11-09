#include "oor/model/cards/materials/orthoMat3DTemperature/OrthoMat3DTemperatureTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

std::optional<OrthoMat3DTemperatureCard> OrthoMat3DTemperatureFormat::read(FieldReader& reader)
{
    OrthoMat3DTemperatureCard card;
    card.id = reader.requiredInteger(1, "id");
    card.youngsModulus1TableId = reader.integer(2, "youngsModulus1TableId", 0);
    card.youngsModulus2TableId = reader.integer(3, "youngsModulus2TableId", 0);
    card.youngsModulus3TableId = reader.integer(4, "youngsModulus3TableId", 0);
    card.poissonRatio12TableId = reader.integer(5, "poissonRatio12TableId", 0);
    card.poissonRatio13TableId = reader.integer(6, "poissonRatio13TableId", 0);
    card.poissonRatio23TableId = reader.integer(7, "poissonRatio23TableId", 0);
    card.shearModulus12TableId = reader.integer(8, "shearModulus12TableId", 0);
    card.shearModulus13TableId = reader.integer(9, "shearModulus13TableId", 0);
    card.shearModulus23TableId = reader.integer(10, "shearModulus23TableId", 0);
    card.densityTableId = reader.integer(11, "densityTableId", 0);
    card.thermalExpansion1TableId = reader.integer(12, "thermalExpansion1TableId", 0);
    card.thermalExpansion2TableId = reader.integer(13, "thermalExpansion2TableId", 0);
    card.thermalExpansion3TableId = reader.integer(14, "thermalExpansion3TableId", 0);
    card.structuralDampingTableId = reader.integer(16, "structuralDampingTableId", 0);
    requireBlankFields(reader, {15}, 16);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> OrthoMat3DTemperatureFormat::check(const OrthoMat3DTemperatureCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (const auto problem = checkTableReference(card.youngsModulus1TableId, "youngsModulus1TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.youngsModulus2TableId, "youngsModulus2TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.youngsModulus3TableId, "youngsModulus3TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.poissonRatio12TableId, "poissonRatio12TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.poissonRatio13TableId, "poissonRatio13TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.poissonRatio23TableId, "poissonRatio23TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.shearModulus12TableId, "shearModulus12TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.shearModulus13TableId, "shearModulus13TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.shearModulus23TableId, "shearModulus23TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.densityTableId, "densityTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.thermalExpansion1TableId, "thermalExpansion1TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.thermalExpansion2TableId, "thermalExpansion2TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.thermalExpansion3TableId, "thermalExpansion3TableId"))
        return problem;
    return checkTableReference(card.structuralDampingTableId, "structuralDampingTableId");
}

void OrthoMat3DTemperatureFormat::write(const OrthoMat3DTemperatureCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.youngsModulus1TableId, 0);
    writer.integerUnlessDefault(3, card.youngsModulus2TableId, 0);
    writer.integerUnlessDefault(4, card.youngsModulus3TableId, 0);
    writer.integerUnlessDefault(5, card.poissonRatio12TableId, 0);
    writer.integerUnlessDefault(6, card.poissonRatio13TableId, 0);
    writer.integerUnlessDefault(7, card.poissonRatio23TableId, 0);
    writer.integerUnlessDefault(8, card.shearModulus12TableId, 0);
    writer.integerUnlessDefault(9, card.shearModulus13TableId, 0);
    writer.integerUnlessDefault(10, card.shearModulus23TableId, 0);
    writer.integerUnlessDefault(11, card.densityTableId, 0);
    writer.integerUnlessDefault(12, card.thermalExpansion1TableId, 0);
    writer.integerUnlessDefault(13, card.thermalExpansion2TableId, 0);
    writer.integerUnlessDefault(14, card.thermalExpansion3TableId, 0);
    writer.integerUnlessDefault(16, card.structuralDampingTableId, 0);
}

std::uint32_t OrthoMat3DTemperatureTable::append(const OrthoMat3DTemperatureCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_youngsModulus1TableId.push(card.youngsModulus1TableId);
    m_youngsModulus2TableId.push(card.youngsModulus2TableId);
    m_youngsModulus3TableId.push(card.youngsModulus3TableId);
    m_poissonRatio12TableId.push(card.poissonRatio12TableId);
    m_poissonRatio13TableId.push(card.poissonRatio13TableId);
    m_poissonRatio23TableId.push(card.poissonRatio23TableId);
    m_shearModulus12TableId.push(card.shearModulus12TableId);
    m_shearModulus13TableId.push(card.shearModulus13TableId);
    m_shearModulus23TableId.push(card.shearModulus23TableId);
    m_densityTableId.push(card.densityTableId);
    m_thermalExpansion1TableId.push(card.thermalExpansion1TableId);
    m_thermalExpansion2TableId.push(card.thermalExpansion2TableId);
    m_thermalExpansion3TableId.push(card.thermalExpansion3TableId);
    m_structuralDampingTableId.push(card.structuralDampingTableId);
    return row;
}

OrthoMat3DTemperatureCard OrthoMat3DTemperatureTable::card(std::uint32_t row) const
{
    OrthoMat3DTemperatureCard card;
    card.id = m_id.get(row);
    card.youngsModulus1TableId = m_youngsModulus1TableId.get(row);
    card.youngsModulus2TableId = m_youngsModulus2TableId.get(row);
    card.youngsModulus3TableId = m_youngsModulus3TableId.get(row);
    card.poissonRatio12TableId = m_poissonRatio12TableId.get(row);
    card.poissonRatio13TableId = m_poissonRatio13TableId.get(row);
    card.poissonRatio23TableId = m_poissonRatio23TableId.get(row);
    card.shearModulus12TableId = m_shearModulus12TableId.get(row);
    card.shearModulus13TableId = m_shearModulus13TableId.get(row);
    card.shearModulus23TableId = m_shearModulus23TableId.get(row);
    card.densityTableId = m_densityTableId.get(row);
    card.thermalExpansion1TableId = m_thermalExpansion1TableId.get(row);
    card.thermalExpansion2TableId = m_thermalExpansion2TableId.get(row);
    card.thermalExpansion3TableId = m_thermalExpansion3TableId.get(row);
    card.structuralDampingTableId = m_structuralDampingTableId.get(row);
    return card;
}

void OrthoMat3DTemperatureTable::store(std::uint32_t row, const OrthoMat3DTemperatureCard& card)
{
    m_id.set(row, card.id);
    m_youngsModulus1TableId.set(row, card.youngsModulus1TableId);
    m_youngsModulus2TableId.set(row, card.youngsModulus2TableId);
    m_youngsModulus3TableId.set(row, card.youngsModulus3TableId);
    m_poissonRatio12TableId.set(row, card.poissonRatio12TableId);
    m_poissonRatio13TableId.set(row, card.poissonRatio13TableId);
    m_poissonRatio23TableId.set(row, card.poissonRatio23TableId);
    m_shearModulus12TableId.set(row, card.shearModulus12TableId);
    m_shearModulus13TableId.set(row, card.shearModulus13TableId);
    m_shearModulus23TableId.set(row, card.shearModulus23TableId);
    m_densityTableId.set(row, card.densityTableId);
    m_thermalExpansion1TableId.set(row, card.thermalExpansion1TableId);
    m_thermalExpansion2TableId.set(row, card.thermalExpansion2TableId);
    m_thermalExpansion3TableId.set(row, card.thermalExpansion3TableId);
    m_structuralDampingTableId.set(row, card.structuralDampingTableId);
}

void OrthoMat3DTemperatureTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_youngsModulus1TableId.keepRows(rows);
    m_youngsModulus2TableId.keepRows(rows);
    m_youngsModulus3TableId.keepRows(rows);
    m_poissonRatio12TableId.keepRows(rows);
    m_poissonRatio13TableId.keepRows(rows);
    m_poissonRatio23TableId.keepRows(rows);
    m_shearModulus12TableId.keepRows(rows);
    m_shearModulus13TableId.keepRows(rows);
    m_shearModulus23TableId.keepRows(rows);
    m_densityTableId.keepRows(rows);
    m_thermalExpansion1TableId.keepRows(rows);
    m_thermalExpansion2TableId.keepRows(rows);
    m_thermalExpansion3TableId.keepRows(rows);
    m_structuralDampingTableId.keepRows(rows);
}

void OrthoMat3DTemperatureTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MAT11 that the card extends; the tables are TABLEMi cards.
    m_id.set(row, visitor.visit("id", ReferenceTarget::Material, m_id.get(row)));
    if (const base::EntityId id = m_youngsModulus1TableId.get(row); id != 0)
        m_youngsModulus1TableId.set(row, visitor.visit("youngsModulus1TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_youngsModulus2TableId.get(row); id != 0)
        m_youngsModulus2TableId.set(row, visitor.visit("youngsModulus2TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_youngsModulus3TableId.get(row); id != 0)
        m_youngsModulus3TableId.set(row, visitor.visit("youngsModulus3TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_poissonRatio12TableId.get(row); id != 0)
        m_poissonRatio12TableId.set(row, visitor.visit("poissonRatio12TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_poissonRatio13TableId.get(row); id != 0)
        m_poissonRatio13TableId.set(row, visitor.visit("poissonRatio13TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_poissonRatio23TableId.get(row); id != 0)
        m_poissonRatio23TableId.set(row, visitor.visit("poissonRatio23TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_shearModulus12TableId.get(row); id != 0)
        m_shearModulus12TableId.set(row, visitor.visit("shearModulus12TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_shearModulus13TableId.get(row); id != 0)
        m_shearModulus13TableId.set(row, visitor.visit("shearModulus13TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_shearModulus23TableId.get(row); id != 0)
        m_shearModulus23TableId.set(row, visitor.visit("shearModulus23TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_densityTableId.get(row); id != 0)
        m_densityTableId.set(row, visitor.visit("densityTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_thermalExpansion1TableId.get(row); id != 0)
        m_thermalExpansion1TableId.set(row,
                                       visitor.visit("thermalExpansion1TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_thermalExpansion2TableId.get(row); id != 0)
        m_thermalExpansion2TableId.set(row,
                                       visitor.visit("thermalExpansion2TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_thermalExpansion3TableId.get(row); id != 0)
        m_thermalExpansion3TableId.set(row,
                                       visitor.visit("thermalExpansion3TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_structuralDampingTableId.get(row); id != 0)
        m_structuralDampingTableId.set(row,
                                       visitor.visit("structuralDampingTableId", ReferenceTarget::MaterialTable, id));
}

} // namespace oor::model
