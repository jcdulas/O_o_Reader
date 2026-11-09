#include "oor/model/cards/materials/orthoMat2DStressDependent/OrthoMat2DStressDependentTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

std::optional<OrthoMat2DStressDependentCard> OrthoMat2DStressDependentFormat::read(FieldReader& reader)
{
    OrthoMat2DStressDependentCard card;
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
    requireBlankFields(reader, {}, 10);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> OrthoMat2DStressDependentFormat::check(const OrthoMat2DStressDependentCard& card)
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
    if (const auto problem = checkTableReference(card.thermalExpansion1TableId, "thermalExpansion1TableId"))
        return problem;
    return checkTableReference(card.thermalExpansion2TableId, "thermalExpansion2TableId");
}

void OrthoMat2DStressDependentFormat::write(const OrthoMat2DStressDependentCard& card, FieldWriter& writer)
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
}

std::uint32_t OrthoMat2DStressDependentTable::append(const OrthoMat2DStressDependentCard& card, base::CardKey key)
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
    return row;
}

OrthoMat2DStressDependentCard OrthoMat2DStressDependentTable::card(std::uint32_t row) const
{
    OrthoMat2DStressDependentCard card;
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
    return card;
}

void OrthoMat2DStressDependentTable::store(std::uint32_t row, const OrthoMat2DStressDependentCard& card)
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
}

void OrthoMat2DStressDependentTable::keepRows(const std::vector<std::uint32_t>& rows)
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
}

void OrthoMat2DStressDependentTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MAT8 that the card extends; every other field names a TABL3D0, TABL3D1 or TABL3D2 (0
    // when blank: the property is not a table).
    m_id.set(row, visitor.visit("id", ReferenceTarget::Material, m_id.get(row)));
    const auto visitTable = [&](Column<base::EntityId>& column, const char* field)
    {
        if (const base::EntityId id = column.get(row); id > 0)
            column.set(row, visitor.visit(field, ReferenceTarget::MultiDimensionalTable, id));
    };
    visitTable(m_youngsModulus1TableId, "youngsModulus1TableId");
    visitTable(m_youngsModulus2TableId, "youngsModulus2TableId");
    visitTable(m_poissonRatio12TableId, "poissonRatio12TableId");
    visitTable(m_shearModulus12TableId, "shearModulus12TableId");
    visitTable(m_shearModulus1ZTableId, "shearModulus1ZTableId");
    visitTable(m_shearModulus2ZTableId, "shearModulus2ZTableId");
    visitTable(m_densityTableId, "densityTableId");
    visitTable(m_thermalExpansion1TableId, "thermalExpansion1TableId");
    visitTable(m_thermalExpansion2TableId, "thermalExpansion2TableId");
}

} // namespace oor::model
