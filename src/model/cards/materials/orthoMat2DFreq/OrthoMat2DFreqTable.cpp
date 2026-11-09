#include "oor/model/cards/materials/orthoMat2DFreq/OrthoMat2DFreqTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELD_DAMPING = 17;

} // namespace

std::optional<OrthoMat2DFreqCard> OrthoMat2DFreqFormat::read(FieldReader& reader)
{
    OrthoMat2DFreqCard card;
    card.id = reader.requiredInteger(1, "id");
    card.youngsModulus1TableId = reader.integer(2, "youngsModulus1TableId", 0);
    card.youngsModulus2TableId = reader.integer(3, "youngsModulus2TableId", 0);
    card.poissonRatio12TableId = reader.integer(4, "poissonRatio12TableId", 0);
    card.shearModulus12TableId = reader.integer(5, "shearModulus12TableId", 0);
    card.shearModulus1ZTableId = reader.integer(6, "shearModulus1ZTableId", 0);
    card.shearModulus2ZTableId = reader.integer(7, "shearModulus2ZTableId", 0);
    card.structuralDampingTableId = reader.integer(FIELD_DAMPING, "structuralDampingTableId", 0);
    requireBlankFields(reader, {8, 9, 10, 11, 12, 13, 14, 15, 16}, FIELD_DAMPING);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> OrthoMat2DFreqFormat::check(const OrthoMat2DFreqCard& card)
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
    return checkTableReference(card.structuralDampingTableId, "structuralDampingTableId");
}

void OrthoMat2DFreqFormat::write(const OrthoMat2DFreqCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.youngsModulus1TableId, 0);
    writer.integerUnlessDefault(3, card.youngsModulus2TableId, 0);
    writer.integerUnlessDefault(4, card.poissonRatio12TableId, 0);
    writer.integerUnlessDefault(5, card.shearModulus12TableId, 0);
    writer.integerUnlessDefault(6, card.shearModulus1ZTableId, 0);
    writer.integerUnlessDefault(7, card.shearModulus2ZTableId, 0);
    writer.integerUnlessDefault(FIELD_DAMPING, card.structuralDampingTableId, 0);
}

std::uint32_t OrthoMat2DFreqTable::append(const OrthoMat2DFreqCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void OrthoMat2DFreqTable::store(std::uint32_t row, const OrthoMat2DFreqCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void OrthoMat2DFreqTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void OrthoMat2DFreqTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void OrthoMat2DFreqTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MAT8 that the card extends; the tables are TABLEDi cards.
    OrthoMat2DFreqCard& card = m_cards.edit(row);
    card.id = visitor.visit("id", ReferenceTarget::Material, card.id);
    m_id.set(row, card.id);
    const auto visitTable = [&](base::EntityId& table, const char* field)
    {
        if (table != 0)
            table = visitor.visit(field, ReferenceTarget::DynamicTable, table);
    };
    visitTable(card.youngsModulus1TableId, "youngsModulus1TableId");
    visitTable(card.youngsModulus2TableId, "youngsModulus2TableId");
    visitTable(card.poissonRatio12TableId, "poissonRatio12TableId");
    visitTable(card.shearModulus12TableId, "shearModulus12TableId");
    visitTable(card.shearModulus1ZTableId, "shearModulus1ZTableId");
    visitTable(card.shearModulus2ZTableId, "shearModulus2ZTableId");
    visitTable(card.structuralDampingTableId, "structuralDampingTableId");
}

} // namespace oor::model
