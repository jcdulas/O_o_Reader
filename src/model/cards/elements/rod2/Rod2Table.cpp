#include "oor/model/cards/elements/rod2/Rod2Table.hpp"

namespace oor::model
{

std::optional<Rod2Card> Rod2Format::read(FieldReader& reader)
{
    Rod2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.integer(2, "propId", card.id); // format reference: PID defaults to EID
    card.nodes[0] = reader.requiredInteger(3, "node1");
    card.nodes[1] = reader.requiredInteger(4, "node2");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> Rod2Format::check(const Rod2Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.nodes[0] == card.nodes[1])
        return CardProblem{"card.duplicate-node", "the two grid points must be different", "node1"};
    return std::nullopt;
}

void Rod2Format::write(const Rod2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.integer(3, card.nodes[0]);
    writer.integer(4, card.nodes[1]);
}

std::uint32_t Rod2Table::append(const Rod2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodes.push(card.nodes);
    return row;
}

Rod2Card Rod2Table::card(std::uint32_t row) const
{
    return Rod2Card{m_id.get(row), m_propId.get(row), m_nodes.get(row)};
}

void Rod2Table::store(std::uint32_t row, const Rod2Card& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodes.set(row, card.nodes);
}

void Rod2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodes.keepRows(rows);
}

void Rod2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    std::array<base::EntityId, 2> nodes = m_nodes.get(row);
    bool changed = false;
    for (std::size_t index = 0; index < 2; ++index)
    {
        const base::EntityId updated = visitor.visit("nodes", ReferenceTarget::Node, nodes[index]);
        changed = changed || updated != nodes[index];
        nodes[index] = updated;
    }
    if (changed)
        m_nodes.set(row, nodes);
}

} // namespace oor::model
