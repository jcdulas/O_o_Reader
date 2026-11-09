#include "oor/model/cards/loads/pointMomentLoad1/PointMomentLoad1Table.hpp"

namespace oor::model
{

std::optional<PointMomentLoad1Card> PointMomentLoad1Format::read(FieldReader& reader)
{
    PointMomentLoad1Card card;
    card.id = reader.requiredInteger(1, "id");
    card.node = reader.requiredInteger(2, "node");
    card.magnitude = reader.requiredReal(3, "magnitude");
    card.node1 = reader.requiredInteger(4, "node1");
    card.node2 = reader.requiredInteger(5, "node2");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PointMomentLoad1Format::check(const PointMomentLoad1Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    if (card.node <= 0)
        return CardProblem{"card.bad-id", "the grid point ID must be a positive integer", "node"};
    if (card.node1 <= 0)
        return CardProblem{"card.bad-id", "the grid point ID must be a positive integer", "node1"};
    if (card.node2 <= 0)
        return CardProblem{"card.bad-id", "the grid point ID must be a positive integer", "node2"};
    // format reference: G1 and G2 may not be coincident; the same point certainly is.
    if (card.node1 == card.node2)
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("node1") + " and " + std::string(CardProblem::FIELD) +
                               " define the axis of the moment and must differ",
                           "node2"};
    return std::nullopt;
}

void PointMomentLoad1Format::write(const PointMomentLoad1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.node);
    writer.real(3, card.magnitude);
    writer.integer(4, card.node1);
    writer.integer(5, card.node2);
}

std::uint32_t PointMomentLoad1Table::append(const PointMomentLoad1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_node.push(card.node);
    m_magnitude.push(card.magnitude);
    m_node1.push(card.node1);
    m_node2.push(card.node2);
    return row;
}

PointMomentLoad1Card PointMomentLoad1Table::card(std::uint32_t row) const
{
    return PointMomentLoad1Card{m_id.get(row), m_node.get(row), m_magnitude.get(row), m_node1.get(row),
                                m_node2.get(row)};
}

void PointMomentLoad1Table::store(std::uint32_t row, const PointMomentLoad1Card& card)
{
    m_id.set(row, card.id);
    m_node.set(row, card.node);
    m_magnitude.set(row, card.magnitude);
    m_node1.set(row, card.node1);
    m_node2.set(row, card.node2);
}

void PointMomentLoad1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_node.keepRows(rows);
    m_magnitude.keepRows(rows);
    m_node1.keepRows(rows);
    m_node2.keepRows(rows);
}

void PointMomentLoad1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_node.set(row, visitor.visit("node", ReferenceTarget::Node, m_node.get(row)));
    m_node1.set(row, visitor.visit("node1", ReferenceTarget::Node, m_node1.get(row)));
    m_node2.set(row, visitor.visit("node2", ReferenceTarget::Node, m_node2.get(row)));
}

} // namespace oor::model
