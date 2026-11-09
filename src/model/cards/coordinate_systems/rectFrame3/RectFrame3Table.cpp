#include "oor/model/cards/coordinate_systems/rectFrame3/RectFrame3Table.hpp"

#include <string>

namespace oor::model
{

namespace
{

// The system whose four fields start at `first` (1 for the first system, 5 for the second).
RectFrame3Card readSystem(FieldReader& reader, int first, const char* const (&names)[4])
{
    RectFrame3Card card;
    card.id = reader.requiredInteger(first, names[0]);
    for (int index = 0; index < 3; ++index)
        card.nodes[static_cast<std::size_t>(index)] = reader.requiredInteger(first + 1 + index, names[1 + index]);
    if (!reader.failed())
        reader.report(RectFrame3Format::check(card));
    return card;
}

} // namespace

std::vector<RectFrame3Card> RectFrame3Format::readAll(FieldReader& reader)
{
    // The format names the fields of both systems alike (CID, N1, N2, N3); the second is told apart by a B in the
    // diagnostics, as for the CORD1R.
    static constexpr const char* NAMES_A[4] = {"frameIdA", "node1A", "node2A", "node3A"};
    static constexpr const char* NAMES_B[4] = {"frameIdB", "node1B", "node2B", "node3B"};
    std::vector<RectFrame3Card> systems;
    systems.push_back(readSystem(reader, 1, NAMES_A));
    // The second system is there as soon as one of its fields is; its CID and grid points are then all required.
    if (!reader.isBlank(5) || !reader.isBlank(6) || !reader.isBlank(7) || !reader.isBlank(8))
        systems.push_back(readSystem(reader, 5, NAMES_B));
    if (reader.lastUsedField() > 8)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " defines at most two systems",
                     "node3B");
    return systems;
}

std::optional<CardProblem> RectFrame3Format::check(const RectFrame3Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the coordinate system ID must be a positive integer", "id"};
    static constexpr const char* NAMES[3] = {"node1", "node2", "node3"};
    for (std::size_t first = 0; first < 3; ++first)
    {
        if (card.nodes[first] <= 0)
            return CardProblem{"card.bad-id", "the grid point IDs must be positive integers", NAMES[first]};
        // format reference: the grid points must be unique (the origin, the z-axis point and the x-z plane point are
        // three points).
        for (std::size_t second = first + 1; second < 3; ++second)
        {
            if (card.nodes[first] == card.nodes[second])
                return CardProblem{"card.duplicate-node", "the three grid points must all be different", NAMES[second]};
        }
    }
    return std::nullopt;
}

void RectFrame3Format::write(const RectFrame3Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (int index = 0; index < 3; ++index)
        writer.integer(2 + index, card.nodes[static_cast<std::size_t>(index)]);
}

std::uint32_t RectFrame3Table::append(const RectFrame3Card& card, base::CardKey key)
{
    ++m_version;
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_nodes.push(card.nodes);
    return row;
}

RectFrame3Card RectFrame3Table::card(std::uint32_t row) const
{
    return RectFrame3Card{m_id.get(row), m_nodes.get(row)};
}

void RectFrame3Table::store(std::uint32_t row, const RectFrame3Card& card)
{
    ++m_version;
    m_id.set(row, card.id);
    m_nodes.set(row, card.nodes);
}

void RectFrame3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    ++m_version;
    m_id.keepRows(rows);
    m_nodes.keepRows(rows);
}

void RectFrame3Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
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
