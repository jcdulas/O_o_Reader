#include "oor/model/cards/elements/shearPanel4/ShearPanel4Table.hpp"

namespace oor::model
{

std::optional<ShearPanel4Card> ShearPanel4Format::read(FieldReader& reader)
{
    ShearPanel4Card card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.integer(2, "propId", card.id); // format reference: PID defaults to EID
    for (int index = 0; index < 4; ++index)
    {
        static constexpr const char* NAMES[4] = {"node1", "node2", "node3", "node4"};
        card.nodes[static_cast<std::size_t>(index)] = reader.requiredInteger(3 + index, NAMES[index]);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ShearPanel4Format::check(const ShearPanel4Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    for (std::size_t first = 0; first < 4; ++first)
    {
        if (card.nodes[first] <= 0)
            return CardProblem{"card.bad-id", "the grid point IDs must be positive integers", "node1"};
        for (std::size_t second = first + 1; second < 4; ++second)
        {
            if (card.nodes[first] == card.nodes[second])
                return CardProblem{"card.duplicate-node", "the four grid points must all be different", "node1"};
        }
    }
    return std::nullopt;
}

void ShearPanel4Format::write(const ShearPanel4Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    for (int index = 0; index < 4; ++index)
        writer.integer(3 + index, card.nodes[static_cast<std::size_t>(index)]);
}

std::uint32_t ShearPanel4Table::append(const ShearPanel4Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodes.push(card.nodes);
    return row;
}

ShearPanel4Card ShearPanel4Table::card(std::uint32_t row) const
{
    return ShearPanel4Card{m_id.get(row), m_propId.get(row), m_nodes.get(row)};
}

void ShearPanel4Table::store(std::uint32_t row, const ShearPanel4Card& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodes.set(row, card.nodes);
}

void ShearPanel4Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodes.keepRows(rows);
}

void ShearPanel4Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    std::array<base::EntityId, 4> nodes = m_nodes.get(row);
    bool changed = false;
    for (std::size_t index = 0; index < 4; ++index)
    {
        const base::EntityId updated = visitor.visit("nodes", ReferenceTarget::Node, nodes[index]);
        changed = changed || updated != nodes[index];
        nodes[index] = updated;
    }
    if (changed)
        m_nodes.set(row, nodes);
}

} // namespace oor::model
