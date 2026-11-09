#include "oor/model/cards/elements/shellTria3Drilling/ShellTria3DrillingTable.hpp"

#include <variant>

namespace oor::model
{

std::optional<ShellTria3DrillingCard> ShellTria3DrillingFormat::read(FieldReader& reader)
{
    ShellTria3DrillingCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.integer(2, "propId", card.id); // format reference: PID defaults to EID
    for (int index = 0; index < 3; ++index)
    {
        static constexpr const char* NAMES[3] = {"node1", "node2", "node3"};
        card.nodes[static_cast<std::size_t>(index)] = reader.requiredInteger(3 + index, NAMES[index]);
    }
    // Field 6 is THETA (a real) or MCID (an integer).
    if (const auto angleOrSystem = reader.integerOrReal(6, "materialAngle/matFrameId"))
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
    card.offset = reader.real(7, "offset", 0.0);
    card.thicknessFlag = reader.integer(10, "thicknessFlag", 0);
    for (int index = 0; index < 3; ++index)
    {
        static constexpr const char* NAMES[3] = {"cornerThickness1", "cornerThickness2", "cornerThickness3"};
        card.cornerThickness[static_cast<std::size_t>(index)] =
            reader.real(11 + index, NAMES[index], ShellTria3DrillingCard::NO_THICKNESS);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ShellTria3DrillingFormat::check(const ShellTria3DrillingCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    for (std::size_t first = 0; first < 3; ++first)
    {
        if (card.nodes[first] <= 0)
            return CardProblem{"card.bad-id", "the grid point IDs must be positive integers", "node1"};
        for (std::size_t second = first + 1; second < 3; ++second)
        {
            if (card.nodes[first] == card.nodes[second])
                return CardProblem{"card.duplicate-node", "the three grid points must all be different", "node1"};
        }
    }
    if (card.thicknessFlag != 0 && card.thicknessFlag != 1)
        return CardProblem{"card.bad-value", "{field} must be 0 or 1", "thicknessFlag"};
    return std::nullopt;
}

void ShellTria3DrillingFormat::write(const ShellTria3DrillingCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    for (int index = 0; index < 3; ++index)
        writer.integer(3 + index, card.nodes[static_cast<std::size_t>(index)]);
    if (card.usesMaterialSystem())
        writer.integer(6, card.matFrameId);
    else
        writer.realUnlessDefault(6, card.thetaInDegrees, 0.0);
    writer.realUnlessDefault(7, card.offset, 0.0);
    writer.integerUnlessDefault(10, card.thicknessFlag, 0);
    for (int index = 0; index < 3; ++index)
    {
        const double thickness = card.cornerThickness[static_cast<std::size_t>(index)];
        if (thickness != ShellTria3DrillingCard::NO_THICKNESS)
            writer.real(11 + index, thickness);
    }
}

std::uint32_t ShellTria3DrillingTable::append(const ShellTria3DrillingCard& card, base::CardKey key)
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

ShellTria3DrillingCard ShellTria3DrillingTable::card(std::uint32_t row) const
{
    return ShellTria3DrillingCard{
        m_id.get(row),         m_propId.get(row), m_nodes.get(row),         m_thetaInDegrees.get(row),
        m_matFrameId.get(row), m_offset.get(row), m_thicknessFlag.get(row), m_cornerThickness.get(row)};
}

void ShellTria3DrillingTable::store(std::uint32_t row, const ShellTria3DrillingCard& card)
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

void ShellTria3DrillingTable::keepRows(const std::vector<std::uint32_t>& rows)
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

void ShellTria3DrillingTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    std::array<base::EntityId, 3> nodes = m_nodes.get(row);
    bool changed = false;
    for (std::size_t index = 0; index < 3; ++index)
    {
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
