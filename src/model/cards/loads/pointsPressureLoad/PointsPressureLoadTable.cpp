#include "oor/model/cards/loads/pointsPressureLoad/PointsPressureLoadTable.hpp"

namespace oor::model
{

std::optional<PointsPressureLoadCard> PointsPressureLoadFormat::read(FieldReader& reader)
{
    PointsPressureLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.pressure = reader.requiredReal(2, "pressure");
    card.nodes[0] = reader.requiredInteger(3, "node1");
    card.nodes[1] = reader.requiredInteger(4, "node2");
    card.nodes[2] = reader.requiredInteger(5, "node3");
    card.nodes[3] = reader.integer(6, "node4", 0); // format reference: G4 may be zero or blank
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PointsPressureLoadFormat::check(const PointsPressureLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    static constexpr const char* NAMES[4] = {"node1", "node2", "node3", "node4"};
    for (std::size_t index = 0; index < 4; ++index)
    {
        const bool optional = index == 3;
        if (card.nodes[index] < 0 || (card.nodes[index] == 0 && !optional))
            return CardProblem{"card.bad-id", "the grid point ID must be a positive integer", NAMES[index]};
        // A grid point cannot appear twice on the surface.
        for (std::size_t other = 0; other < index; ++other)
        {
            if (card.nodes[index] != 0 && card.nodes[index] == card.nodes[other])
                return CardProblem{"card.bad-value", "the grid points of the surface must be different", NAMES[index]};
        }
    }
    return std::nullopt;
}

void PointsPressureLoadFormat::write(const PointsPressureLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.pressure);
    writer.integer(3, card.nodes[0]);
    writer.integer(4, card.nodes[1]);
    writer.integer(5, card.nodes[2]);
    writer.integerUnlessDefault(6, card.nodes[3], 0);
}

std::uint32_t PointsPressureLoadTable::append(const PointsPressureLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_pressure.push(card.pressure);
    m_nodes.push(card.nodes);
    return row;
}

PointsPressureLoadCard PointsPressureLoadTable::card(std::uint32_t row) const
{
    return PointsPressureLoadCard{m_id.get(row), m_pressure.get(row), m_nodes.get(row)};
}

void PointsPressureLoadTable::store(std::uint32_t row, const PointsPressureLoadCard& card)
{
    m_id.set(row, card.id);
    m_pressure.set(row, card.pressure);
    m_nodes.set(row, card.nodes);
}

void PointsPressureLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_pressure.keepRows(rows);
    m_nodes.keepRows(rows);
}

void PointsPressureLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    std::array<base::EntityId, 4> nodes = m_nodes.get(row);
    static constexpr const char* NAMES[4] = {"node1", "node2", "node3", "node4"};
    bool changed = false;
    for (std::size_t index = 0; index < 4; ++index)
    {
        if (nodes[index] == 0)
            continue;
        const base::EntityId updated = visitor.visit(NAMES[index], ReferenceTarget::Node, nodes[index]);
        changed = changed || updated != nodes[index];
        nodes[index] = updated;
    }
    if (changed)
        m_nodes.set(row, nodes);
}

} // namespace oor::model
