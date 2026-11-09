#include "oor/model/cards/elements/planeStrainQuad4/PlaneStrainQuad4Table.hpp"

namespace oor::model
{

std::optional<PlaneStrainQuad4Card> PlaneStrainQuad4Format::read(FieldReader& reader)
{
    PlaneStrainQuad4Card card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.integer(2, "propId", card.id); // PID defaults to EID
    for (int index = 0; index < 4; ++index)
    {
        static constexpr const char* NAMES[4] = {"node1", "node2", "node3", "node4"};
        card.nodes[static_cast<std::size_t>(index)] = reader.requiredInteger(3 + index, NAMES[index]);
    }
    card.thetaInDegrees = reader.real(7, "materialAngle", 0.0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PlaneStrainQuad4Format::check(const PlaneStrainQuad4Card& card)
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

void PlaneStrainQuad4Format::write(const PlaneStrainQuad4Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    for (int index = 0; index < 4; ++index)
        writer.integer(3 + index, card.nodes[static_cast<std::size_t>(index)]);
    writer.realUnlessDefault(7, card.thetaInDegrees, 0.0);
}

std::uint32_t PlaneStrainQuad4Table::append(const PlaneStrainQuad4Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodes.push(card.nodes);
    m_thetaInDegrees.push(card.thetaInDegrees);
    return row;
}

PlaneStrainQuad4Card PlaneStrainQuad4Table::card(std::uint32_t row) const
{
    return PlaneStrainQuad4Card{m_id.get(row), m_propId.get(row), m_nodes.get(row), m_thetaInDegrees.get(row)};
}

void PlaneStrainQuad4Table::store(std::uint32_t row, const PlaneStrainQuad4Card& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodes.set(row, card.nodes);
    m_thetaInDegrees.set(row, card.thetaInDegrees);
}

void PlaneStrainQuad4Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodes.keepRows(rows);
    m_thetaInDegrees.keepRows(rows);
}

void PlaneStrainQuad4Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
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
