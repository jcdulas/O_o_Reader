#include "oor/model/cards/elements/viscousDamper2/ViscousDamper2Table.hpp"

namespace oor::model
{

std::optional<ViscousDamper2Card> ViscousDamper2Format::read(FieldReader& reader)
{
    ViscousDamper2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.integer(2, "propId", card.id); // format reference: PID defaults to EID
    card.node1 = reader.requiredInteger(3, "node1");
    card.node2 = reader.requiredInteger(4, "node2");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ViscousDamper2Format::check(const ViscousDamper2Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    if (card.node1 <= 0 || card.node2 <= 0)
        return CardProblem{"card.bad-id", "the grid point IDs must be positive integers", "node1"};
    if (card.node1 == card.node2)
        return CardProblem{"card.duplicate-node", "{field:node1} and {field} must be different", "node2"};
    return std::nullopt;
}

void ViscousDamper2Format::write(const ViscousDamper2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.integer(3, card.node1);
    writer.integer(4, card.node2);
}

std::uint32_t ViscousDamper2Table::append(const ViscousDamper2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_node1.push(card.node1);
    m_node2.push(card.node2);
    return row;
}

ViscousDamper2Card ViscousDamper2Table::card(std::uint32_t row) const
{
    return ViscousDamper2Card{m_id.get(row), m_propId.get(row), m_node1.get(row), m_node2.get(row)};
}

void ViscousDamper2Table::store(std::uint32_t row, const ViscousDamper2Card& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_node1.set(row, card.node1);
    m_node2.set(row, card.node2);
}

void ViscousDamper2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_node1.keepRows(rows);
    m_node2.keepRows(rows);
}

void ViscousDamper2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    m_node1.set(row, visitor.visit("node1", ReferenceTarget::Node, m_node1.get(row)));
    m_node2.set(row, visitor.visit("node2", ReferenceTarget::Node, m_node2.get(row)));
}

} // namespace oor::model
