#include "oor/model/cards/materials/orthoMat2DTemperature/OrthoMat2DTemperatureTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

std::optional<OrthoMat2DTemperatureCard> OrthoMat2DTemperatureFormat::read(FieldReader& reader)
{
    OrthoMat2DTemperatureCard card;
    card.id = reader.requiredInteger(1, "id");
    card.youngsModulus1TableId = reader.integer(2, "youngsModulus1TableId", 0);
    card.youngsModulus2TableId = reader.integer(3, "youngsModulus2TableId", 0);
    card.poissonRatio12TableId = reader.integer(4, "poissonRatio12TableId", 0);
    card.shearModulus12TableId = reader.integer(5, "shearModulus12TableId", 0);
    card.shearModulus1ZTableId = reader.integer(6, "shearModulus1ZTableId", 0);
    card.shearModulus2ZTableId = reader.integer(7, "shearModulus2ZTableId", 0);
    card.densityTableId = reader.integer(8, "densityTableId", 0);
    card.thermalExpansion1TableId = reader.integer(9, "thermalExpansion1TableId", 0);
    card.thermalExpansion2TableId = reader.integer(10, "thermalExpansion2TableId", 0);
    card.tensionStrength1TableId = reader.integer(12, "tensionStrength1TableId", 0);
    card.compressionStrength1TableId = reader.integer(13, "compressionStrength1TableId", 0);
    card.tensionStrength2TableId = reader.integer(14, "tensionStrength2TableId", 0);
    card.compressionStrength2TableId = reader.integer(15, "compressionStrength2TableId", 0);
    card.shearStrengthTableId = reader.integer(16, "shearStrengthTableId", 0);
    card.structuralDampingTableId = reader.integer(17, "structuralDampingTableId", 0);
    card.interactionTerm12TableId = reader.integer(18, "interactionTerm12TableId", 0);
    requireBlankFields(reader, {11}, 18);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> OrthoMat2DTemperatureFormat::check(const OrthoMat2DTemperatureCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (const auto problem = checkTableReference(card.youngsModulus1TableId, "youngsModulus1TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.youngsModulus2TableId, "youngsModulus2TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.poissonRatio12TableId, "poissonRatio12TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.shearModulus12TableId, "shearModulus12TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.shearModulus1ZTableId, "shearModulus1ZTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.shearModulus2ZTableId, "shearModulus2ZTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.densityTableId, "densityTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.thermalExpansion1TableId, "thermalExpansion1TableId", true))
        return problem;
    if (const auto problem = checkTableReference(card.thermalExpansion2TableId, "thermalExpansion2TableId", true))
        return problem;
    if (const auto problem = checkTableReference(card.tensionStrength1TableId, "tensionStrength1TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.compressionStrength1TableId, "compressionStrength1TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.tensionStrength2TableId, "tensionStrength2TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.compressionStrength2TableId, "compressionStrength2TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.shearStrengthTableId, "shearStrengthTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.structuralDampingTableId, "structuralDampingTableId"))
        return problem;
    return checkTableReference(card.interactionTerm12TableId, "interactionTerm12TableId");
}

void OrthoMat2DTemperatureFormat::write(const OrthoMat2DTemperatureCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.youngsModulus1TableId, 0);
    writer.integerUnlessDefault(3, card.youngsModulus2TableId, 0);
    writer.integerUnlessDefault(4, card.poissonRatio12TableId, 0);
    writer.integerUnlessDefault(5, card.shearModulus12TableId, 0);
    writer.integerUnlessDefault(6, card.shearModulus1ZTableId, 0);
    writer.integerUnlessDefault(7, card.shearModulus2ZTableId, 0);
    writer.integerUnlessDefault(8, card.densityTableId, 0);
    writer.integerUnlessDefault(9, card.thermalExpansion1TableId, 0);
    writer.integerUnlessDefault(10, card.thermalExpansion2TableId, 0);
    writer.integerUnlessDefault(12, card.tensionStrength1TableId, 0);
    writer.integerUnlessDefault(13, card.compressionStrength1TableId, 0);
    writer.integerUnlessDefault(14, card.tensionStrength2TableId, 0);
    writer.integerUnlessDefault(15, card.compressionStrength2TableId, 0);
    writer.integerUnlessDefault(16, card.shearStrengthTableId, 0);
    writer.integerUnlessDefault(17, card.structuralDampingTableId, 0);
    writer.integerUnlessDefault(18, card.interactionTerm12TableId, 0);
}

std::uint32_t OrthoMat2DTemperatureTable::append(const OrthoMat2DTemperatureCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_youngsModulus1TableId.push(card.youngsModulus1TableId);
    m_youngsModulus2TableId.push(card.youngsModulus2TableId);
    m_poissonRatio12TableId.push(card.poissonRatio12TableId);
    m_shearModulus12TableId.push(card.shearModulus12TableId);
    m_shearModulus1ZTableId.push(card.shearModulus1ZTableId);
    m_shearModulus2ZTableId.push(card.shearModulus2ZTableId);
    m_densityTableId.push(card.densityTableId);
    m_thermalExpansion1TableId.push(card.thermalExpansion1TableId);
    m_thermalExpansion2TableId.push(card.thermalExpansion2TableId);
    m_tensionStrength1TableId.push(card.tensionStrength1TableId);
    m_compressionStrength1TableId.push(card.compressionStrength1TableId);
    m_tensionStrength2TableId.push(card.tensionStrength2TableId);
    m_compressionStrength2TableId.push(card.compressionStrength2TableId);
    m_shearStrengthTableId.push(card.shearStrengthTableId);
    m_structuralDampingTableId.push(card.structuralDampingTableId);
    m_interactionTerm12TableId.push(card.interactionTerm12TableId);
    return row;
}

OrthoMat2DTemperatureCard OrthoMat2DTemperatureTable::card(std::uint32_t row) const
{
    OrthoMat2DTemperatureCard card;
    card.id = m_id.get(row);
    card.youngsModulus1TableId = m_youngsModulus1TableId.get(row);
    card.youngsModulus2TableId = m_youngsModulus2TableId.get(row);
    card.poissonRatio12TableId = m_poissonRatio12TableId.get(row);
    card.shearModulus12TableId = m_shearModulus12TableId.get(row);
    card.shearModulus1ZTableId = m_shearModulus1ZTableId.get(row);
    card.shearModulus2ZTableId = m_shearModulus2ZTableId.get(row);
    card.densityTableId = m_densityTableId.get(row);
    card.thermalExpansion1TableId = m_thermalExpansion1TableId.get(row);
    card.thermalExpansion2TableId = m_thermalExpansion2TableId.get(row);
    card.tensionStrength1TableId = m_tensionStrength1TableId.get(row);
    card.compressionStrength1TableId = m_compressionStrength1TableId.get(row);
    card.tensionStrength2TableId = m_tensionStrength2TableId.get(row);
    card.compressionStrength2TableId = m_compressionStrength2TableId.get(row);
    card.shearStrengthTableId = m_shearStrengthTableId.get(row);
    card.structuralDampingTableId = m_structuralDampingTableId.get(row);
    card.interactionTerm12TableId = m_interactionTerm12TableId.get(row);
    return card;
}

void OrthoMat2DTemperatureTable::store(std::uint32_t row, const OrthoMat2DTemperatureCard& card)
{
    m_id.set(row, card.id);
    m_youngsModulus1TableId.set(row, card.youngsModulus1TableId);
    m_youngsModulus2TableId.set(row, card.youngsModulus2TableId);
    m_poissonRatio12TableId.set(row, card.poissonRatio12TableId);
    m_shearModulus12TableId.set(row, card.shearModulus12TableId);
    m_shearModulus1ZTableId.set(row, card.shearModulus1ZTableId);
    m_shearModulus2ZTableId.set(row, card.shearModulus2ZTableId);
    m_densityTableId.set(row, card.densityTableId);
    m_thermalExpansion1TableId.set(row, card.thermalExpansion1TableId);
    m_thermalExpansion2TableId.set(row, card.thermalExpansion2TableId);
    m_tensionStrength1TableId.set(row, card.tensionStrength1TableId);
    m_compressionStrength1TableId.set(row, card.compressionStrength1TableId);
    m_tensionStrength2TableId.set(row, card.tensionStrength2TableId);
    m_compressionStrength2TableId.set(row, card.compressionStrength2TableId);
    m_shearStrengthTableId.set(row, card.shearStrengthTableId);
    m_structuralDampingTableId.set(row, card.structuralDampingTableId);
    m_interactionTerm12TableId.set(row, card.interactionTerm12TableId);
}

void OrthoMat2DTemperatureTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_youngsModulus1TableId.keepRows(rows);
    m_youngsModulus2TableId.keepRows(rows);
    m_poissonRatio12TableId.keepRows(rows);
    m_shearModulus12TableId.keepRows(rows);
    m_shearModulus1ZTableId.keepRows(rows);
    m_shearModulus2ZTableId.keepRows(rows);
    m_densityTableId.keepRows(rows);
    m_thermalExpansion1TableId.keepRows(rows);
    m_thermalExpansion2TableId.keepRows(rows);
    m_tensionStrength1TableId.keepRows(rows);
    m_compressionStrength1TableId.keepRows(rows);
    m_tensionStrength2TableId.keepRows(rows);
    m_compressionStrength2TableId.keepRows(rows);
    m_shearStrengthTableId.keepRows(rows);
    m_structuralDampingTableId.keepRows(rows);
    m_interactionTerm12TableId.keepRows(rows);
}

void OrthoMat2DTemperatureTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MAT8 that the card extends; the tables are TABLEMi cards (a negative T(A1) or T(A2) is
    // the table of thermal strains of that number).
    m_id.set(row, visitor.visit("id", ReferenceTarget::Material, m_id.get(row)));
    if (const base::EntityId id = m_youngsModulus1TableId.get(row); id != 0)
        m_youngsModulus1TableId.set(row, visitor.visit("youngsModulus1TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_youngsModulus2TableId.get(row); id != 0)
        m_youngsModulus2TableId.set(row, visitor.visit("youngsModulus2TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_poissonRatio12TableId.get(row); id != 0)
        m_poissonRatio12TableId.set(row, visitor.visit("poissonRatio12TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_shearModulus12TableId.get(row); id != 0)
        m_shearModulus12TableId.set(row, visitor.visit("shearModulus12TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_shearModulus1ZTableId.get(row); id != 0)
        m_shearModulus1ZTableId.set(row, visitor.visit("shearModulus1ZTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_shearModulus2ZTableId.get(row); id != 0)
        m_shearModulus2ZTableId.set(row, visitor.visit("shearModulus2ZTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_densityTableId.get(row); id != 0)
        m_densityTableId.set(row, visitor.visit("densityTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_thermalExpansion1TableId.get(row); id != 0)
        m_thermalExpansion1TableId.set(row,
                                       visitor.visit("thermalExpansion1TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_thermalExpansion2TableId.get(row); id != 0)
        m_thermalExpansion2TableId.set(row,
                                       visitor.visit("thermalExpansion2TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_tensionStrength1TableId.get(row); id != 0)
        m_tensionStrength1TableId.set(row,
                                      visitor.visit("tensionStrength1TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_compressionStrength1TableId.get(row); id != 0)
        m_compressionStrength1TableId.set(
            row, visitor.visit("compressionStrength1TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_tensionStrength2TableId.get(row); id != 0)
        m_tensionStrength2TableId.set(row,
                                      visitor.visit("tensionStrength2TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_compressionStrength2TableId.get(row); id != 0)
        m_compressionStrength2TableId.set(
            row, visitor.visit("compressionStrength2TableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_shearStrengthTableId.get(row); id != 0)
        m_shearStrengthTableId.set(row, visitor.visit("shearStrengthTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_structuralDampingTableId.get(row); id != 0)
        m_structuralDampingTableId.set(row,
                                       visitor.visit("structuralDampingTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_interactionTerm12TableId.get(row); id != 0)
        m_interactionTerm12TableId.set(row,
                                       visitor.visit("interactionTerm12TableId", ReferenceTarget::MaterialTable, id));
}

} // namespace oor::model
