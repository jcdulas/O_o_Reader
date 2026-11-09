#include "oor/model/cards/elements/springOffsetExplicit/SpringOffsetExplicitTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 4;

} // namespace

std::optional<SpringOffsetExplicitCard> SpringOffsetExplicitFormat::read(FieldReader& reader)
{
    SpringOffsetExplicitCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    card.nodes[0] = reader.requiredInteger(3, "node1");
    card.nodes[1] = reader.requiredInteger(4, "node2");
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SpringOffsetExplicitFormat::check(const SpringOffsetExplicitCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    if (card.nodes[0] <= 0)
        return CardProblem{"card.bad-id", "the grid point IDs must be positive integers", "node1"};
    if (card.nodes[1] <= 0)
        return CardProblem{"card.bad-id", "the grid point IDs must be positive integers", "node2"};
    return std::nullopt;
}

void SpringOffsetExplicitFormat::write(const SpringOffsetExplicitCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.integer(3, card.nodes[0]);
    writer.integer(4, card.nodes[1]);
}

std::uint32_t SpringOffsetExplicitTable::append(const SpringOffsetExplicitCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodes.push(card.nodes);
    return row;
}

SpringOffsetExplicitCard SpringOffsetExplicitTable::card(std::uint32_t row) const
{
    return SpringOffsetExplicitCard{m_id.get(row), m_propId.get(row), m_nodes.get(row)};
}

void SpringOffsetExplicitTable::store(std::uint32_t row, const SpringOffsetExplicitCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodes.set(row, card.nodes);
}

void SpringOffsetExplicitTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodes.keepRows(rows);
}

void SpringOffsetExplicitTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // A PSPRMAT is a property card: the reference resolves to it.
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    std::array<base::EntityId, 2> nodes = m_nodes.get(row);
    bool changed = false;
    for (base::EntityId& node : nodes)
    {
        if (node <= 0)
            continue;
        const base::EntityId updated = visitor.visit("nodes", ReferenceTarget::Node, node);
        changed = changed || updated != node;
        node = updated;
    }
    if (changed)
        m_nodes.set(row, nodes);
}

} // namespace oor::model
