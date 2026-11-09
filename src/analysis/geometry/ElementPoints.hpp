#pragma once

#include "ElementProvider.hpp"

#include <cstdint>
#include <optional>
#include <vector>

// How the nodes of an element become points: the IDs of the card are looked up in the GRID table, and the positions
// come from the cache of the basic system. The geometry and the quality of the elements both start here.

namespace oor::analysis::detail
{

// The Node row of a node ID. Looking up a million IDs one by one costs more than the work on the elements, so when
// there are many lookups relative to the grid points the IDs are put in a table indexed by ID (when the IDs are not too
// sparse).
class NodeLookup
{
public:
    NodeLookup(const model::Model& model, std::size_t expectedLookups);

    [[nodiscard]] const model::Model& model() const noexcept { return m_model; }
    // The row of the grid point with this ID in the Node table, or -1.
    [[nodiscard]] std::int32_t nodeRow(base::EntityId id) const noexcept;
    // True when a card of the node ID space has this ID (a grid point, a scalar point, ...).
    [[nodiscard]] bool exists(base::EntityId id) const noexcept;
    // The position of a slot point (an AcousticSlotSurfacePoint) in the plane of the slot cavity: x is its R, y its Z
    // and z is 0. The slot width W of the point is not a coordinate (the format reference only plots it as the basic
    // z), so the elements of a slot cavity (CSLOT3, CSLOT4) are surfaces in the R-Z plane. Nothing for any other card.
    [[nodiscard]] std::optional<base::Vec3> slotPoint(base::EntityId id) const;

private:
    const model::Model& m_model;
    std::uint16_t m_nodeType = 0;
    std::vector<std::int32_t> m_direct;
    bool m_useDirect = false;
};

// The lists a gather fills, kept between rows so that they are allocated once per thread.
struct RowScratch
{
    std::vector<base::EntityId> ids;
    std::vector<std::int32_t> nodeRows;
    std::vector<base::Vec3> points;
};

// Finds the nodes of the element in `row` (the order its provider gives them, 0 for a blank slot) and their positions
// in the basic system (NaN for a blank slot or a scalar point). Throws GeometryError naming the element when a node
// does not exist.
void gatherElementPoints(const ElementProvider& provider, const model::CardTable& table, BasicPositionCache& cache,
                         const NodeLookup& lookup, RowScratch& scratch, std::uint32_t row);

} // namespace oor::analysis::detail
