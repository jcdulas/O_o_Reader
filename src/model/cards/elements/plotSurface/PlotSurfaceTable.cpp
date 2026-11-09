#include "oor/model/cards/elements/plotSurface/PlotSurfaceTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int FIRST_GRID_FIELD = 2;
constexpr int LAST_FIELD = FIRST_GRID_FIELD + static_cast<int>(PlotSurfaceCard::MAX_GRIDS) - 1;

// The names of the grid points for the diagnostics, in the order of the card.
constexpr const char* GRID_NAMES[PlotSurfaceCard::MAX_GRIDS] = {"nodeId1", "nodeId2", "nodeId3", "nodeId4",
                                                                    "nodeId5", "nodeId6", "nodeId7", "nodeId8"};

bool hasValidGridCount(std::size_t count) noexcept
{
    return count == 3 || count == 4 || count == 6 || count == 8;
}

} // namespace

std::optional<PlotSurfaceCard> PlotSurfaceFormat::read(FieldReader& reader)
{
    PlotSurfaceCard card;
    card.id = reader.requiredInteger(1, "id");
    for (std::size_t index = 0; index < PlotSurfaceCard::MAX_GRIDS; ++index)
        card.grids[index] = reader.integer(FIRST_GRID_FIELD + static_cast<int>(index), GRID_NAMES[index], 0);
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PlotSurfaceFormat::check(const PlotSurfaceCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    for (std::size_t index = 0; index < PlotSurfaceCard::MAX_GRIDS; ++index)
    {
        if (card.grids[index] < 0)
            return CardProblem{"card.bad-id", "a grid point ID must be a positive integer", GRID_NAMES[index]};
    }
    // The points are a run from the first one: a positive ID after a blank is a hole in the list, reported on the
    // blank.
    const std::size_t count = card.gridCount();
    for (std::size_t index = count + 1; index < PlotSurfaceCard::MAX_GRIDS; ++index)
    {
        if (card.grids[index] != 0)
            return CardProblem{"card.bad-value", "the grid points of the card are listed with no blank between them",
                               GRID_NAMES[count]};
    }
    if (!hasValidGridCount(count))
        return CardProblem{"card.bad-value", "the card has 3, 4, 6 or 8 grid points, found " + std::to_string(count),
                           "nodeId1"};
    for (std::size_t first = 0; first < count; ++first)
    {
        for (std::size_t second = first + 1; second < count; ++second)
        {
            if (card.grids[first] == card.grids[second])
                return CardProblem{"card.duplicate-node", "the grid points of an element must all be different",
                                   GRID_NAMES[second]};
        }
    }
    return std::nullopt;
}

void PlotSurfaceFormat::write(const PlotSurfaceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    const std::size_t count = card.gridCount();
    for (std::size_t index = 0; index < count; ++index)
        writer.integer(FIRST_GRID_FIELD + static_cast<int>(index), card.grids[index]);
}

std::uint32_t PlotSurfaceTable::append(const PlotSurfaceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_grids.push(card.grids);
    return row;
}

PlotSurfaceCard PlotSurfaceTable::card(std::uint32_t row) const
{
    PlotSurfaceCard card;
    card.id = m_id.get(row);
    card.grids = m_grids.get(row);
    return card;
}

void PlotSurfaceTable::store(std::uint32_t row, const PlotSurfaceCard& card)
{
    m_id.set(row, card.id);
    m_grids.set(row, card.grids);
}

void PlotSurfaceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_grids.keepRows(rows);
}

void PlotSurfaceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    std::array<base::EntityId, PlotSurfaceCard::MAX_GRIDS> grids = m_grids.get(row);
    bool changed = false;
    for (base::EntityId& grid : grids)
    {
        if (grid <= 0)
            continue;
        const base::EntityId updated = visitor.visit("nodeId1", ReferenceTarget::Node, grid);
        changed = changed || updated != grid;
        grid = updated;
    }
    if (changed)
        m_grids.set(row, grids);
}

} // namespace oor::model
