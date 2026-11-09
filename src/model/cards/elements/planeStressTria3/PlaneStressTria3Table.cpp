#include "oor/model/cards/elements/planeStressTria3/PlaneStressTria3Table.hpp"

namespace oor::model
{

std::optional<PlaneStressTria3Card> PlaneStressTria3Format::read(FieldReader& reader)
{
    PlaneStressTria3Card card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.integer(2, "propId", card.id); // PID defaults to EID
    for (int index = 0; index < 3; ++index)
    {
        static constexpr const char* NAMES[3] = {"node1", "node2", "node3"};
        card.nodes[static_cast<std::size_t>(index)] = reader.requiredInteger(3 + index, NAMES[index]);
    }
    card.thetaInDegrees = reader.real(6, "materialAngle", 0.0);
    card.thicknessFlag = reader.integer(10, "thicknessFlag", 0);
    for (int index = 0; index < 3; ++index)
    {
        static constexpr const char* NAMES[3] = {"thickness1", "thickness2", "thickness3"};
        card.cornerThickness[static_cast<std::size_t>(index)] =
            reader.real(11 + index, NAMES[index], PlaneStressTria3Card::NO_THICKNESS);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PlaneStressTria3Format::check(const PlaneStressTria3Card& card)
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

void PlaneStressTria3Format::write(const PlaneStressTria3Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    for (int index = 0; index < 3; ++index)
        writer.integer(3 + index, card.nodes[static_cast<std::size_t>(index)]);
    writer.realUnlessDefault(6, card.thetaInDegrees, 0.0);
    writer.integerUnlessDefault(10, card.thicknessFlag, 0);
    for (int index = 0; index < 3; ++index)
    {
        const double thickness = card.cornerThickness[static_cast<std::size_t>(index)];
        if (thickness != PlaneStressTria3Card::NO_THICKNESS)
            writer.real(11 + index, thickness);
    }
}

std::uint32_t PlaneStressTria3Table::append(const PlaneStressTria3Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodes.push(card.nodes);
    m_thetaInDegrees.push(card.thetaInDegrees);
    m_thicknessFlag.push(card.thicknessFlag);
    m_cornerThickness.push(card.cornerThickness);
    return row;
}

PlaneStressTria3Card PlaneStressTria3Table::card(std::uint32_t row) const
{
    return PlaneStressTria3Card{m_id.get(row),
                                m_propId.get(row),
                                m_nodes.get(row),
                                m_thetaInDegrees.get(row),
                                m_thicknessFlag.get(row),
                                m_cornerThickness.get(row)};
}

void PlaneStressTria3Table::store(std::uint32_t row, const PlaneStressTria3Card& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodes.set(row, card.nodes);
    m_thetaInDegrees.set(row, card.thetaInDegrees);
    m_thicknessFlag.set(row, card.thicknessFlag);
    m_cornerThickness.set(row, card.cornerThickness);
}

void PlaneStressTria3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodes.keepRows(rows);
    m_thetaInDegrees.keepRows(rows);
    m_thicknessFlag.keepRows(rows);
    m_cornerThickness.keepRows(rows);
}

void PlaneStressTria3Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
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
}

} // namespace oor::model
