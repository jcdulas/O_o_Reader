#include "oor/model/cards/materials/axisymOrthoMatStressDependent/AxisymOrthoMatStressDependentTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

std::optional<AxisymOrthoMatStressDependentCard> AxisymOrthoMatStressDependentFormat::read(FieldReader& reader)
{
    AxisymOrthoMatStressDependentCard card;
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
    requireBlankFields(reader, {9, 10}, 14);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymOrthoMatStressDependentFormat::check(const AxisymOrthoMatStressDependentCard& card)
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
    return checkTableReference(card.thermalExpansionZTableId, "thermalExpansionZTableId");
}

void AxisymOrthoMatStressDependentFormat::write(const AxisymOrthoMatStressDependentCard& card, FieldWriter& writer)
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
}

std::uint32_t AxisymOrthoMatStressDependentTable::append(const AxisymOrthoMatStressDependentCard& card,
                                                         base::CardKey key)
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
    return row;
}

AxisymOrthoMatStressDependentCard AxisymOrthoMatStressDependentTable::card(std::uint32_t row) const
{
    AxisymOrthoMatStressDependentCard card;
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
    return card;
}

void AxisymOrthoMatStressDependentTable::store(std::uint32_t row, const AxisymOrthoMatStressDependentCard& card)
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
}

void AxisymOrthoMatStressDependentTable::keepRows(const std::vector<std::uint32_t>& rows)
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
}

void AxisymOrthoMatStressDependentTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MAT3 that the card extends; every other field names a TABL3D0, TABL3D1 or TABL3D2 (0
    // when blank: the property is not a table).
    m_id.set(row, visitor.visit("id", ReferenceTarget::Material, m_id.get(row)));
    const auto visitTable = [&](Column<base::EntityId>& column, const char* field)
    {
        if (const base::EntityId id = column.get(row); id > 0)
            column.set(row, visitor.visit(field, ReferenceTarget::MultiDimensionalTable, id));
    };
    visitTable(m_youngsModulusXTableId, "youngsModulusXTableId");
    visitTable(m_youngsModulusThetaTableId, "youngsModulusThetaTableId");
    visitTable(m_youngsModulusZTableId, "youngsModulusZTableId");
    visitTable(m_poissonRatioXthTableId, "poissonRatioXthTableId");
    visitTable(m_poissonRatioThzTableId, "poissonRatioThzTableId");
    visitTable(m_poissonRatioZxTableId, "poissonRatioZxTableId");
    visitTable(m_densityTableId, "densityTableId");
    visitTable(m_shearModulusZxTableId, "shearModulusZxTableId");
    visitTable(m_thermalExpansionXTableId, "thermalExpansionXTableId");
    visitTable(m_thermalExpansionThetaTableId, "thermalExpansionThetaTableId");
    visitTable(m_thermalExpansionZTableId, "thermalExpansionZTableId");
}

} // namespace oor::model
