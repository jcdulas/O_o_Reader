#include "oor/model/cards/materials/isoMatTemperature/IsoMatTemperatureTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

std::optional<IsoMatTemperatureCard> IsoMatTemperatureFormat::read(FieldReader& reader)
{
    IsoMatTemperatureCard card;
    card.id = reader.requiredInteger(1, "id");
    card.youngsModulusTableId = reader.integer(2, "youngsModulusTableId", 0);
    card.shearModulusTableId = reader.integer(3, "shearModulusTableId", 0);
    card.poissonRatioTableId = reader.integer(4, "poissonRatioTableId", 0);
    card.densityTableId = reader.integer(5, "densityTableId", 0);
    card.thermalExpansionTableId = reader.integer(6, "thermalExpansionTableId", 0);
    card.structuralDampingTableId = reader.integer(8, "structuralDampingTableId", 0);
    card.tensionAllowableTableId = reader.integer(9, "tensionAllowableTableId", 0);
    card.compressionAllowableTableId = reader.integer(10, "compressionAllowableTableId", 0);
    card.shearAllowableTableId = reader.integer(11, "shearAllowableTableId", 0);
    requireBlankFields(reader, {7}, 11);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> IsoMatTemperatureFormat::check(const IsoMatTemperatureCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (const auto problem = checkTableReference(card.youngsModulusTableId, "youngsModulusTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.shearModulusTableId, "shearModulusTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.poissonRatioTableId, "poissonRatioTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.densityTableId, "densityTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.thermalExpansionTableId, "thermalExpansionTableId", true))
        return problem;
    if (const auto problem = checkTableReference(card.structuralDampingTableId, "structuralDampingTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.tensionAllowableTableId, "tensionAllowableTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.compressionAllowableTableId, "compressionAllowableTableId"))
        return problem;
    return checkTableReference(card.shearAllowableTableId, "shearAllowableTableId");
}

void IsoMatTemperatureFormat::write(const IsoMatTemperatureCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.youngsModulusTableId, 0);
    writer.integerUnlessDefault(3, card.shearModulusTableId, 0);
    writer.integerUnlessDefault(4, card.poissonRatioTableId, 0);
    writer.integerUnlessDefault(5, card.densityTableId, 0);
    writer.integerUnlessDefault(6, card.thermalExpansionTableId, 0);
    writer.integerUnlessDefault(8, card.structuralDampingTableId, 0);
    writer.integerUnlessDefault(9, card.tensionAllowableTableId, 0);
    writer.integerUnlessDefault(10, card.compressionAllowableTableId, 0);
    writer.integerUnlessDefault(11, card.shearAllowableTableId, 0);
}

std::uint32_t IsoMatTemperatureTable::append(const IsoMatTemperatureCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_youngsModulusTableId.push(card.youngsModulusTableId);
    m_shearModulusTableId.push(card.shearModulusTableId);
    m_poissonRatioTableId.push(card.poissonRatioTableId);
    m_densityTableId.push(card.densityTableId);
    m_thermalExpansionTableId.push(card.thermalExpansionTableId);
    m_structuralDampingTableId.push(card.structuralDampingTableId);
    m_tensionAllowableTableId.push(card.tensionAllowableTableId);
    m_compressionAllowableTableId.push(card.compressionAllowableTableId);
    m_shearAllowableTableId.push(card.shearAllowableTableId);
    return row;
}

IsoMatTemperatureCard IsoMatTemperatureTable::card(std::uint32_t row) const
{
    IsoMatTemperatureCard card;
    card.id = m_id.get(row);
    card.youngsModulusTableId = m_youngsModulusTableId.get(row);
    card.shearModulusTableId = m_shearModulusTableId.get(row);
    card.poissonRatioTableId = m_poissonRatioTableId.get(row);
    card.densityTableId = m_densityTableId.get(row);
    card.thermalExpansionTableId = m_thermalExpansionTableId.get(row);
    card.structuralDampingTableId = m_structuralDampingTableId.get(row);
    card.tensionAllowableTableId = m_tensionAllowableTableId.get(row);
    card.compressionAllowableTableId = m_compressionAllowableTableId.get(row);
    card.shearAllowableTableId = m_shearAllowableTableId.get(row);
    return card;
}

void IsoMatTemperatureTable::store(std::uint32_t row, const IsoMatTemperatureCard& card)
{
    m_id.set(row, card.id);
    m_youngsModulusTableId.set(row, card.youngsModulusTableId);
    m_shearModulusTableId.set(row, card.shearModulusTableId);
    m_poissonRatioTableId.set(row, card.poissonRatioTableId);
    m_densityTableId.set(row, card.densityTableId);
    m_thermalExpansionTableId.set(row, card.thermalExpansionTableId);
    m_structuralDampingTableId.set(row, card.structuralDampingTableId);
    m_tensionAllowableTableId.set(row, card.tensionAllowableTableId);
    m_compressionAllowableTableId.set(row, card.compressionAllowableTableId);
    m_shearAllowableTableId.set(row, card.shearAllowableTableId);
}

void IsoMatTemperatureTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_youngsModulusTableId.keepRows(rows);
    m_shearModulusTableId.keepRows(rows);
    m_poissonRatioTableId.keepRows(rows);
    m_densityTableId.keepRows(rows);
    m_thermalExpansionTableId.keepRows(rows);
    m_structuralDampingTableId.keepRows(rows);
    m_tensionAllowableTableId.keepRows(rows);
    m_compressionAllowableTableId.keepRows(rows);
    m_shearAllowableTableId.keepRows(rows);
}

void IsoMatTemperatureTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MAT1 that the card extends; the tables are TABLEMi cards (a negative T(A) is the table
    // of thermal strains of that number).
    m_id.set(row, visitor.visit("id", ReferenceTarget::Material, m_id.get(row)));
    if (const base::EntityId id = m_youngsModulusTableId.get(row); id != 0)
        m_youngsModulusTableId.set(row, visitor.visit("youngsModulusTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_shearModulusTableId.get(row); id != 0)
        m_shearModulusTableId.set(row, visitor.visit("shearModulusTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_poissonRatioTableId.get(row); id != 0)
        m_poissonRatioTableId.set(row, visitor.visit("poissonRatioTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_densityTableId.get(row); id != 0)
        m_densityTableId.set(row, visitor.visit("densityTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_thermalExpansionTableId.get(row); id != 0)
        m_thermalExpansionTableId.set(row,
                                      visitor.visit("thermalExpansionTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_structuralDampingTableId.get(row); id != 0)
        m_structuralDampingTableId.set(row,
                                       visitor.visit("structuralDampingTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_tensionAllowableTableId.get(row); id != 0)
        m_tensionAllowableTableId.set(row,
                                      visitor.visit("tensionAllowableTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_compressionAllowableTableId.get(row); id != 0)
        m_compressionAllowableTableId.set(
            row, visitor.visit("compressionAllowableTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_shearAllowableTableId.get(row); id != 0)
        m_shearAllowableTableId.set(row, visitor.visit("shearAllowableTableId", ReferenceTarget::MaterialTable, id));
}

} // namespace oor::model
