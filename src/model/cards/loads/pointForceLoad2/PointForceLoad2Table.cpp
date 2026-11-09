#include "oor/model/cards/loads/pointForceLoad2/PointForceLoad2Table.hpp"

namespace oor::model
{

std::optional<PointForceLoad2Card> PointForceLoad2Format::read(FieldReader& reader)
{
    PointForceLoad2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.node = reader.requiredInteger(2, "node");
    card.magnitude = reader.requiredReal(3, "magnitude");
    card.node1 = reader.requiredInteger(4, "node1");
    card.node2 = reader.requiredInteger(5, "node2");
    card.node3 = reader.requiredInteger(6, "node3");
    card.node4 = reader.requiredInteger(7, "node4");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PointForceLoad2Format::check(const PointForceLoad2Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    if (card.node <= 0)
        return CardProblem{"card.bad-id", "the grid point ID must be a positive integer", "node"};
    if (card.node1 <= 0)
        return CardProblem{"card.bad-id", "the grid point ID must be a positive integer", "node1"};
    if (card.node2 <= 0)
        return CardProblem{"card.bad-id", "the grid point ID must be a positive integer", "node2"};
    if (card.node3 <= 0)
        return CardProblem{"card.bad-id", "the grid point ID must be a positive integer", "node3"};
    if (card.node4 <= 0)
        return CardProblem{"card.bad-id", "the grid point ID must be a positive integer", "node4"};
    // format reference: G1 and G2 may not be coincident, nor G3 and G4; the same point certainly is.
    if (card.node1 == card.node2)
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("node1") + " and " + std::string(CardProblem::FIELD) +
                               " define a direction and must differ",
                           "node2"};
    if (card.node3 == card.node4)
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("node3") + " and " + std::string(CardProblem::FIELD) +
                               " define a direction and must differ",
                           "node4"};
    return std::nullopt;
}

void PointForceLoad2Format::write(const PointForceLoad2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.node);
    writer.real(3, card.magnitude);
    writer.integer(4, card.node1);
    writer.integer(5, card.node2);
    writer.integer(6, card.node3);
    writer.integer(7, card.node4);
}

std::uint32_t PointForceLoad2Table::append(const PointForceLoad2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_node.push(card.node);
    m_magnitude.push(card.magnitude);
    m_node1.push(card.node1);
    m_node2.push(card.node2);
    m_node3.push(card.node3);
    m_node4.push(card.node4);
    return row;
}

PointForceLoad2Card PointForceLoad2Table::card(std::uint32_t row) const
{
    return PointForceLoad2Card{m_id.get(row),    m_node.get(row),  m_magnitude.get(row), m_node1.get(row),
                               m_node2.get(row), m_node3.get(row), m_node4.get(row)};
}

void PointForceLoad2Table::store(std::uint32_t row, const PointForceLoad2Card& card)
{
    m_id.set(row, card.id);
    m_node.set(row, card.node);
    m_magnitude.set(row, card.magnitude);
    m_node1.set(row, card.node1);
    m_node2.set(row, card.node2);
    m_node3.set(row, card.node3);
    m_node4.set(row, card.node4);
}

void PointForceLoad2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_node.keepRows(rows);
    m_magnitude.keepRows(rows);
    m_node1.keepRows(rows);
    m_node2.keepRows(rows);
    m_node3.keepRows(rows);
    m_node4.keepRows(rows);
}

void PointForceLoad2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_node.set(row, visitor.visit("node", ReferenceTarget::Node, m_node.get(row)));
    m_node1.set(row, visitor.visit("node1", ReferenceTarget::Node, m_node1.get(row)));
    m_node2.set(row, visitor.visit("node2", ReferenceTarget::Node, m_node2.get(row)));
    m_node3.set(row, visitor.visit("node3", ReferenceTarget::Node, m_node3.get(row)));
    m_node4.set(row, visitor.visit("node4", ReferenceTarget::Node, m_node4.get(row)));
}

} // namespace oor::model
