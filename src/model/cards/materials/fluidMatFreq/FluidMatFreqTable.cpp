#include "oor/model/cards/materials/fluidMatFreq/FluidMatFreqTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

std::optional<FluidMatFreqCard> FluidMatFreqFormat::read(FieldReader& reader)
{
    FluidMatFreqCard card;
    card.id = reader.requiredInteger(1, "id");
    card.bulkModulusTableId = reader.integer(2, "bulkModulusTableId", 0);
    card.densityTableId = reader.integer(3, "densityTableId", 0);
    card.soundSpeedTableId = reader.integer(4, "soundSpeedTableId", 0);
    card.structuralDampingTableId = reader.integer(5, "structuralDampingTableId", 0);
    card.normalizedAdmittanceTableId = reader.integer(6, "normalizedAdmittanceTableId", 0);
    requireBlankFields(reader, {}, 6);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FluidMatFreqFormat::check(const FluidMatFreqCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (const auto problem = checkTableReference(card.bulkModulusTableId, "bulkModulusTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.densityTableId, "densityTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.soundSpeedTableId, "soundSpeedTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.structuralDampingTableId, "structuralDampingTableId"))
        return problem;
    return checkTableReference(card.normalizedAdmittanceTableId, "normalizedAdmittanceTableId");
}

void FluidMatFreqFormat::write(const FluidMatFreqCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.bulkModulusTableId, 0);
    writer.integerUnlessDefault(3, card.densityTableId, 0);
    writer.integerUnlessDefault(4, card.soundSpeedTableId, 0);
    writer.integerUnlessDefault(5, card.structuralDampingTableId, 0);
    writer.integerUnlessDefault(6, card.normalizedAdmittanceTableId, 0);
}

std::uint32_t FluidMatFreqTable::append(const FluidMatFreqCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void FluidMatFreqTable::store(std::uint32_t row, const FluidMatFreqCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void FluidMatFreqTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void FluidMatFreqTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void FluidMatFreqTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MAT10 that the card extends; the tables are TABLEDi cards.
    FluidMatFreqCard& card = m_cards.edit(row);
    card.id = visitor.visit("id", ReferenceTarget::Material, card.id);
    m_id.set(row, card.id);
    const auto visitTable = [&](base::EntityId& table, const char* field)
    {
        if (table != 0)
            table = visitor.visit(field, ReferenceTarget::DynamicTable, table);
    };
    visitTable(card.bulkModulusTableId, "bulkModulusTableId");
    visitTable(card.densityTableId, "densityTableId");
    visitTable(card.soundSpeedTableId, "soundSpeedTableId");
    visitTable(card.structuralDampingTableId, "structuralDampingTableId");
    visitTable(card.normalizedAdmittanceTableId, "normalizedAdmittanceTableId");
}

} // namespace oor::model
