#include "oor/model/cards/elements/plotLine2/PlotLine2Table.hpp"

namespace oor::model
{

namespace
{

// The element whose three fields start at `first` (1 for the first, 5 for the second).
PlotLine2Card readElement(FieldReader& reader, int first, const char* const (&names)[3])
{
    PlotLine2Card card;
    card.id = reader.requiredInteger(first, names[0]);
    card.node1 = reader.requiredInteger(first + 1, names[1]);
    card.node2 = reader.requiredInteger(first + 2, names[2]);
    if (!reader.failed())
        reader.report(PlotLine2Format::check(card));
    return card;
}

} // namespace

std::vector<PlotLine2Card> PlotLine2Format::readAll(FieldReader& reader)
{
    static constexpr const char* NAMES_FIRST[3] = {"id", "node1", "node2"};
    static constexpr const char* NAMES_SECOND[3] = {"id2", "node3", "node4"};
    std::vector<PlotLine2Card> elements;
    elements.push_back(readElement(reader, 1, NAMES_FIRST));
    // The second element is there as soon as one of its fields is (field 4 stays blank); all three are then required.
    if (!reader.isBlank(5) || !reader.isBlank(6) || !reader.isBlank(7))
        elements.push_back(readElement(reader, 5, NAMES_SECOND));
    if (!reader.isBlank(4) || reader.lastUsedField() > 7)
        reader.error(
            "card.bad-value",
            "a " + std::string(reader.cardName()) + " defines at most two elements, in fields 1 to 3 and 5 to 7",
            "node4");
    return elements;
}

std::optional<CardProblem> PlotLine2Format::check(const PlotLine2Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.node1 <= 0 || card.node2 <= 0)
        return CardProblem{"card.bad-id", "the grid point IDs must be positive integers", "node1"};
    if (card.node1 == card.node2)
        return CardProblem{"card.duplicate-node", "{field:node1} and {field} must be different", "node2"};
    return std::nullopt;
}

void PlotLine2Format::write(const PlotLine2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.node1);
    writer.integer(3, card.node2);
}

std::uint32_t PlotLine2Table::append(const PlotLine2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_node1.push(card.node1);
    m_node2.push(card.node2);
    return row;
}

PlotLine2Card PlotLine2Table::card(std::uint32_t row) const
{
    return PlotLine2Card{m_id.get(row), m_node1.get(row), m_node2.get(row)};
}

void PlotLine2Table::store(std::uint32_t row, const PlotLine2Card& card)
{
    m_id.set(row, card.id);
    m_node1.set(row, card.node1);
    m_node2.set(row, card.node2);
}

void PlotLine2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_node1.keepRows(rows);
    m_node2.keepRows(rows);
}

void PlotLine2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_node1.set(row, visitor.visit("node1", ReferenceTarget::Node, m_node1.get(row)));
    m_node2.set(row, visitor.visit("node2", ReferenceTarget::Node, m_node2.get(row)));
}

} // namespace oor::model
