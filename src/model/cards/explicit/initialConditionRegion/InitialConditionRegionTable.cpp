#include "oor/model/cards/explicit/initialConditionRegion/InitialConditionRegionTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<InitialConditionRegionCard> InitialConditionRegionFormat::read(FieldReader& reader)
{
    InitialConditionRegionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.groupId = reader.requiredInteger(2, "groupId");
    card.type = readRequiredKeyword<EulerRegionType>(reader, 3, "shapeType", EULER_REGION_WORDS);
    card.shapeId = reader.requiredInteger(4, "shapeId");
    card.matId = reader.integer(5, "matId", 0);
    card.valuesId = reader.integer(6, "valuesId", 0);
    card.level = reader.real(7, "level", 0.0);
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 7 fields (one region)", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> InitialConditionRegionFormat::check(const InitialConditionRegionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.groupId, "groupId"))
        return problem;
    if (const auto problem = checkRequiredId(card.shapeId, "shapeId"))
        return problem;
    if (const auto problem = checkOptionalId(card.matId, "matId"))
        return problem;
    if (const auto problem = checkOptionalId(card.valuesId, "valuesId"))
        return problem;
    return checkReal(card.level, "level", RealRule::Finite);
}

void InitialConditionRegionFormat::write(const InitialConditionRegionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.groupId);
    writeKeyword(writer, 3, EULER_REGION_WORDS, card.type);
    writer.integer(4, card.shapeId);
    writer.integerUnlessDefault(5, card.matId, 0);
    writer.integerUnlessDefault(6, card.valuesId, 0);
    writer.realUnlessDefault(7, card.level, 0.0);
}

std::uint32_t InitialConditionRegionTable::append(const InitialConditionRegionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_groupId.push(card.groupId);
    m_cards.push(card);
    return row;
}

void InitialConditionRegionTable::store(std::uint32_t row, const InitialConditionRegionCard& card)
{
    m_groupId.set(row, card.groupId);
    m_cards.set(row, card);
}

void InitialConditionRegionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_groupId.keepRows(rows);
    m_cards.keepRows(rows);
}

void InitialConditionRegionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_groupId.set(row, id);
    m_cards.edit(row).groupId = id;
}

void InitialConditionRegionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    InitialConditionRegionCard& card = m_cards.edit(row);
    visitId(visitor, "shapeId", regionShapeTarget(card.type), card.shapeId);
    visitId(visitor, "matId", ReferenceTarget::Material, card.matId);
    visitId(visitor, "valuesId", ReferenceTarget::InitialValueSet, card.valuesId);
}

} // namespace oor::model
