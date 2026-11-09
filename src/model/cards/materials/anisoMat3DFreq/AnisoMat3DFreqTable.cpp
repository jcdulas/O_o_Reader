#include "oor/model/cards/materials/anisoMat3DFreq/AnisoMat3DFreqTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_STIFFNESS_FIELD = 2;     // G11 to G66: fields 2 to 22
constexpr int FIELD_DAMPING = 31;            // the GE of the element
constexpr int FIRST_DAMPING_TERM_FIELD = 33; // GE11 to GE66: fields 33 to 53

constexpr const char* STIFFNESS_NAMES[AnisoMat3DFreqCard::TERMS] = {
    "stiffness11TableId", "stiffness12TableId", "stiffness13TableId", "stiffness14TableId", "stiffness15TableId",
    "stiffness16TableId", "stiffness22TableId", "stiffness23TableId", "stiffness24TableId", "stiffness25TableId",
    "stiffness26TableId", "stiffness33TableId", "stiffness34TableId", "stiffness35TableId", "stiffness36TableId",
    "stiffness44TableId", "stiffness45TableId", "stiffness46TableId", "stiffness55TableId", "stiffness56TableId",
    "stiffness66TableId"};
constexpr const char* DAMPING_TERM_NAMES[AnisoMat3DFreqCard::TERMS] = {
    "structuralDamping11TableId", "structuralDamping12TableId", "structuralDamping13TableId",
    "structuralDamping14TableId", "structuralDamping15TableId", "structuralDamping16TableId",
    "structuralDamping22TableId", "structuralDamping23TableId", "structuralDamping24TableId",
    "structuralDamping25TableId", "structuralDamping26TableId", "structuralDamping33TableId",
    "structuralDamping34TableId", "structuralDamping35TableId", "structuralDamping36TableId",
    "structuralDamping44TableId", "structuralDamping45TableId", "structuralDamping46TableId",
    "structuralDamping55TableId", "structuralDamping56TableId", "structuralDamping66TableId"};

} // namespace

std::optional<AnisoMat3DFreqCard> AnisoMat3DFreqFormat::read(FieldReader& reader)
{
    AnisoMat3DFreqCard card;
    card.id = reader.requiredInteger(1, "id");
    for (std::size_t index = 0; index < card.stiffnessTableIds.size(); ++index)
    {
        card.stiffnessTableIds[index] =
            reader.integer(FIRST_STIFFNESS_FIELD + static_cast<int>(index), STIFFNESS_NAMES[index], 0);
        card.structuralDampingTermTableIds[index] =
            reader.integer(FIRST_DAMPING_TERM_FIELD + static_cast<int>(index), DAMPING_TERM_NAMES[index], 0);
    }
    card.structuralDampingTableId = reader.integer(FIELD_DAMPING, "structuralDampingTableId", 0);
    requireBlankFields(reader, {23, 24, 25, 26, 27, 28, 29, 30, 32},
                       FIRST_DAMPING_TERM_FIELD + static_cast<int>(AnisoMat3DFreqCard::TERMS) - 1);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AnisoMat3DFreqFormat::check(const AnisoMat3DFreqCard& card)
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

void AnisoMat3DFreqFormat::write(const AnisoMat3DFreqCard& card, FieldWriter& writer)
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

std::uint32_t AnisoMat3DFreqTable::append(const AnisoMat3DFreqCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AnisoMat3DFreqTable::store(std::uint32_t row, const AnisoMat3DFreqCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AnisoMat3DFreqTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AnisoMat3DFreqTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AnisoMat3DFreqTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MAT9 that the card extends; the tables are TABLEDi cards.
    AnisoMat3DFreqCard& card = m_cards.edit(row);
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
