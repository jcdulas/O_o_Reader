#include "oor/model/cards/materials/orthoMat3DNonlinearStressDependent/OrthoMat3DNonlinearStressDependentTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

std::optional<OrthoMat3DNonlinearStressDependentCard> OrthoMat3DNonlinearStressDependentFormat::read(
    FieldReader& reader)
{
    OrthoMat3DNonlinearStressDependentCard card;
    card.id = reader.requiredInteger(1, "id");
    card.youngsModulus1TableId = reader.integer(2, "youngsModulus1TableId", 0);
    card.youngsModulus2TableId = reader.integer(3, "youngsModulus2TableId", 0);
    card.youngsModulus3TableId = reader.integer(4, "youngsModulus3TableId", 0);
    card.poissonRatio12TableId = reader.integer(5, "poissonRatio12TableId", 0);
    card.poissonRatio23TableId = reader.integer(6, "poissonRatio23TableId", 0);
    card.poissonRatio31TableId = reader.integer(7, "poissonRatio31TableId", 0);
    card.densityTableId = reader.integer(8, "densityTableId", 0);
    card.shearModulus12TableId = reader.integer(9, "shearModulus12TableId", 0);
    card.shearModulus23TableId = reader.integer(10, "shearModulus23TableId", 0);
    card.shearModulus31TableId = reader.integer(11, "shearModulus31TableId", 0);
    card.thermalExpansion1TableId = reader.integer(12, "thermalExpansion1TableId", 0);
    card.thermalExpansion2TableId = reader.integer(13, "thermalExpansion2TableId", 0);
    card.thermalExpansion3TableId = reader.integer(14, "thermalExpansion3TableId", 0);
    requireBlankFields(reader, {}, 14);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> OrthoMat3DNonlinearStressDependentFormat::check(
    const OrthoMat3DNonlinearStressDependentCard& card)
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
    if (const auto problem = checkTableReference(card.poissonRatio23TableId, "poissonRatio23TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.poissonRatio31TableId, "poissonRatio31TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.densityTableId, "densityTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.shearModulus12TableId, "shearModulus12TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.shearModulus23TableId, "shearModulus23TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.shearModulus31TableId, "shearModulus31TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.thermalExpansion1TableId, "thermalExpansion1TableId"))
        return problem;
    if (const auto problem = checkTableReference(card.thermalExpansion2TableId, "thermalExpansion2TableId"))
        return problem;
    return checkTableReference(card.thermalExpansion3TableId, "thermalExpansion3TableId");
}

void OrthoMat3DNonlinearStressDependentFormat::write(const OrthoMat3DNonlinearStressDependentCard& card,
                                                     FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.youngsModulus1TableId, 0);
    writer.integerUnlessDefault(3, card.youngsModulus2TableId, 0);
    writer.integerUnlessDefault(4, card.youngsModulus3TableId, 0);
    writer.integerUnlessDefault(5, card.poissonRatio12TableId, 0);
    writer.integerUnlessDefault(6, card.poissonRatio23TableId, 0);
    writer.integerUnlessDefault(7, card.poissonRatio31TableId, 0);
    writer.integerUnlessDefault(8, card.densityTableId, 0);
    writer.integerUnlessDefault(9, card.shearModulus12TableId, 0);
    writer.integerUnlessDefault(10, card.shearModulus23TableId, 0);
    writer.integerUnlessDefault(11, card.shearModulus31TableId, 0);
    writer.integerUnlessDefault(12, card.thermalExpansion1TableId, 0);
    writer.integerUnlessDefault(13, card.thermalExpansion2TableId, 0);
    writer.integerUnlessDefault(14, card.thermalExpansion3TableId, 0);
}

std::uint32_t OrthoMat3DNonlinearStressDependentTable::append(const OrthoMat3DNonlinearStressDependentCard& card,
                                                              base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_youngsModulus1TableId.push(card.youngsModulus1TableId);
    m_youngsModulus2TableId.push(card.youngsModulus2TableId);
    m_youngsModulus3TableId.push(card.youngsModulus3TableId);
    m_poissonRatio12TableId.push(card.poissonRatio12TableId);
    m_poissonRatio23TableId.push(card.poissonRatio23TableId);
    m_poissonRatio31TableId.push(card.poissonRatio31TableId);
    m_densityTableId.push(card.densityTableId);
    m_shearModulus12TableId.push(card.shearModulus12TableId);
    m_shearModulus23TableId.push(card.shearModulus23TableId);
    m_shearModulus31TableId.push(card.shearModulus31TableId);
    m_thermalExpansion1TableId.push(card.thermalExpansion1TableId);
    m_thermalExpansion2TableId.push(card.thermalExpansion2TableId);
    m_thermalExpansion3TableId.push(card.thermalExpansion3TableId);
    return row;
}

OrthoMat3DNonlinearStressDependentCard OrthoMat3DNonlinearStressDependentTable::card(std::uint32_t row) const
{
    OrthoMat3DNonlinearStressDependentCard card;
    card.id = m_id.get(row);
    card.youngsModulus1TableId = m_youngsModulus1TableId.get(row);
    card.youngsModulus2TableId = m_youngsModulus2TableId.get(row);
    card.youngsModulus3TableId = m_youngsModulus3TableId.get(row);
    card.poissonRatio12TableId = m_poissonRatio12TableId.get(row);
    card.poissonRatio23TableId = m_poissonRatio23TableId.get(row);
    card.poissonRatio31TableId = m_poissonRatio31TableId.get(row);
    card.densityTableId = m_densityTableId.get(row);
    card.shearModulus12TableId = m_shearModulus12TableId.get(row);
    card.shearModulus23TableId = m_shearModulus23TableId.get(row);
    card.shearModulus31TableId = m_shearModulus31TableId.get(row);
    card.thermalExpansion1TableId = m_thermalExpansion1TableId.get(row);
    card.thermalExpansion2TableId = m_thermalExpansion2TableId.get(row);
    card.thermalExpansion3TableId = m_thermalExpansion3TableId.get(row);
    return card;
}

void OrthoMat3DNonlinearStressDependentTable::store(std::uint32_t row,
                                                    const OrthoMat3DNonlinearStressDependentCard& card)
{
    m_id.set(row, card.id);
    m_youngsModulus1TableId.set(row, card.youngsModulus1TableId);
    m_youngsModulus2TableId.set(row, card.youngsModulus2TableId);
    m_youngsModulus3TableId.set(row, card.youngsModulus3TableId);
    m_poissonRatio12TableId.set(row, card.poissonRatio12TableId);
    m_poissonRatio23TableId.set(row, card.poissonRatio23TableId);
    m_poissonRatio31TableId.set(row, card.poissonRatio31TableId);
    m_densityTableId.set(row, card.densityTableId);
    m_shearModulus12TableId.set(row, card.shearModulus12TableId);
    m_shearModulus23TableId.set(row, card.shearModulus23TableId);
    m_shearModulus31TableId.set(row, card.shearModulus31TableId);
    m_thermalExpansion1TableId.set(row, card.thermalExpansion1TableId);
    m_thermalExpansion2TableId.set(row, card.thermalExpansion2TableId);
    m_thermalExpansion3TableId.set(row, card.thermalExpansion3TableId);
}

void OrthoMat3DNonlinearStressDependentTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_youngsModulus1TableId.keepRows(rows);
    m_youngsModulus2TableId.keepRows(rows);
    m_youngsModulus3TableId.keepRows(rows);
    m_poissonRatio12TableId.keepRows(rows);
    m_poissonRatio23TableId.keepRows(rows);
    m_poissonRatio31TableId.keepRows(rows);
    m_densityTableId.keepRows(rows);
    m_shearModulus12TableId.keepRows(rows);
    m_shearModulus23TableId.keepRows(rows);
    m_shearModulus31TableId.keepRows(rows);
    m_thermalExpansion1TableId.keepRows(rows);
    m_thermalExpansion2TableId.keepRows(rows);
    m_thermalExpansion3TableId.keepRows(rows);
}

void OrthoMat3DNonlinearStressDependentTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MATORT that the card extends; every other field names a TABL3D0, TABL3D1 or TABL3D2 (0
    // when blank: the property is not a table).
    m_id.set(row, visitor.visit("id", ReferenceTarget::Material, m_id.get(row)));
    const auto visitTable = [&](Column<base::EntityId>& column, const char* field)
    {
        if (const base::EntityId id = column.get(row); id > 0)
            column.set(row, visitor.visit(field, ReferenceTarget::MultiDimensionalTable, id));
    };
    visitTable(m_youngsModulus1TableId, "youngsModulus1TableId");
    visitTable(m_youngsModulus2TableId, "youngsModulus2TableId");
    visitTable(m_youngsModulus3TableId, "youngsModulus3TableId");
    visitTable(m_poissonRatio12TableId, "poissonRatio12TableId");
    visitTable(m_poissonRatio23TableId, "poissonRatio23TableId");
    visitTable(m_poissonRatio31TableId, "poissonRatio31TableId");
    visitTable(m_densityTableId, "densityTableId");
    visitTable(m_shearModulus12TableId, "shearModulus12TableId");
    visitTable(m_shearModulus23TableId, "shearModulus23TableId");
    visitTable(m_shearModulus31TableId, "shearModulus31TableId");
    visitTable(m_thermalExpansion1TableId, "thermalExpansion1TableId");
    visitTable(m_thermalExpansion2TableId, "thermalExpansion2TableId");
    visitTable(m_thermalExpansion3TableId, "thermalExpansion3TableId");
}

} // namespace oor::model
