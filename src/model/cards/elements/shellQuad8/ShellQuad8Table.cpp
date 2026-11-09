#include "oor/model/cards/elements/shellQuad8/ShellQuad8Table.hpp"

#include <variant>

namespace oor::model
{

std::optional<ShellQuad8Card> ShellQuad8Format::read(FieldReader& reader)
{
    static constexpr const char* NAMES[ShellQuad8Card::NODE_COUNT] = {"node1", "node2", "node3", "node4",
                                                                     "node5", "node6", "node7", "node8"};
    ShellQuad8Card card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    for (std::size_t index = 0; index < ShellQuad8Card::NODE_COUNT; ++index)
    {
        const int field = 3 + static_cast<int>(index);
        // The corners are required; an edge point may be blank or zero (the element then has fewer points).
        card.nodes[index] = index < ShellQuad8Card::CORNER_COUNT ? reader.requiredInteger(field, NAMES[index])
                                                                : reader.integer(field, NAMES[index], 0);
    }
    // Fields 11 to 14 are the corner thicknesses, field 15 is THETA (a real) or MCID (an integer).
    for (int index = 0; index < 4; ++index)
    {
        static constexpr const char* THICKNESS_NAMES[4] = {"cornerThickness1", "cornerThickness2", "cornerThickness3",
                                                           "cornerThickness4"};
        card.cornerThickness[static_cast<std::size_t>(index)] =
            reader.real(11 + index, THICKNESS_NAMES[index], ShellQuad8Card::NO_THICKNESS);
    }
    if (const auto angleOrSystem = reader.integerOrReal(15, "materialAngle/matFrameId"))
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
    card.offset = reader.real(16, "offset", 0.0);
    card.thicknessFlag = reader.integer(17, "thicknessFlag", 0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ShellQuad8Format::check(const ShellQuad8Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    for (std::size_t first = 0; first < ShellQuad8Card::NODE_COUNT; ++first)
    {
        if (first < ShellQuad8Card::CORNER_COUNT && card.nodes[first] <= 0)
            return CardProblem{"card.bad-id", "the corner grid points must be positive integers", "node1"};
        if (card.nodes[first] < 0)
            return CardProblem{"card.bad-id", "the grid point IDs must not be negative", "node5"};
        if (card.nodes[first] == 0)
            continue;
        for (std::size_t second = first + 1; second < ShellQuad8Card::NODE_COUNT; ++second)
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

void ShellQuad8Format::write(const ShellQuad8Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    for (std::size_t index = 0; index < ShellQuad8Card::NODE_COUNT; ++index)
    {
        // An absent edge point is a blank field.
        if (index < ShellQuad8Card::CORNER_COUNT || card.nodes[index] != 0)
            writer.integer(3 + static_cast<int>(index), card.nodes[index]);
    }
    for (int index = 0; index < 4; ++index)
    {
        const double thickness = card.cornerThickness[static_cast<std::size_t>(index)];
        if (thickness != ShellQuad8Card::NO_THICKNESS)
            writer.real(11 + index, thickness);
    }
    if (card.usesMaterialSystem())
        writer.integer(15, card.matFrameId);
    else
        writer.realUnlessDefault(15, card.thetaInDegrees, 0.0);
    writer.realUnlessDefault(16, card.offset, 0.0);
    writer.integerUnlessDefault(17, card.thicknessFlag, 0);
}

std::uint32_t ShellQuad8Table::append(const ShellQuad8Card& card, base::CardKey key)
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

ShellQuad8Card ShellQuad8Table::card(std::uint32_t row) const
{
    return ShellQuad8Card{
        m_id.get(row),         m_propId.get(row), m_nodes.get(row),         m_thetaInDegrees.get(row),
        m_matFrameId.get(row), m_offset.get(row), m_thicknessFlag.get(row), m_cornerThickness.get(row)};
}

void ShellQuad8Table::store(std::uint32_t row, const ShellQuad8Card& card)
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

void ShellQuad8Table::keepRows(const std::vector<std::uint32_t>& rows)
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

void ShellQuad8Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    std::array<base::EntityId, ShellQuad8Card::NODE_COUNT> nodes = m_nodes.get(row);
    bool changed = false;
    for (std::size_t index = 0; index < ShellQuad8Card::NODE_COUNT; ++index)
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
