#include "oor/model/cards/elements/plotQuad4/PlotQuad4Table.hpp"

namespace oor::model
{

std::optional<PlotQuad4Card> PlotQuad4Format::read(FieldReader& reader)
{
    PlotQuad4Card card;
    card.id = reader.requiredInteger(1, "id");
    card.node1 = reader.requiredInteger(2, "node1");
    card.node2 = reader.requiredInteger(3, "node2");
    card.node3 = reader.requiredInteger(4, "node3");
    card.node4 = reader.requiredInteger(5, "node4");
    // The layout of the decks has no second element and nothing after G4: what is not known is refused.
    if (reader.lastUsedField() > 5)
        reader.error("card.bad-value",
                     "a " + std::string(reader.cardName()) + " has five fields, " + std::string("id") + " and " +
                         std::string("node1") + " to " + std::string("node4"),
                     "node4");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PlotQuad4Format::check(const PlotQuad4Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.node1 <= 0 || card.node2 <= 0 || card.node3 <= 0 || card.node4 <= 0)
        return CardProblem{"card.bad-id", "the grid point IDs must be positive integers", "node1"};
    // Four different points: a quadrilateral with a repeated corner is a triangle, which is a PLOTEL3.
    if (card.node1 == card.node2)
        return CardProblem{"card.duplicate-node",
                           "the four grid points must be different: {field} repeats {field:node1}", "node2"};
    if (card.node3 == card.node1 || card.node3 == card.node2)
        return CardProblem{"card.duplicate-node", "the four grid points must be different: {field} repeats another one",
                           "node3"};
    if (card.node4 == card.node1 || card.node4 == card.node2 || card.node4 == card.node3)
        return CardProblem{"card.duplicate-node", "the four grid points must be different: {field} repeats another one",
                           "node4"};
    return std::nullopt;
}

void PlotQuad4Format::write(const PlotQuad4Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.node1);
    writer.integer(3, card.node2);
    writer.integer(4, card.node3);
    writer.integer(5, card.node4);
}

std::uint32_t PlotQuad4Table::append(const PlotQuad4Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_node1.push(card.node1);
    m_node2.push(card.node2);
    m_node3.push(card.node3);
    m_node4.push(card.node4);
    return row;
}

PlotQuad4Card PlotQuad4Table::card(std::uint32_t row) const
{
    return PlotQuad4Card{m_id.get(row), m_node1.get(row), m_node2.get(row), m_node3.get(row), m_node4.get(row)};
}

void PlotQuad4Table::store(std::uint32_t row, const PlotQuad4Card& card)
{
    m_id.set(row, card.id);
    m_node1.set(row, card.node1);
    m_node2.set(row, card.node2);
    m_node3.set(row, card.node3);
    m_node4.set(row, card.node4);
}

void PlotQuad4Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_node1.keepRows(rows);
    m_node2.keepRows(rows);
    m_node3.keepRows(rows);
    m_node4.keepRows(rows);
}

void PlotQuad4Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_node1.set(row, visitor.visit("node1", ReferenceTarget::Node, m_node1.get(row)));
    m_node2.set(row, visitor.visit("node2", ReferenceTarget::Node, m_node2.get(row)));
    m_node3.set(row, visitor.visit("node3", ReferenceTarget::Node, m_node3.get(row)));
    m_node4.set(row, visitor.visit("node4", ReferenceTarget::Node, m_node4.get(row)));
}

} // namespace oor::model
