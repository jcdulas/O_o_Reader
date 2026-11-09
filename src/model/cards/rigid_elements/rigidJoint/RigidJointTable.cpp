#include "oor/model/cards/rigid_elements/rigidJoint/RigidJointTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/model/cards/rigid_elements/ComponentSets.hpp"

namespace oor::model
{

std::optional<RigidJointCard> RigidJointFormat::read(FieldReader& reader)
{
    RigidJointCard card;
    card.id = reader.requiredInteger(1, "id");
    card.nodeA = reader.requiredInteger(2, "nodeA");
    card.nodeB = reader.requiredInteger(3, "nodeB");
    // A blank CB is 123456: GB moves with GA as a rigid body.
    card.componentsB = base::sortedDigits(reader.integer(4, "componentsB", components::ALL));
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RigidJointFormat::check(const RigidJointCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.nodeA <= 0)
        return CardProblem{"card.bad-id",
                           "the grid point " + std::string(CardProblem::FIELD) + " must be a positive integer",
                           "nodeA"};
    if (card.nodeB <= 0 || card.nodeB == card.nodeA)
        return CardProblem{"card.bad-id",
                           std::string(CardProblem::FIELD) +
                               " must be a positive integer, not the same grid point as " +
                               CardProblem::fieldNamed("nodeA"),
                           "nodeB"};
    if (card.componentsB == 0 || !base::isComponentSet(card.componentsB))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be a list of the digits 1 to 6, each at most once",
                           "componentsB"};
    return std::nullopt;
}

void RigidJointFormat::write(const RigidJointCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.nodeA);
    writer.integer(3, card.nodeB);
    writer.integerUnlessDefault(4, card.componentsB, components::ALL);
}

std::uint32_t RigidJointTable::append(const RigidJointCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_nodeA.push(card.nodeA);
    m_nodeB.push(card.nodeB);
    m_componentsB.push(card.componentsB);
    return row;
}

RigidJointCard RigidJointTable::card(std::uint32_t row) const
{
    return RigidJointCard{m_id.get(row), m_nodeA.get(row), m_nodeB.get(row), m_componentsB.get(row)};
}

void RigidJointTable::store(std::uint32_t row, const RigidJointCard& card)
{
    m_id.set(row, card.id);
    m_nodeA.set(row, card.nodeA);
    m_nodeB.set(row, card.nodeB);
    m_componentsB.set(row, card.componentsB);
}

void RigidJointTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_nodeA.keepRows(rows);
    m_nodeB.keepRows(rows);
    m_componentsB.keepRows(rows);
}

void RigidJointTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_nodeA.set(row, visitor.visit("nodeA", ReferenceTarget::Node, m_nodeA.get(row)));
    m_nodeB.set(row, visitor.visit("nodeB", ReferenceTarget::Node, m_nodeB.get(row)));
}

} // namespace oor::model
