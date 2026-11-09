#include "oor/model/cards/explicit/eulerForceRegion/EulerForceRegionTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<EulerForceRegionCard> EulerForceRegionFormat::read(FieldReader& reader)
{
    EulerForceRegionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.setId = reader.requiredInteger(2, "setId");
    card.type = readRequiredKeyword<EulerRegionType>(reader, 3, "shapeType", EULER_REGION_WORDS);
    card.shapeId = reader.requiredInteger(4, "shapeId");
    card.matId = reader.requiredInteger(5, "matId");
    card.accelX = reader.requiredInteger(6, "accelX");
    card.accelY = reader.requiredInteger(7, "accelY");
    card.accelZ = reader.requiredInteger(8, "accelZ");
    card.level = reader.real(9, "level", 0.0);
    if (reader.lastUsedField() > 9)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 9 fields (one region)", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> EulerForceRegionFormat::check(const EulerForceRegionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.setId, "setId"))
        return problem;
    if (const auto problem = checkRequiredId(card.shapeId, "shapeId"))
        return problem;
    if (const auto problem = checkRequiredId(card.matId, "matId"))
        return problem;
    if (const auto problem = checkRequiredId(card.accelX, "accelX"))
        return problem;
    if (const auto problem = checkRequiredId(card.accelY, "accelY"))
        return problem;
    if (const auto problem = checkRequiredId(card.accelZ, "accelZ"))
        return problem;
    return checkReal(card.level, "level", RealRule::Finite);
}

void EulerForceRegionFormat::write(const EulerForceRegionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.setId);
    writeKeyword(writer, 3, EULER_REGION_WORDS, card.type);
    writer.integer(4, card.shapeId);
    writer.integer(5, card.matId);
    writer.integer(6, card.accelX);
    writer.integer(7, card.accelY);
    writer.integer(8, card.accelZ);
    writer.realUnlessDefault(9, card.level, 0.0);
}

std::uint32_t EulerForceRegionTable::append(const EulerForceRegionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_setId.push(card.setId);
    m_cards.push(card);
    return row;
}

void EulerForceRegionTable::store(std::uint32_t row, const EulerForceRegionCard& card)
{
    m_setId.set(row, card.setId);
    m_cards.set(row, card);
}

void EulerForceRegionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_setId.keepRows(rows);
    m_cards.keepRows(rows);
}

void EulerForceRegionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_setId.set(row, id);
    m_cards.edit(row).setId = id;
}

void EulerForceRegionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    EulerForceRegionCard& card = m_cards.edit(row);
    visitId(visitor, "shapeId", regionShapeTarget(card.type), card.shapeId);
    visitId(visitor, "matId", ReferenceTarget::Material, card.matId);
    visitId(visitor, "accelX", ReferenceTarget::DynamicTable, card.accelX);
    visitId(visitor, "accelY", ReferenceTarget::DynamicTable, card.accelY);
    visitId(visitor, "accelZ", ReferenceTarget::DynamicTable, card.accelZ);
}

} // namespace oor::model
