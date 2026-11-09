#include "oor/model/cards/rigid_elements/rigidTriaPlate1/RigidTriaPlate1Table.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/model/cards/rigid_elements/ComponentSets.hpp"

namespace oor::model
{

std::optional<RigidTriaPlate1Card> RigidTriaPlate1Format::read(FieldReader& reader)
{
    RigidTriaPlate1Card card;
    card.id = reader.requiredInteger(1, "id");
    card.nodes[0] = reader.requiredInteger(2, "nodeA");
    card.nodes[1] = reader.requiredInteger(3, "nodeB");
    card.nodes[2] = reader.requiredInteger(4, "nodeC");
    // A blank CMB or CMC is 123456: the point moves with GA as a rigid body.
    card.components[0] = base::sortedDigits(reader.integer(5, "componentsAtB", components::ALL));
    card.components[1] = base::sortedDigits(reader.integer(6, "componentsAtC", components::ALL));
    card.alphaCoefficient = reader.real(7, "alphaCoefficient", 0.0);
    card.referenceTemperature = reader.real(8, "referenceTemperature", 0.0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RigidTriaPlate1Format::check(const RigidTriaPlate1Card& card)
{
    static constexpr const char* NODE_NAMES[] = {"nodeA", "nodeB", "nodeC"};
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    for (std::size_t index = 0; index < 3; ++index)
    {
        if (card.nodes[index] <= 0)
            return CardProblem{"card.bad-id", "a grid point ID must be a positive integer", NODE_NAMES[index]};
        for (std::size_t other = 0; other < index; ++other)
        {
            if (card.nodes[other] == card.nodes[index])
                return CardProblem{"card.bad-id", "the three grid points must be different", NODE_NAMES[index]};
        }
    }
    if (card.components[0] == 0 || !base::isComponentSet(card.components[0]))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be a list of the digits 1 to 6, each at most once",
                           "componentsAtB"};
    if (card.components[1] == 0 || !base::isComponentSet(card.components[1]))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be a list of the digits 1 to 6, each at most once",
                           "componentsAtC"};
    return std::nullopt;
}

void RigidTriaPlate1Format::write(const RigidTriaPlate1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.nodes[0]);
    writer.integer(3, card.nodes[1]);
    writer.integer(4, card.nodes[2]);
    writer.integerUnlessDefault(5, card.components[0], components::ALL);
    writer.integerUnlessDefault(6, card.components[1], components::ALL);
    writer.realUnlessDefault(7, card.alphaCoefficient, 0.0);
    writer.realUnlessDefault(8, card.referenceTemperature, 0.0);
}

std::uint32_t RigidTriaPlate1Table::append(const RigidTriaPlate1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_nodes.push(card.nodes);
    m_dependentComponents.push(card.components);
    m_alphaCoefficient.push(card.alphaCoefficient);
    m_referenceTemperature.push(card.referenceTemperature);
    return row;
}

RigidTriaPlate1Card RigidTriaPlate1Table::card(std::uint32_t row) const
{
    return RigidTriaPlate1Card{m_id.get(row), m_nodes.get(row), m_dependentComponents.get(row),
                               m_alphaCoefficient.get(row), m_referenceTemperature.get(row)};
}

void RigidTriaPlate1Table::store(std::uint32_t row, const RigidTriaPlate1Card& card)
{
    m_id.set(row, card.id);
    m_nodes.set(row, card.nodes);
    m_dependentComponents.set(row, card.components);
    m_alphaCoefficient.set(row, card.alphaCoefficient);
    m_referenceTemperature.set(row, card.referenceTemperature);
}

void RigidTriaPlate1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_nodes.keepRows(rows);
    m_dependentComponents.keepRows(rows);
    m_alphaCoefficient.keepRows(rows);
    m_referenceTemperature.keepRows(rows);
}

void RigidTriaPlate1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    std::array<base::EntityId, 3> nodes = m_nodes.get(row);
    nodes[0] = visitor.visit("nodeA", ReferenceTarget::Node, nodes[0]);
    nodes[1] = visitor.visit("nodeB", ReferenceTarget::Node, nodes[1]);
    nodes[2] = visitor.visit("nodeC", ReferenceTarget::Node, nodes[2]);
    m_nodes.set(row, nodes);
}

} // namespace oor::model
