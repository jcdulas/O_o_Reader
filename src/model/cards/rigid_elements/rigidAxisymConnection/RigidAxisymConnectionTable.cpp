#include "oor/model/cards/rigid_elements/rigidAxisymConnection/RigidAxisymConnectionTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 3;

} // namespace

std::optional<RigidAxisymConnectionCard> RigidAxisymConnectionFormat::read(FieldReader& reader)
{
    RigidAxisymConnectionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.node3D = reader.requiredInteger(2, "node3D");
    card.axisymNode = reader.requiredInteger(3, "axisymNode");
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RigidAxisymConnectionFormat::check(const RigidAxisymConnectionCard& card)
{
    if (card.id <= 0 || card.id >= RigidAxisymConnectionCard::ID_LIMIT)
        return CardProblem{"card.bad-id", "the element ID must be an integer from 1 to 99999999", "id"};
    // G3D is meant to be above 100,000,000, but small numbers are common: any positive number is accepted.
    if (const auto problem = checkRequiredId(card.node3D, "node3D"))
        return problem;
    if (card.axisymNode <= 0 || card.axisymNode >= RigidAxisymConnectionCard::ID_LIMIT)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be an integer from 1 to 99999999",
                           "axisymNode"};
    if (card.node3D == card.axisymNode)
        return CardProblem{"card.duplicate-node",
                           CardProblem::fieldNamed("node3D") + " and " + std::string(CardProblem::FIELD) +
                               " must be different grid points",
                           "axisymNode"};
    return std::nullopt;
}

void RigidAxisymConnectionFormat::write(const RigidAxisymConnectionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.node3D);
    writer.integer(3, card.axisymNode);
}

std::uint32_t RigidAxisymConnectionTable::append(const RigidAxisymConnectionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_node3D.push(card.node3D);
    m_axisymNode.push(card.axisymNode);
    return row;
}

RigidAxisymConnectionCard RigidAxisymConnectionTable::card(std::uint32_t row) const
{
    return RigidAxisymConnectionCard{m_id.get(row), m_node3D.get(row), m_axisymNode.get(row)};
}

void RigidAxisymConnectionTable::store(std::uint32_t row, const RigidAxisymConnectionCard& card)
{
    m_id.set(row, card.id);
    m_node3D.set(row, card.node3D);
    m_axisymNode.set(row, card.axisymNode);
}

void RigidAxisymConnectionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_node3D.keepRows(rows);
    m_axisymNode.keepRows(rows);
}

void RigidAxisymConnectionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_node3D.set(row, visitor.visit("node3D", ReferenceTarget::Node, m_node3D.get(row)));
    m_axisymNode.set(row, visitor.visit("axisymNode", ReferenceTarget::Node, m_axisymNode.get(row)));
}

} // namespace oor::model
