#include "oor/model/cards/elements/shellTria6/ShellTria6Table.hpp"

#include <variant>

namespace oor::model
{

std::optional<ShellTria6Card> ShellTria6Format::read(FieldReader& reader)
{
    static constexpr const char* NAMES[ShellTria6Card::NODE_COUNT] = {"node1", "node2", "node3",
                                                                     "node4", "node5", "node6"};
    ShellTria6Card card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    for (std::size_t index = 0; index < ShellTria6Card::NODE_COUNT; ++index)
    {
        const int field = 3 + static_cast<int>(index);
        // The corners are required; an edge point may be blank or zero (the element then has fewer points).
        card.nodes[index] = index < ShellTria6Card::CORNER_COUNT ? reader.requiredInteger(field, NAMES[index])
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
    card.offset = reader.real(10, "offset", 0.0);
    for (int index = 0; index < 3; ++index)
    {
        static constexpr const char* THICKNESS_NAMES[3] = {"cornerThickness1", "cornerThickness2", "cornerThickness3"};
        card.cornerThickness[static_cast<std::size_t>(index)] =
            reader.real(11 + index, THICKNESS_NAMES[index], ShellTria6Card::NO_THICKNESS);
    }
    card.thicknessFlag = reader.integer(14, "thicknessFlag", 0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ShellTria6Format::check(const ShellTria6Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    for (std::size_t first = 0; first < ShellTria6Card::NODE_COUNT; ++first)
    {
        if (first < ShellTria6Card::CORNER_COUNT && card.nodes[first] <= 0)
            return CardProblem{"card.bad-id", "the corner grid points must be positive integers", "node1"};
        if (card.nodes[first] < 0)
            return CardProblem{"card.bad-id", "the grid point IDs must not be negative", "node4"};
        if (card.nodes[first] == 0)
            continue;
        for (std::size_t second = first + 1; second < ShellTria6Card::NODE_COUNT; ++second)
        {
            if (card.nodes[first] == card.nodes[second])
                return CardProblem{"card.duplicate-node", "the grid points of an element must all be different",
                                   "node1"};
        }
    }
    if (card.thicknessFlag != 0 && card.thicknessFlag != 1)
        return CardProblem{"card.bad-value", "{field} must be 0 or 1", "thicknessFlag"};
    return std::nullopt;
}

void ShellTria6Format::write(const ShellTria6Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    for (std::size_t index = 0; index < ShellTria6Card::NODE_COUNT; ++index)
    {
        // An absent edge point is a blank field.
        if (index < ShellTria6Card::CORNER_COUNT || card.nodes[index] != 0)
            writer.integer(3 + static_cast<int>(index), card.nodes[index]);
    }
    if (card.usesMaterialSystem())
        writer.integer(9, card.matFrameId);
    else
        writer.realUnlessDefault(9, card.thetaInDegrees, 0.0);
    writer.realUnlessDefault(10, card.offset, 0.0);
    for (int index = 0; index < 3; ++index)
    {
        const double thickness = card.cornerThickness[static_cast<std::size_t>(index)];
        if (thickness != ShellTria6Card::NO_THICKNESS)
            writer.real(11 + index, thickness);
    }
    writer.integerUnlessDefault(14, card.thicknessFlag, 0);
}

std::uint32_t ShellTria6Table::append(const ShellTria6Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodes.push(card.nodes);
    m_thetaInDegrees.push(card.thetaInDegrees);
    m_matFrameId.push(card.matFrameId);
    m_offset.push(card.offset);
    m_thicknessFlag.push(card.thicknessFlag);
    m_cornerThickness.push(card.cornerThickness);
    return row;
}

ShellTria6Card ShellTria6Table::card(std::uint32_t row) const
{
    return ShellTria6Card{
        m_id.get(row),         m_propId.get(row), m_nodes.get(row),         m_thetaInDegrees.get(row),
        m_matFrameId.get(row), m_offset.get(row), m_thicknessFlag.get(row), m_cornerThickness.get(row)};
}

void ShellTria6Table::store(std::uint32_t row, const ShellTria6Card& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodes.set(row, card.nodes);
    m_thetaInDegrees.set(row, card.thetaInDegrees);
    m_matFrameId.set(row, card.matFrameId);
    m_offset.set(row, card.offset);
    m_thicknessFlag.set(row, card.thicknessFlag);
    m_cornerThickness.set(row, card.cornerThickness);
}

void ShellTria6Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodes.keepRows(rows);
    m_thetaInDegrees.keepRows(rows);
    m_matFrameId.keepRows(rows);
    m_offset.keepRows(rows);
    m_thicknessFlag.keepRows(rows);
    m_cornerThickness.keepRows(rows);
}

void ShellTria6Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    std::array<base::EntityId, ShellTria6Card::NODE_COUNT> nodes = m_nodes.get(row);
    bool changed = false;
    for (std::size_t index = 0; index < ShellTria6Card::NODE_COUNT; ++index)
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
