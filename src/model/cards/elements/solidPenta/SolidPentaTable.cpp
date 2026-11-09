#include "oor/model/cards/elements/solidPenta/SolidPentaTable.hpp"

namespace oor::model
{

std::optional<SolidPentaCard> SolidPentaFormat::read(FieldReader& reader)
{
    static constexpr const char* NAMES[SolidPentaCard::NODE_COUNT] = {"node1",  "node2",  "node3",  "node4",  "node5",
                                                                     "node6",  "node7",  "node8",  "node9",  "node10",
                                                                     "node11", "node12", "node13", "node14", "node15"};
    SolidPentaCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    for (std::size_t index = 0; index < SolidPentaCard::NODE_COUNT; ++index)
    {
        const int field = 3 + static_cast<int>(index);
        // The corners are required; an edge point may be blank or zero (the element then has fewer points).
        card.nodes[index] = index < SolidPentaCard::CORNER_COUNT ? reader.requiredInteger(field, NAMES[index])
                                                                : reader.integer(field, NAMES[index], 0);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SolidPentaFormat::check(const SolidPentaCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    for (std::size_t first = 0; first < SolidPentaCard::NODE_COUNT; ++first)
    {
        if (first < SolidPentaCard::CORNER_COUNT && card.nodes[first] <= 0)
            return CardProblem{"card.bad-id", "the corner grid points must be positive integers", "node1"};
        if (card.nodes[first] < 0)
            return CardProblem{"card.bad-id", "the grid point IDs must not be negative", "node5"};
        if (card.nodes[first] == 0)
            continue;
        for (std::size_t second = first + 1; second < SolidPentaCard::NODE_COUNT; ++second)
        {
            if (card.nodes[first] == card.nodes[second])
                return CardProblem{"card.duplicate-node", "the grid points of an element must all be different",
                                   "node1"};
        }
    }
    return std::nullopt;
}

void SolidPentaFormat::write(const SolidPentaCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    for (std::size_t index = 0; index < SolidPentaCard::NODE_COUNT; ++index)
    {
        // An absent edge point is a blank field.
        if (index < SolidPentaCard::CORNER_COUNT || card.nodes[index] != 0)
            writer.integer(3 + static_cast<int>(index), card.nodes[index]);
    }
}

std::uint32_t SolidPentaTable::append(const SolidPentaCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodes.push(card.nodes);
    return row;
}

SolidPentaCard SolidPentaTable::card(std::uint32_t row) const
{
    return SolidPentaCard{m_id.get(row), m_propId.get(row), m_nodes.get(row)};
}

void SolidPentaTable::store(std::uint32_t row, const SolidPentaCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodes.set(row, card.nodes);
}

void SolidPentaTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodes.keepRows(rows);
}

void SolidPentaTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    std::array<base::EntityId, SolidPentaCard::NODE_COUNT> nodes = m_nodes.get(row);
    bool changed = false;
    for (std::size_t index = 0; index < SolidPentaCard::NODE_COUNT; ++index)
    {
        if (nodes[index] <= 0)
            continue; // an absent edge point
        const base::EntityId updated = visitor.visit("nodes", ReferenceTarget::Node, nodes[index]);
        changed = changed || updated != nodes[index];
        nodes[index] = updated;
    }
    if (changed)
        m_nodes.set(row, nodes);
}

} // namespace oor::model
