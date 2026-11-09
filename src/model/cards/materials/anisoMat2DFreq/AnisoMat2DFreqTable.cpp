#include "oor/model/cards/materials/anisoMat2DFreq/AnisoMat2DFreqTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_STIFFNESS_FIELD = 2;     // G11 to G33: fields 2 to 7
constexpr int FIELD_DAMPING = 13;            // the GE of the element
constexpr int FIRST_DAMPING_TERM_FIELD = 18; // GE11 to GE33: fields 18 to 23

constexpr const char* STIFFNESS_NAMES[AnisoMat2DFreqCard::TERMS] = {"stiffness11TableId", "stiffness12TableId",
                                                                    "stiffness13TableId", "stiffness22TableId",
                                                                    "stiffness23TableId", "stiffness33TableId"};
constexpr const char* DAMPING_TERM_NAMES[AnisoMat2DFreqCard::TERMS] = {
    "structuralDamping11TableId", "structuralDamping12TableId", "structuralDamping13TableId",
    "structuralDamping22TableId", "structuralDamping23TableId", "structuralDamping33TableId"};

} // namespace

std::optional<AnisoMat2DFreqCard> AnisoMat2DFreqFormat::read(FieldReader& reader)
{
    AnisoMat2DFreqCard card;
    card.id = reader.requiredInteger(1, "id");
    for (std::size_t index = 0; index < card.stiffnessTableIds.size(); ++index)
    {
        card.stiffnessTableIds[index] =
            reader.integer(FIRST_STIFFNESS_FIELD + static_cast<int>(index), STIFFNESS_NAMES[index], 0);
        card.structuralDampingTermTableIds[index] =
            reader.integer(FIRST_DAMPING_TERM_FIELD + static_cast<int>(index), DAMPING_TERM_NAMES[index], 0);
    }
    card.structuralDampingTableId = reader.integer(FIELD_DAMPING, "structuralDampingTableId", 0);
    requireBlankFields(reader, {8, 9, 10, 11, 12, 14, 15, 16, 17},
                       FIRST_DAMPING_TERM_FIELD + static_cast<int>(AnisoMat2DFreqCard::TERMS) - 1);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AnisoMat2DFreqFormat::check(const AnisoMat2DFreqCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    for (std::size_t index = 0; index < card.stiffnessTableIds.size(); ++index)
    {
        if (const auto problem = checkTableReference(card.stiffnessTableIds[index], STIFFNESS_NAMES[index]))
            return problem;
        if (const auto problem =
                checkTableReference(card.structuralDampingTermTableIds[index], DAMPING_TERM_NAMES[index]))
            return problem;
    }
    return checkTableReference(card.structuralDampingTableId, "structuralDampingTableId");
}

void AnisoMat2DFreqFormat::write(const AnisoMat2DFreqCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (std::size_t index = 0; index < card.stiffnessTableIds.size(); ++index)
    {
        writer.integerUnlessDefault(FIRST_STIFFNESS_FIELD + static_cast<int>(index), card.stiffnessTableIds[index], 0);
        writer.integerUnlessDefault(FIRST_DAMPING_TERM_FIELD + static_cast<int>(index),
                                    card.structuralDampingTermTableIds[index], 0);
    }
    writer.integerUnlessDefault(FIELD_DAMPING, card.structuralDampingTableId, 0);
}

std::uint32_t AnisoMat2DFreqTable::append(const AnisoMat2DFreqCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AnisoMat2DFreqTable::store(std::uint32_t row, const AnisoMat2DFreqCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AnisoMat2DFreqTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AnisoMat2DFreqTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AnisoMat2DFreqTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MAT2 that the card extends; the tables are TABLEDi cards.
    AnisoMat2DFreqCard& card = m_cards.edit(row);
    card.id = visitor.visit("id", ReferenceTarget::Material, card.id);
    m_id.set(row, card.id);
    const auto visitTable = [&](base::EntityId& table, const char* field)
    {
        if (table != 0)
            table = visitor.visit(field, ReferenceTarget::DynamicTable, table);
    };
    for (std::size_t index = 0; index < card.stiffnessTableIds.size(); ++index)
    {
        visitTable(card.stiffnessTableIds[index], STIFFNESS_NAMES[index]);
        visitTable(card.structuralDampingTermTableIds[index], DAMPING_TERM_NAMES[index]);
    }
    visitTable(card.structuralDampingTableId, "structuralDampingTableId");
}

} // namespace oor::model
