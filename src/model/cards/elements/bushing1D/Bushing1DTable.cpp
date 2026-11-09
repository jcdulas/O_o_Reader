#include "oor/model/cards/elements/bushing1D/Bushing1DTable.hpp"

namespace oor::model
{

std::optional<Bushing1DCard> Bushing1DFormat::read(FieldReader& reader)
{
    Bushing1DCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.integer(2, "propId", card.id); // PID defaults to EID (the table is unclear)
    card.nodeA = reader.requiredInteger(3, "nodeA");
    card.nodeB = reader.integer(4, "nodeB", 0); // blank: grounded
    card.frameId = reader.integer(5, "frameId", Bushing1DCard::NO_FRAME_ID);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> Bushing1DFormat::check(const Bushing1DCard& card)
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
    if (card.frameId < Bushing1DCard::NO_FRAME_ID)
        return CardProblem{"card.bad-value", "{field} must be blank or a coordinate system ID", "frameId"};
    return std::nullopt;
}

void Bushing1DFormat::write(const Bushing1DCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.integer(3, card.nodeA);
    writer.integerUnlessDefault(4, card.nodeB, 0);
    if (card.frameId != Bushing1DCard::NO_FRAME_ID)
        writer.integer(5, card.frameId);
}

std::uint32_t Bushing1DTable::append(const Bushing1DCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodeA.push(card.nodeA);
    m_nodeB.push(card.nodeB);
    m_frameId.push(card.frameId);
    return row;
}

Bushing1DCard Bushing1DTable::card(std::uint32_t row) const
{
    return Bushing1DCard{m_id.get(row), m_propId.get(row), m_nodeA.get(row), m_nodeB.get(row), m_frameId.get(row)};
}

void Bushing1DTable::store(std::uint32_t row, const Bushing1DCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodeA.set(row, card.nodeA);
    m_nodeB.set(row, card.nodeB);
    m_frameId.set(row, card.frameId);
}

void Bushing1DTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodeA.keepRows(rows);
    m_nodeB.keepRows(rows);
    m_frameId.keepRows(rows);
}

void Bushing1DTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    m_nodeA.set(row, visitor.visit("nodeA", ReferenceTarget::Node, m_nodeA.get(row)));
    if (const base::EntityId nodeB = m_nodeB.get(row); nodeB > 0)
        m_nodeB.set(row, visitor.visit("nodeB", ReferenceTarget::Node, nodeB));
    // CID 0 is the basic system and NO_FRAME_ID is blank: neither refers to a card.
    if (const base::EntityId frameId = m_frameId.get(row); frameId > 0)
        m_frameId.set(row, visitor.visit("frameId", ReferenceTarget::Coordinate, frameId));
}

} // namespace oor::model
