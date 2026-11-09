#include "oor/model/cards/elements/plotTria3/PlotTria3Table.hpp"

namespace oor::model
{

std::optional<PlotTria3Card> PlotTria3Format::read(FieldReader& reader)
{
    PlotTria3Card card;
    card.id = reader.requiredInteger(1, "id");
    card.node1 = reader.requiredInteger(2, "node1");
    card.node2 = reader.requiredInteger(3, "node2");
    card.node3 = reader.requiredInteger(4, "node3");
    // The layout of the decks has no second element and nothing after G3: what is not known is refused.
    if (reader.lastUsedField() > 4)
        reader.error("card.bad-value",
                     "a " + std::string(reader.cardName()) + " has four fields, " + std::string("id") + ", " +
                         std::string("node1") + ", " + std::string("node2") + " and " + std::string("node3"),
                     "node3");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PlotTria3Format::check(const PlotTria3Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.node1 <= 0 || card.node2 <= 0 || card.node3 <= 0)
        return CardProblem{"card.bad-id", "the grid point IDs must be positive integers", "node1"};
    if (card.node1 == card.node2)
        return CardProblem{"card.duplicate-node",
                           "the three grid points must be different: {field:node1} and {field} are the same", "node2"};
    if (card.node1 == card.node3 || card.node2 == card.node3)
        return CardProblem{"card.duplicate-node",
                           "the three grid points must be different: {field} repeats another one", "node3"};
    return std::nullopt;
}

void PlotTria3Format::write(const PlotTria3Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.node1);
    writer.integer(3, card.node2);
    writer.integer(4, card.node3);
}

std::uint32_t PlotTria3Table::append(const PlotTria3Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_node1.push(card.node1);
    m_node2.push(card.node2);
    m_node3.push(card.node3);
    return row;
}

PlotTria3Card PlotTria3Table::card(std::uint32_t row) const
{
    return PlotTria3Card{m_id.get(row), m_node1.get(row), m_node2.get(row), m_node3.get(row)};
}

void PlotTria3Table::store(std::uint32_t row, const PlotTria3Card& card)
{
    m_id.set(row, card.id);
    m_node1.set(row, card.node1);
    m_node2.set(row, card.node2);
    m_node3.set(row, card.node3);
}

void PlotTria3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_node1.keepRows(rows);
    m_node2.keepRows(rows);
    m_node3.keepRows(rows);
}

void PlotTria3Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_node1.set(row, visitor.visit("node1", ReferenceTarget::Node, m_node1.get(row)));
    m_node2.set(row, visitor.visit("node2", ReferenceTarget::Node, m_node2.get(row)));
    m_node3.set(row, visitor.visit("node3", ReferenceTarget::Node, m_node3.get(row)));
}

} // namespace oor::model
