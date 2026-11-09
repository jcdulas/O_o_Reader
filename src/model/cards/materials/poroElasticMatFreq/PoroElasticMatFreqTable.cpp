#include "oor/model/cards/materials/poroElasticMatFreq/PoroElasticMatFreqTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELD_TGE = 8;
constexpr int FIELD_TST = 9; // T(ST), T(SC) and T(SS): fields 9 to 11

} // namespace

std::optional<PoroElasticMatFreqCard> PoroElasticMatFreqFormat::read(FieldReader& reader)
{
    PoroElasticMatFreqCard card;
    card.id = reader.requiredInteger(1, "id");
    card.youngsModulusTableId = reader.integer(2, "youngsModulusTableId", 0);
    card.shearModulusTableId = reader.integer(3, "shearModulusTableId", 0);
    card.poissonRatioTableId = reader.integer(4, "poissonRatioTableId", 0);
    card.densityTableId = reader.integer(5, "densityTableId", 0);
    card.thermalExpansionTableId = reader.integer(6, "thermalExpansionTableId", 0);
    card.structuralDampingTableId = reader.integer(FIELD_TGE, "structuralDampingTableId", 0);
    card.tensionAllowableTableId = reader.integer(FIELD_TST, "tensionAllowableTableId", 0);
    card.compressionAllowableTableId = reader.integer(FIELD_TST + 1, "compressionAllowableTableId", 0);
    card.shearAllowableTableId = reader.integer(FIELD_TST + 2, "shearAllowableTableId", 0);
    requireBlankFields(reader, {7}, FIELD_TST + 2);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PoroElasticMatFreqFormat::check(const PoroElasticMatFreqCard& card)
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
    if (const auto problem = checkTableReference(card.thermalExpansionTableId, "thermalExpansionTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.structuralDampingTableId, "structuralDampingTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.tensionAllowableTableId, "tensionAllowableTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.compressionAllowableTableId, "compressionAllowableTableId"))
        return problem;
    return checkTableReference(card.shearAllowableTableId, "shearAllowableTableId");
}

void PoroElasticMatFreqFormat::write(const PoroElasticMatFreqCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.youngsModulusTableId, 0);
    writer.integerUnlessDefault(3, card.shearModulusTableId, 0);
    writer.integerUnlessDefault(4, card.poissonRatioTableId, 0);
    writer.integerUnlessDefault(5, card.densityTableId, 0);
    writer.integerUnlessDefault(6, card.thermalExpansionTableId, 0);
    writer.integerUnlessDefault(FIELD_TGE, card.structuralDampingTableId, 0);
    writer.integerUnlessDefault(FIELD_TST, card.tensionAllowableTableId, 0);
    writer.integerUnlessDefault(FIELD_TST + 1, card.compressionAllowableTableId, 0);
    writer.integerUnlessDefault(FIELD_TST + 2, card.shearAllowableTableId, 0);
}

std::uint32_t PoroElasticMatFreqTable::append(const PoroElasticMatFreqCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void PoroElasticMatFreqTable::store(std::uint32_t row, const PoroElasticMatFreqCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void PoroElasticMatFreqTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void PoroElasticMatFreqTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void PoroElasticMatFreqTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MAT1 that the card extends; the tables are TABLEMi cards.
    PoroElasticMatFreqCard& card = m_cards.edit(row);
    card.id = visitor.visit("id", ReferenceTarget::Material, card.id);
    m_id.set(row, card.id);
    const auto visitTable = [&](base::EntityId& table, const char* field)
    {
        if (table != 0)
            table = visitor.visit(field, ReferenceTarget::MaterialTable, table);
    };
    visitTable(card.youngsModulusTableId, "youngsModulusTableId");
    visitTable(card.shearModulusTableId, "shearModulusTableId");
    visitTable(card.poissonRatioTableId, "poissonRatioTableId");
    visitTable(card.densityTableId, "densityTableId");
    visitTable(card.thermalExpansionTableId, "thermalExpansionTableId");
    visitTable(card.structuralDampingTableId, "structuralDampingTableId");
    visitTable(card.tensionAllowableTableId, "tensionAllowableTableId");
    visitTable(card.compressionAllowableTableId, "compressionAllowableTableId");
    visitTable(card.shearAllowableTableId, "shearAllowableTableId");
}

} // namespace oor::model
