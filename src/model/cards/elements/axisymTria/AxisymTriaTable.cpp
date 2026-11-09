#include "oor/model/cards/elements/axisymTria/AxisymTriaTable.hpp"

#include <variant>

namespace oor::model
{

std::optional<AxisymTriaCard> AxisymTriaFormat::read(FieldReader& reader)
{
    static constexpr const char* NAMES[AxisymTriaCard::NODE_COUNT] = {"node1", "node2", "node3",
                                                                          "node4", "node5", "node6"};
    AxisymTriaCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    for (std::size_t index = 0; index < AxisymTriaCard::NODE_COUNT; ++index)
    {
        const int field = 3 + static_cast<int>(index);
        // The corners are required; an edge point may be blank or zero (the element then has fewer points).
        card.nodes[index] = index < AxisymTriaCard::CORNER_COUNT ? reader.requiredInteger(field, NAMES[index])
                                                                     : reader.integer(field, NAMES[index], 0);
    }
    // Field 9 is THETA (a real) or MCID (an integer).
    if (const auto angleOrSystem = reader.integerOrReal(9, "materialAngle/matFrameId"))
    {
        if (std::holds_alternative<std::int32_t>(*angleOrSystem))
        {
            card.matFrameId = std::get<std::int32_t>(*angleOrSystem);
            if (card.matFrameId < 0)
                reader.error("card.bad-value", std::string("matFrameId") + " must not be negative", "matFrameId");
        }
        else
        {
            card.thetaInDegrees = std::get<double>(*angleOrSystem);
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymTriaFormat::check(const AxisymTriaCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    for (std::size_t first = 0; first < AxisymTriaCard::NODE_COUNT; ++first)
    {
        if (first < AxisymTriaCard::CORNER_COUNT && card.nodes[first] <= 0)
            return CardProblem{"card.bad-id", "the corner grid points must be positive integers", "node1"};
        if (card.nodes[first] < 0)
            return CardProblem{"card.bad-id", "the grid point IDs must not be negative", "node4"};
        if (card.nodes[first] == 0)
            continue;
        for (std::size_t second = first + 1; second < AxisymTriaCard::NODE_COUNT; ++second)
        {
            if (card.nodes[first] == card.nodes[second])
                return CardProblem{"card.duplicate-node", "the grid points of an element must all be different",
                                   "node1"};
        }
    }
    return std::nullopt;
}

void AxisymTriaFormat::write(const AxisymTriaCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    for (std::size_t index = 0; index < AxisymTriaCard::NODE_COUNT; ++index)
    {
        // An absent edge point is a blank field.
        if (index < AxisymTriaCard::CORNER_COUNT || card.nodes[index] != 0)
            writer.integer(3 + static_cast<int>(index), card.nodes[index]);
    }
    if (card.usesMaterialSystem())
        writer.integer(9, card.matFrameId);
    else
        writer.realUnlessDefault(9, card.thetaInDegrees, 0.0);
}

std::uint32_t AxisymTriaTable::append(const AxisymTriaCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodes.push(card.nodes);
    m_thetaInDegrees.push(card.thetaInDegrees);
    m_matFrameId.push(card.matFrameId);
    return row;
}

AxisymTriaCard AxisymTriaTable::card(std::uint32_t row) const
{
    return AxisymTriaCard{m_id.get(row), m_propId.get(row), m_nodes.get(row), m_thetaInDegrees.get(row),
                              m_matFrameId.get(row)};
}

void AxisymTriaTable::store(std::uint32_t row, const AxisymTriaCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodes.set(row, card.nodes);
    m_thetaInDegrees.set(row, card.thetaInDegrees);
    m_matFrameId.set(row, card.matFrameId);
}

void AxisymTriaTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodes.keepRows(rows);
    m_thetaInDegrees.keepRows(rows);
    m_matFrameId.keepRows(rows);
}

void AxisymTriaTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    std::array<base::EntityId, AxisymTriaCard::NODE_COUNT> nodes = m_nodes.get(row);
    bool changed = false;
    for (std::size_t index = 0; index < AxisymTriaCard::NODE_COUNT; ++index)
    {
        if (nodes[index] <= 0)
            continue; // an absent edge point
        const base::EntityId updated = visitor.visit("nodes", ReferenceTarget::Node, nodes[index]);
        changed = changed || updated != nodes[index];
        nodes[index] = updated;
    }
    if (changed)
        m_nodes.set(row, nodes);
    if (const base::EntityId matFrameId = m_matFrameId.get(row); matFrameId > 0)
        m_matFrameId.set(row, visitor.visit("matFrameId", ReferenceTarget::Coordinate, matFrameId));
}

} // namespace oor::model
