#include "oor/model/cards/explicit/occupantJoint/OccupantJointTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/explicit/ExternalProgramNames.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <vector>

namespace oor::model
{

namespace
{

// The places of the fields of the two lines of grid points and element numbers, and their names in the format
// reference.
constexpr int NODE_FIELDS[8] = {9, 10, 11, 12, 17, 18, 19, 20};
constexpr int ELEM_ID_FIELDS[6] = {13, 14, 15, 21, 22, 23};
constexpr const char* NODE_NAMES[8] = {"orientationNode", "node1", "node2", "node3",
                                       "node4",           "node5", "node6", "node7"};
constexpr const char* NODE_VISIT_NAMES[8] = {"orientationNode", "node1", "node2", "node3",
                                             "node4",           "node5", "node6", "node7"};
constexpr const char* ELEM_ID_NAMES[6] = {"elemId1", "elemId2", "elemId3", "elemId4", "elemId5", "elemId6"};

} // namespace

std::optional<OccupantJointCard> OccupantJointFormat::read(FieldReader& reader)
{
    OccupantJointCard card;
    card.id = reader.requiredInteger(1, "id");
    card.name = readExternalName(reader, 2, "jointName");
    for (std::size_t index = 0; index < card.grids.size(); ++index)
        card.grids[index] = reader.requiredInteger(NODE_FIELDS[index], NODE_NAMES[index]);
    for (std::size_t index = 0; index < card.elemIds.size(); ++index)
        card.elemIds[index] = reader.integer(ELEM_ID_FIELDS[index], ELEM_ID_NAMES[index], 0);
    std::vector<int> used = {1, 2};
    used.insert(used.end(), std::begin(NODE_FIELDS), std::end(NODE_FIELDS));
    used.insert(used.end(), std::begin(ELEM_ID_FIELDS), std::end(ELEM_ID_FIELDS));
    requireOnlyFields(reader, used);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> OccupantJointFormat::check(const OccupantJointCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkNameWord(card.name, "jointName"))
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
    return std::nullopt;
}

void OccupantJointFormat::write(const OccupantJointCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.name);
    for (std::size_t index = 0; index < card.grids.size(); ++index)
        writer.integer(NODE_FIELDS[index], card.grids[index]);
    for (std::size_t index = 0; index < card.elemIds.size(); ++index)
        writer.integerUnlessDefault(ELEM_ID_FIELDS[index], card.elemIds[index], 0);
}

std::uint32_t OccupantJointTable::append(const OccupantJointCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void OccupantJointTable::store(std::uint32_t row, const OccupantJointCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void OccupantJointTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void OccupantJointTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void OccupantJointTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    OccupantJointCard& card = m_cards.edit(row);
    for (std::size_t index = 0; index < card.grids.size(); ++index)
        visitId(visitor, NODE_VISIT_NAMES[index], ReferenceTarget::Node, card.grids[index]);
}

} // namespace oor::model
