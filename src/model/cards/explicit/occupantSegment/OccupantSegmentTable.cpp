#include "oor/model/cards/explicit/occupantSegment/OccupantSegmentTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/explicit/ExternalProgramNames.hpp"

namespace oor::model
{

namespace
{

// The places of the fields of the second line and their names in the format reference.
constexpr int NODE_FIELDS[4] = {9, 10, 11, 12};
constexpr int ELEM_ID_FIELDS[3] = {13, 14, 15};
constexpr int CENTER_OF_GRAVITY_PROP_FIELD = 16;
constexpr const char* NODE_NAMES[4] = {"orientationNode", "node1", "node2", "node3"};
constexpr const char* NODE_VISIT_NAMES[4] = {"orientationNode", "node1", "node2", "node3"};
constexpr const char* ELEM_ID_NAMES[3] = {"elemId1", "elemId2", "elemId3"};

} // namespace

std::optional<OccupantSegmentCard> OccupantSegmentFormat::read(FieldReader& reader)
{
    OccupantSegmentCard card;
    card.id = reader.requiredInteger(1, "id");
    card.name = readExternalName(reader, 2, "segmentName");
    card.cover = readOptionalKeyword<OccupantSegmentCover>(reader, 3, "cover", OCCUPANT_SEGMENT_COVER_WORDS);
    card.elemCount = reader.integer(4, "elemCount", 0);
    card.firstNodeId = reader.integer(5, "firstNodeId", 0);
    card.firstElemId = reader.integer(6, "firstElemId", 0);
    card.matId = reader.integer(7, "matId", 0);
    card.coverPropId = reader.integer(8, "coverPropId", 0);
    for (std::size_t index = 0; index < card.grids.size(); ++index)
        card.grids[index] = reader.requiredInteger(NODE_FIELDS[index], NODE_NAMES[index]);
    for (std::size_t index = 0; index < card.elemIds.size(); ++index)
        card.elemIds[index] = reader.integer(ELEM_ID_FIELDS[index], ELEM_ID_NAMES[index], 0);
    card.centerOfGravityPropId = reader.integer(CENTER_OF_GRAVITY_PROP_FIELD, "centerOfGravityPropId", 0);
    if (reader.lastUsedField() > CENTER_OF_GRAVITY_PROP_FIELD)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 16 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> OccupantSegmentFormat::check(const OccupantSegmentCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkNameWord(card.name, "segmentName"))
        return problem;
    if (const auto problem = checkOptionalId(card.elemCount, "elemCount"))
        return problem;
    if (const auto problem = checkOptionalId(card.firstNodeId, "firstNodeId"))
        return problem;
    if (const auto problem = checkOptionalId(card.firstElemId, "firstElemId"))
        return problem;
    if (const auto problem = checkOptionalId(card.matId, "matId"))
        return problem;
    if (const auto problem = checkOptionalId(card.coverPropId, "coverPropId"))
        return problem;
    for (std::size_t index = 0; index < card.grids.size(); ++index)
    {
        if (const auto problem = checkRequiredId(card.grids[index], NODE_NAMES[index]))
            return problem;
    }
    for (std::size_t index = 0; index < card.elemIds.size(); ++index)
    {
        if (const auto problem = checkOptionalId(card.elemIds[index], ELEM_ID_NAMES[index]))
            return problem;
    }
    return checkOptionalId(card.centerOfGravityPropId, "centerOfGravityPropId");
}

void OccupantSegmentFormat::write(const OccupantSegmentCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.name);
    writeOptionalKeyword(writer, 3, OCCUPANT_SEGMENT_COVER_WORDS, card.cover);
    writer.integerUnlessDefault(4, card.elemCount, 0);
    writer.integerUnlessDefault(5, card.firstNodeId, 0);
    writer.integerUnlessDefault(6, card.firstElemId, 0);
    writer.integerUnlessDefault(7, card.matId, 0);
    writer.integerUnlessDefault(8, card.coverPropId, 0);
    for (std::size_t index = 0; index < card.grids.size(); ++index)
        writer.integer(NODE_FIELDS[index], card.grids[index]);
    for (std::size_t index = 0; index < card.elemIds.size(); ++index)
        writer.integerUnlessDefault(ELEM_ID_FIELDS[index], card.elemIds[index], 0);
    writer.integerUnlessDefault(CENTER_OF_GRAVITY_PROP_FIELD, card.centerOfGravityPropId, 0);
}

std::uint32_t OccupantSegmentTable::append(const OccupantSegmentCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void OccupantSegmentTable::store(std::uint32_t row, const OccupantSegmentCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void OccupantSegmentTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void OccupantSegmentTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void OccupantSegmentTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    OccupantSegmentCard& card = m_cards.edit(row);
    for (std::size_t index = 0; index < card.grids.size(); ++index)
        visitId(visitor, NODE_VISIT_NAMES[index], ReferenceTarget::Node, card.grids[index]);
}

} // namespace oor::model
