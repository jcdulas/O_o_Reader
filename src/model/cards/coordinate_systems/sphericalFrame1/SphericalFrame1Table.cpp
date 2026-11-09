#include "oor/model/cards/coordinate_systems/sphericalFrame1/SphericalFrame1Table.hpp"

namespace oor::model
{

namespace
{

// The system whose four fields start at `first` (1 for system A, 5 for system B).
SphericalFrame1Card readSystem(FieldReader& reader, int first, const char* const (&names)[4])
{
    SphericalFrame1Card card;
    card.id = reader.requiredInteger(first, names[0]);
    for (int index = 0; index < 3; ++index)
        card.nodes[static_cast<std::size_t>(index)] = reader.requiredInteger(first + 1 + index, names[1 + index]);
    if (!reader.failed())
        reader.report(SphericalFrame1Format::check(card));
    return card;
}

} // namespace

std::vector<SphericalFrame1Card> SphericalFrame1Format::readAll(FieldReader& reader)
{
    static constexpr const char* NAMES_A[4] = {"frameIdA", "node1A", "node2A", "node3A"};
    static constexpr const char* NAMES_B[4] = {"frameIdB", "node1B", "node2B", "node3B"};
    std::vector<SphericalFrame1Card> systems;
    systems.push_back(readSystem(reader, 1, NAMES_A));
    // The second system is there as soon as one of its fields is; its CIDB and grid points are then all required.
    if (!reader.isBlank(5) || !reader.isBlank(6) || !reader.isBlank(7) || !reader.isBlank(8))
        systems.push_back(readSystem(reader, 5, NAMES_B));
    return systems;
}

std::optional<CardProblem> SphericalFrame1Format::check(const SphericalFrame1Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the coordinate system ID must be a positive integer", "id"};
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

void SphericalFrame1Format::write(const SphericalFrame1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (int index = 0; index < 3; ++index)
        writer.integer(2 + index, card.nodes[static_cast<std::size_t>(index)]);
}

std::uint32_t SphericalFrame1Table::append(const SphericalFrame1Card& card, base::CardKey key)
{
    ++m_version;
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_nodes.push(card.nodes);
    return row;
}

SphericalFrame1Card SphericalFrame1Table::card(std::uint32_t row) const
{
    return SphericalFrame1Card{m_id.get(row), m_nodes.get(row)};
}

void SphericalFrame1Table::store(std::uint32_t row, const SphericalFrame1Card& card)
{
    ++m_version;
    m_id.set(row, card.id);
    m_nodes.set(row, card.nodes);
}

void SphericalFrame1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    ++m_version;
    m_id.keepRows(rows);
    m_nodes.keepRows(rows);
}

void SphericalFrame1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ++m_version;
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
