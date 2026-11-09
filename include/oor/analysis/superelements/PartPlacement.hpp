#pragma once

#include "oor/analysis/superelements/SuperelementCatalogue.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/base/math/Vec3.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::analysis
{

// The grid points of one superelement with their positions, by ascending ID.
struct PlacedPoints
{
    std::vector<base::EntityId> nids;
    std::vector<base::Vec3> positions; // same order as `nids`

    // The position of a point, or null when it is not one of them.
    [[nodiscard]] const base::Vec3* find(base::EntityId nid) const noexcept;
};

// Where the grid points of a part superelement are in the basic system of the residual structure (SELOC,
// SEMPLN): a part is first mirrored through the plane of its SEMPLN (three points of the main Bulk
// Data), then moved by its SELOC (the rigid move that puts its point PA1 on PB1, the direction PA1 to PA2 along PB1 to
// PB2 and the plane of PA1, PA2, PA3 on the plane of the PBi). An image (REPEAT, MIRROR) starts from the points of its
// primary as its part defines them (the primary's own SELOC is not applied to it) and keeps the primary's IDs.
//
// Stateless: every function computes from the model it is given.
class PartPlacement
{
public:
    // The placed grid points of a superelement whose points are those of a part (`entry.hasPartPoints()`). A SELOC or
    // SEMPLN that cannot be applied (a point that does not exist, three points on a line) is written to `problems` and
    // left out, the points staying where their part puts them.
    [[nodiscard]] static PlacedPoints place(const SuperelementCatalogue& catalogue, const SuperelementEntry& entry,
                                            std::vector<std::string>& problems);
    // The grid points of a model in its own basic system. Throws CoordinateSystemError when a position cannot be
    // computed.
    [[nodiscard]] static PlacedPoints gridPoints(const model::Model& model);
    // The basic position of a GRID or POINT of a model; nullopt when the ID is neither.
    [[nodiscard]] static std::optional<base::Vec3> pointPosition(const model::Model& model, base::EntityId nid);
};

} // namespace oor::analysis
