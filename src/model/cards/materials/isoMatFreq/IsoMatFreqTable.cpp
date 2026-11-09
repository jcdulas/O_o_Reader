#include "oor/model/cards/materials/isoMatFreq/IsoMatFreqTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

std::optional<IsoMatFreqCard> IsoMatFreqFormat::read(FieldReader& reader)
{
    IsoMatFreqCard card;
    card.id = reader.requiredInteger(1, "id");
    card.youngsModulusTableId = reader.integer(2, "youngsModulusTableId", 0);
    card.shearModulusTableId = reader.integer(3, "shearModulusTableId", 0);
    card.poissonRatioTableId = reader.integer(4, "poissonRatioTableId", 0);
    card.structuralDampingTableId = reader.integer(8, "structuralDampingTableId", 0);
    requireBlankFields(reader, {5, 6, 7}, 8);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> IsoMatFreqFormat::check(const IsoMatFreqCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (const auto problem = checkTableReference(card.youngsModulusTableId, "youngsModulusTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.shearModulusTableId, "shearModulusTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.poissonRatioTableId, "poissonRatioTableId"))
        return problem;
    return checkTableReference(card.structuralDampingTableId, "structuralDampingTableId");
}

void IsoMatFreqFormat::write(const IsoMatFreqCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.youngsModulusTableId, 0);
    writer.integerUnlessDefault(3, card.shearModulusTableId, 0);
    writer.integerUnlessDefault(4, card.poissonRatioTableId, 0);
    writer.integerUnlessDefault(8, card.structuralDampingTableId, 0);
}

std::uint32_t IsoMatFreqTable::append(const IsoMatFreqCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_youngsModulusTableId.push(card.youngsModulusTableId);
    m_shearModulusTableId.push(card.shearModulusTableId);
    m_poissonRatioTableId.push(card.poissonRatioTableId);
    m_structuralDampingTableId.push(card.structuralDampingTableId);
    return row;
}

IsoMatFreqCard IsoMatFreqTable::card(std::uint32_t row) const
{
    IsoMatFreqCard card;
    card.id = m_id.get(row);
    card.youngsModulusTableId = m_youngsModulusTableId.get(row);
    card.shearModulusTableId = m_shearModulusTableId.get(row);
    card.poissonRatioTableId = m_poissonRatioTableId.get(row);
    card.structuralDampingTableId = m_structuralDampingTableId.get(row);
    return card;
}

void IsoMatFreqTable::store(std::uint32_t row, const IsoMatFreqCard& card)
{
    m_id.set(row, card.id);
    m_youngsModulusTableId.set(row, card.youngsModulusTableId);
    m_shearModulusTableId.set(row, card.shearModulusTableId);
    m_poissonRatioTableId.set(row, card.poissonRatioTableId);
    m_structuralDampingTableId.set(row, card.structuralDampingTableId);
}

void IsoMatFreqTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_youngsModulusTableId.keepRows(rows);
    m_shearModulusTableId.keepRows(rows);
    m_poissonRatioTableId.keepRows(rows);
    m_structuralDampingTableId.keepRows(rows);
}

void IsoMatFreqTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MAT1 that the card extends; the tables are TABLEDi cards.
    m_id.set(row, visitor.visit("id", ReferenceTarget::Material, m_id.get(row)));
    const auto visitTable = [&](Column<base::EntityId>& column, const char* field)
    {
        if (const base::EntityId id = column.get(row); id != 0)
            column.set(row, visitor.visit(field, ReferenceTarget::DynamicTable, id));
    };
    visitTable(m_youngsModulusTableId, "youngsModulusTableId");
    visitTable(m_shearModulusTableId, "shearModulusTableId");
    visitTable(m_poissonRatioTableId, "poissonRatioTableId");
    visitTable(m_structuralDampingTableId, "structuralDampingTableId");
}

} // namespace oor::model
