#include "oor/model/cards/elements/gap2/Gap2Table.hpp"

#include <variant>

namespace oor::model
{

std::optional<Gap2Card> Gap2Format::read(FieldReader& reader)
{
    Gap2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.integer(2, "propId", card.id); // format reference: PID defaults to EID
    card.nodeA = reader.requiredInteger(3, "nodeA");
    card.nodeB = reader.requiredInteger(4, "nodeB");
    // Field 5 is GO (an integer) or X1 (a real); a blank one is the vector form, whose X2 and X3 may still be there.
    const auto orientation = reader.integerOrReal(5, "orientationNode/orientation1");
    if (orientation && std::holds_alternative<std::int32_t>(*orientation))
    {
        card.orientationNode = std::get<std::int32_t>(*orientation);
        if (card.orientationNode <= 0)
            reader.error("card.bad-value", std::string("orientationNode") + " must be a positive grid point ID",
                         "orientationNode");
    }
    else
    {
        card.orientation[0] = orientation ? std::get<double>(*orientation) : 0.0;
        card.orientation[1] = reader.real(6, "orientation2", 0.0);
        card.orientation[2] = reader.real(7, "orientation3", 0.0);
    }
    card.frameId = reader.integer(8, "frameId", Gap2Card::NO_FRAME_ID);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> Gap2Format::check(const Gap2Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    if (card.nodeA <= 0 || card.nodeB <= 0)
        return CardProblem{"card.bad-id", "the grid point IDs must be positive integers", "nodeA"};
    if (card.nodeA == card.nodeB)
        return CardProblem{"card.duplicate-node", "{field:nodeA} and {field} must be different", "nodeB"};
    if (card.orientationNode < 0)
        return CardProblem{"card.bad-value", "{field} must be a positive grid point ID", "orientationNode"};
    if (card.orientationNode != 0 && card.hasVector())
        return CardProblem{"card.exclusive-fields",
                           "the orientation is a grid point {field} or a vector {field:orientation1} "
                           "{field:orientation2} {field:orientation3}, not both",
                           "orientationNode"};
    if (card.frameId < Gap2Card::NO_FRAME_ID)
        return CardProblem{"card.bad-value", "{field} must be blank or a coordinate system ID", "frameId"};
    return std::nullopt;
}

void Gap2Format::write(const Gap2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.integer(3, card.nodeA);
    writer.integer(4, card.nodeB);
    if (card.orientationNode != 0)
    {
        writer.integer(5, card.orientationNode);
    }
    else if (card.hasVector())
    {
        // The three components are written together, as the format reference lays them out.
        writer.real(5, card.orientation[0]);
        writer.real(6, card.orientation[1]);
        writer.real(7, card.orientation[2]);
    }
    if (card.frameId != Gap2Card::NO_FRAME_ID)
        writer.integer(8, card.frameId);
}

std::uint32_t Gap2Table::append(const Gap2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodeA.push(card.nodeA);
    m_nodeB.push(card.nodeB);
    m_orientationNode.push(card.orientationNode);
    m_orientation.push(card.orientation);
    m_frameId.push(card.frameId);
    return row;
}

Gap2Card Gap2Table::card(std::uint32_t row) const
{
    return Gap2Card{m_id.get(row),     m_propId.get(row),          m_nodeA.get(row),
                       m_nodeB.get(row),  m_orientationNode.get(row), m_orientation.get(row),
                       m_frameId.get(row)};
}

void Gap2Table::store(std::uint32_t row, const Gap2Card& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodeA.set(row, card.nodeA);
    m_nodeB.set(row, card.nodeB);
    m_orientationNode.set(row, card.orientationNode);
    m_orientation.set(row, card.orientation);
    m_frameId.set(row, card.frameId);
}

void Gap2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodeA.keepRows(rows);
    m_nodeB.keepRows(rows);
    m_orientationNode.keepRows(rows);
    m_orientation.keepRows(rows);
    m_frameId.keepRows(rows);
}

void Gap2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    m_nodeA.set(row, visitor.visit("nodeA", ReferenceTarget::Node, m_nodeA.get(row)));
    m_nodeB.set(row, visitor.visit("nodeB", ReferenceTarget::Node, m_nodeB.get(row)));
    if (const base::EntityId orientationNode = m_orientationNode.get(row); orientationNode > 0)
        m_orientationNode.set(row, visitor.visit("orientationNode", ReferenceTarget::Node, orientationNode));
    // CID 0 is the basic system and NO_FRAME_ID is blank: neither refers to a card.
    if (const base::EntityId frameId = m_frameId.get(row); frameId > 0)
        m_frameId.set(row, visitor.visit("frameId", ReferenceTarget::Coordinate, frameId));
}

} // namespace oor::model
