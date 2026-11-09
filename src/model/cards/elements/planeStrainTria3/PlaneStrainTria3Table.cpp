#include "oor/model/cards/elements/planeStrainTria3/PlaneStrainTria3Table.hpp"

namespace oor::model
{

std::optional<PlaneStrainTria3Card> PlaneStrainTria3Format::read(FieldReader& reader)
{
    PlaneStrainTria3Card card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.integer(2, "propId", card.id); // PID defaults to EID
    for (int index = 0; index < 3; ++index)
    {
        static constexpr const char* NAMES[3] = {"node1", "node2", "node3"};
        card.nodes[static_cast<std::size_t>(index)] = reader.requiredInteger(3 + index, NAMES[index]);
    }
    card.thetaInDegrees = reader.real(6, "materialAngle", 0.0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PlaneStrainTria3Format::check(const PlaneStrainTria3Card& card)
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
    return std::nullopt;
}

void PlaneStrainTria3Format::write(const PlaneStrainTria3Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    for (int index = 0; index < 3; ++index)
        writer.integer(3 + index, card.nodes[static_cast<std::size_t>(index)]);
    writer.realUnlessDefault(6, card.thetaInDegrees, 0.0);
}

std::uint32_t PlaneStrainTria3Table::append(const PlaneStrainTria3Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodes.push(card.nodes);
    m_thetaInDegrees.push(card.thetaInDegrees);
    return row;
}

PlaneStrainTria3Card PlaneStrainTria3Table::card(std::uint32_t row) const
{
    return PlaneStrainTria3Card{m_id.get(row), m_propId.get(row), m_nodes.get(row), m_thetaInDegrees.get(row)};
}

void PlaneStrainTria3Table::store(std::uint32_t row, const PlaneStrainTria3Card& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodes.set(row, card.nodes);
    m_thetaInDegrees.set(row, card.thetaInDegrees);
}

void PlaneStrainTria3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodes.keepRows(rows);
    m_thetaInDegrees.keepRows(rows);
}

void PlaneStrainTria3Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
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
