#include "oor/model/cards/elements/axisymTria6/AxisymTria6Table.hpp"

namespace oor::model
{

std::optional<AxisymTria6Card> AxisymTria6Format::read(FieldReader& reader)
{
    static constexpr const char* NAMES[AxisymTria6Card::NODE_COUNT] = {"node1", "node2", "node3",
                                                                           "node4", "node5", "node6"};
    AxisymTria6Card card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    for (std::size_t index = 0; index < AxisymTria6Card::NODE_COUNT; ++index)
    {
        const int field = 3 + static_cast<int>(index);
        // G1, G3 and G5 are required; G2, G4 and G6 may be blank or zero (the element then has fewer points).
        card.nodes[index] = AxisymTria6Card::isCorner(index) ? reader.requiredInteger(field, NAMES[index])
                                                                 : reader.integer(field, NAMES[index], 0);
    }
    card.thetaInDegrees = reader.real(9, "materialAngle", 0.0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymTria6Format::check(const AxisymTria6Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.matId <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "matId"};
    for (std::size_t first = 0; first < AxisymTria6Card::NODE_COUNT; ++first)
    {
        if (AxisymTria6Card::isCorner(first) && card.nodes[first] <= 0)
            return CardProblem{
                "card.bad-id",
                "the corner grid points {field}, {field:node3} and {field:node5} must be positive integers", "node1"};
        if (card.nodes[first] < 0)
            return CardProblem{"card.bad-id", "the grid point IDs must not be negative", "node2"};
        if (card.nodes[first] == 0)
            continue;
        for (std::size_t second = first + 1; second < AxisymTria6Card::NODE_COUNT; ++second)
        {
            if (card.nodes[first] == card.nodes[second])
                return CardProblem{"card.duplicate-node", "the grid points of an element must all be different",
                                   "node1"};
        }
    }
    return std::nullopt;
}

void AxisymTria6Format::write(const AxisymTria6Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    for (std::size_t index = 0; index < AxisymTria6Card::NODE_COUNT; ++index)
    {
        // An absent edge point is a blank field.
        if (AxisymTria6Card::isCorner(index) || card.nodes[index] != 0)
            writer.integer(3 + static_cast<int>(index), card.nodes[index]);
    }
    writer.realUnlessDefault(9, card.thetaInDegrees, 0.0);
}

std::uint32_t AxisymTria6Table::append(const AxisymTria6Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_nodes.push(card.nodes);
    m_thetaInDegrees.push(card.thetaInDegrees);
    return row;
}

AxisymTria6Card AxisymTria6Table::card(std::uint32_t row) const
{
    return AxisymTria6Card{m_id.get(row), m_matId.get(row), m_nodes.get(row), m_thetaInDegrees.get(row)};
}

void AxisymTria6Table::store(std::uint32_t row, const AxisymTria6Card& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_nodes.set(row, card.nodes);
    m_thetaInDegrees.set(row, card.thetaInDegrees);
}

void AxisymTria6Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_nodes.keepRows(rows);
    m_thetaInDegrees.keepRows(rows);
}

void AxisymTria6Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
    std::array<base::EntityId, AxisymTria6Card::NODE_COUNT> nodes = m_nodes.get(row);
    bool changed = false;
    for (std::size_t index = 0; index < AxisymTria6Card::NODE_COUNT; ++index)
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
