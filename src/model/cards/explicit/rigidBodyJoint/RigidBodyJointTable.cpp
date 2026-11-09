#include "oor/model/cards/explicit/rigidBodyJoint/RigidBodyJointTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <vector>

namespace oor::model
{

namespace
{

// The node fields start at field 3; RPS is in field 10 (field 9 is blank).
constexpr int FIRST_NODE_FIELD = 3;
constexpr int PENALTY_SCALE_FIELD = 10;
constexpr const char* NODE_NAMES[6] = {"node1", "node2", "node3", "node4", "node5", "node6"};
constexpr const char* NODE_VISIT_NAMES[6] = {"node1", "node2", "node3", "node4", "node5", "node6"};

} // namespace

std::optional<RigidBodyJointCard> RigidBodyJointFormat::read(FieldReader& reader)
{
    RigidBodyJointCard card;
    card.id = reader.requiredInteger(1, "id");
    card.type = readRequiredKeyword<RigidBodyJointType>(reader, 2, "jointType", RIGID_BODY_JOINT_TYPE_WORDS);
    // One reader for every type: the type says how many nodes the joint has, and every other field must be blank.
    const std::size_t count = rigidBodyJointNodeCount(card.type);
    std::vector<int> used = {1, 2, PENALTY_SCALE_FIELD};
    for (std::size_t index = 0; index < count; ++index)
    {
        const int field = FIRST_NODE_FIELD + static_cast<int>(index);
        card.nodes.push_back(reader.requiredInteger(field, NODE_NAMES[index]));
        used.push_back(field);
    }
    card.penaltyScale = reader.real(PENALTY_SCALE_FIELD, "penaltyScale", 1.0);
    requireOnlyFields(reader, used);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RigidBodyJointFormat::check(const RigidBodyJointCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    const std::size_t count = rigidBodyJointNodeCount(card.type);
    if (card.nodes.size() != count)
    {
        // The field named is the first node that is missing, or the last one the type has when there are too many.
        const char* field = NODE_NAMES[card.nodes.size() < count ? card.nodes.size() : count - 1];
        return CardProblem{"card.bad-value",
                           "a joint of " + CardProblem::fieldNamed("jointType") + " " +
                               std::string(keywordText(RIGID_BODY_JOINT_TYPE_WORDS, card.type)) + " has " +
                               std::to_string(count) + " nodes, not " + std::to_string(card.nodes.size()),
                           field};
    }
    for (std::size_t index = 0; index < count; ++index)
    {
        if (const auto problem = checkRequiredId(card.nodes[index], NODE_NAMES[index]))
            return problem;
    }
    return checkReal(card.penaltyScale, "penaltyScale", RealRule::NonNegative);
}

void RigidBodyJointFormat::write(const RigidBodyJointCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeyword(writer, 2, RIGID_BODY_JOINT_TYPE_WORDS, card.type);
    for (std::size_t index = 0; index < card.nodes.size(); ++index)
        writer.integer(FIRST_NODE_FIELD + static_cast<int>(index), card.nodes[index]);
    writer.realUnlessDefault(PENALTY_SCALE_FIELD, card.penaltyScale, 1.0);
}

std::uint32_t RigidBodyJointTable::append(const RigidBodyJointCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void RigidBodyJointTable::store(std::uint32_t row, const RigidBodyJointCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void RigidBodyJointTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void RigidBodyJointTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void RigidBodyJointTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    RigidBodyJointCard& card = m_cards.edit(row);
    for (std::size_t index = 0; index < card.nodes.size() && index < 6; ++index)
        visitId(visitor, NODE_VISIT_NAMES[index], ReferenceTarget::Node, card.nodes[index]);
}

} // namespace oor::model
