#include "oor/model/cards/elements/bushing/BushingTable.hpp"

#include <variant>

namespace oor::model
{

std::optional<BushingCard> BushingFormat::read(FieldReader& reader)
{
    BushingCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.integer(2, "propId", card.id); // format reference: PID defaults to EID
    card.nodeA = reader.requiredInteger(3, "nodeA");
    card.nodeB = reader.integer(4, "nodeB", 0); // blank: grounded
    // Field 5 is GO (an integer) or X1 (a real); X2 and X3 only count when X1 is there.
    if (const auto orientation = reader.integerOrReal(5, "orientationNode/orientation1"))
    {
        if (std::holds_alternative<std::int32_t>(*orientation))
        {
            card.orientationNode = std::get<std::int32_t>(*orientation);
            if (card.orientationNode <= 0)
                reader.error("card.bad-value", std::string("orientationNode") + " must be a positive grid point ID",
                             "orientationNode");
        }
        else
        {
            card.orientation[0] = std::get<double>(*orientation);
            card.orientation[1] = reader.real(6, "orientation2", 0.0);
            card.orientation[2] = reader.real(7, "orientation3", 0.0);
        }
    }
    card.frameId = reader.integer(8, "frameId", BushingCard::NO_FRAME_ID);
    card.offsetFraction = reader.real(9, "offsetFraction", BushingCard::DEFAULT_S);
    card.offsetFrameId = reader.integer(10, "offsetFrameId", BushingCard::NO_OFFSET_FRAME_ID);
    for (int index = 0; index < 3; ++index)
    {
        static constexpr const char* NAMES[3] = {"offset1", "offset2", "offset3"};
        card.offset[static_cast<std::size_t>(index)] = reader.real(11 + index, NAMES[index], 0.0);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BushingFormat::check(const BushingCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    if (card.nodeA <= 0)
        return CardProblem{"card.bad-id", "{field} must be a positive grid point ID", "nodeA"};
    if (card.nodeB < 0)
        return CardProblem{"card.bad-id", "{field} must be 0 (blank) or a grid point ID", "nodeB"};
    if (card.nodeB == card.nodeA)
        return CardProblem{"card.duplicate-node", "{field:nodeA} and {field} must be different", "nodeB"};
    if (card.orientationNode < 0)
        return CardProblem{"card.bad-value", "{field} must be a positive grid point ID", "orientationNode"};
    if (card.orientationNode != 0 && card.hasVector())
        return CardProblem{"card.exclusive-fields",
                           "the orientation is a grid point {field} or a vector {field:orientation1} "
                           "{field:orientation2} {field:orientation3}, not both",
                           "orientationNode"};
    if (card.frameId < BushingCard::NO_FRAME_ID)
        return CardProblem{"card.bad-value", "{field} must be blank or a coordinate system ID", "frameId"};
    if (card.offsetFrameId < BushingCard::NO_OFFSET_FRAME_ID)
        return CardProblem{"card.bad-value", "{field} must be -1 or a coordinate system ID", "offsetFrameId"};
    return std::nullopt;
}

void BushingFormat::write(const BushingCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.integer(3, card.nodeA);
    writer.integerUnlessDefault(4, card.nodeB, 0);
    if (card.orientationNode != 0)
    {
        writer.integer(5, card.orientationNode);
    }
    else if (card.hasVector())
    {
        // X2 and X3 are only read when X1 is there: the three are written together.
        writer.real(5, card.orientation[0]);
        writer.real(6, card.orientation[1]);
        writer.real(7, card.orientation[2]);
    }
    if (card.frameId != BushingCard::NO_FRAME_ID)
        writer.integer(8, card.frameId);
    // S is a static default (0.5): a blank is 0.5, and a written 0.0 must stay 0.0.
    writer.realUnlessDefault(9, card.offsetFraction, BushingCard::DEFAULT_S);
    writer.integerUnlessDefault(10, card.offsetFrameId, BushingCard::NO_OFFSET_FRAME_ID);
    for (int index = 0; index < 3; ++index)
        writer.realUnlessDefault(11 + index, card.offset[static_cast<std::size_t>(index)], 0.0);
}

std::uint32_t BushingTable::append(const BushingCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodeA.push(card.nodeA);
    m_nodeB.push(card.nodeB);
    m_orientationNode.push(card.orientationNode);
    m_orientation.push(card.orientation);
    m_frameId.push(card.frameId);
    m_offsetFraction.push(card.offsetFraction);
    m_offsetFrameId.push(card.offsetFrameId);
    m_offset.push(card.offset);
    return row;
}

BushingCard BushingTable::card(std::uint32_t row) const
{
    return BushingCard{m_id.get(row),      m_propId.get(row),          m_nodeA.get(row),
                        m_nodeB.get(row),   m_orientationNode.get(row), m_orientation.get(row),
                        m_frameId.get(row), m_offsetFraction.get(row),  m_offsetFrameId.get(row),
                        m_offset.get(row)};
}

void BushingTable::store(std::uint32_t row, const BushingCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodeA.set(row, card.nodeA);
    m_nodeB.set(row, card.nodeB);
    m_orientationNode.set(row, card.orientationNode);
    m_orientation.set(row, card.orientation);
    m_frameId.set(row, card.frameId);
    m_offsetFraction.set(row, card.offsetFraction);
    m_offsetFrameId.set(row, card.offsetFrameId);
    m_offset.set(row, card.offset);
}

void BushingTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodeA.keepRows(rows);
    m_nodeB.keepRows(rows);
    m_orientationNode.keepRows(rows);
    m_orientation.keepRows(rows);
    m_frameId.keepRows(rows);
    m_offsetFraction.keepRows(rows);
    m_offsetFrameId.keepRows(rows);
    m_offset.keepRows(rows);
}

void BushingTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    m_nodeA.set(row, visitor.visit("nodeA", ReferenceTarget::Node, m_nodeA.get(row)));
    if (const base::EntityId nodeB = m_nodeB.get(row); nodeB > 0)
        m_nodeB.set(row, visitor.visit("nodeB", ReferenceTarget::Node, nodeB));
    if (const base::EntityId orientationNode = m_orientationNode.get(row); orientationNode > 0)
        m_orientationNode.set(row, visitor.visit("orientationNode", ReferenceTarget::Node, orientationNode));
    // CID 0 is the basic system and NO_FRAME_ID is blank: neither refers to a card; the same for OCID.
    if (const base::EntityId frameId = m_frameId.get(row); frameId > 0)
        m_frameId.set(row, visitor.visit("frameId", ReferenceTarget::Coordinate, frameId));
    if (const base::EntityId offsetFrameId = m_offsetFrameId.get(row); offsetFrameId > 0)
        m_offsetFrameId.set(row, visitor.visit("offsetFrameId", ReferenceTarget::Coordinate, offsetFrameId));
}

} // namespace oor::model
