#include "oor/model/cards/loads/pointForceLoad/PointForceLoadTable.hpp"

namespace oor::model
{

std::optional<PointForceLoadCard> PointForceLoadFormat::read(FieldReader& reader)
{
    PointForceLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.node = reader.requiredInteger(2, "node");
    card.frameId = reader.integer(3, "frameId", 0);
    card.magnitude = reader.requiredReal(4, "magnitude");
    card.direction[0] = reader.real(5, "directionX", 0.0);
    card.direction[1] = reader.real(6, "directionY", 0.0);
    card.direction[2] = reader.real(7, "directionZ", 0.0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PointForceLoadFormat::check(const PointForceLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    if (card.frameId < 0)
        return CardProblem{"card.bad-value", "the coordinate system ID cannot be negative", "frameId"};
    return std::nullopt;
}

void PointForceLoadFormat::write(const PointForceLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.node);
    writer.integerUnlessDefault(3, card.frameId, 0);
    writer.real(4, card.magnitude);
    writer.realUnlessDefault(5, card.direction[0], 0.0);
    writer.realUnlessDefault(6, card.direction[1], 0.0);
    writer.realUnlessDefault(7, card.direction[2], 0.0);
}

std::uint32_t PointForceLoadTable::append(const PointForceLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_node.push(card.node);
    m_frameId.push(card.frameId);
    m_magnitude.push(card.magnitude);
    m_direction.push(card.direction);
    return row;
}

PointForceLoadCard PointForceLoadTable::card(std::uint32_t row) const
{
    return PointForceLoadCard{m_id.get(row), m_node.get(row), m_frameId.get(row), m_magnitude.get(row),
                              m_direction.get(row)};
}

void PointForceLoadTable::store(std::uint32_t row, const PointForceLoadCard& card)
{
    m_id.set(row, card.id);
    m_node.set(row, card.node);
    m_frameId.set(row, card.frameId);
    m_magnitude.set(row, card.magnitude);
    m_direction.set(row, card.direction);
}

void PointForceLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_node.keepRows(rows);
    m_frameId.keepRows(rows);
    m_magnitude.keepRows(rows);
    m_direction.keepRows(rows);
}

void PointForceLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_node.set(row, visitor.visit("node", ReferenceTarget::Node, m_node.get(row)));
    if (const base::EntityId frameId = m_frameId.get(row); frameId > 0)
        m_frameId.set(row, visitor.visit("frameId", ReferenceTarget::Coordinate, frameId));
}

} // namespace oor::model
