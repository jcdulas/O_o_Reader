#pragma once

#include "oor/analysis/superelements/SuperelementPoint.hpp"

#include <cstdint>
#include <string_view>

namespace oor::analysis
{

// What made a point a boundary point (the `source` column of `part_boundaries`):
//   Automatic                     the location search of a part (SuperelementBulkOptions METHOD = AUTO): it coincides
//                                 with a point of the residual structure or of another part
//   SuperelementConnection        a pair of a SuperelementConnection (METHOD = MANUAL, or a pair added to the search)
//   SuperelementBoundarySearch    the location search, among the points a SuperelementBoundarySearch restricts it to
//   SuperelementExteriorPoints    a point a SuperelementExteriorPoints makes exterior to a list superelement
//   SuperelementInteriorNodes     an exterior point of a list superelement: a point of its elements that is not
//                                 interior to it (interior by a SuperelementInteriorNodes or by the SEID of the Node)
//   SuperelementExternalBoundary  a point a SuperelementExternalBoundary lists for an external superelement that
//                                 nothing joins
//   SuperelementCopyPoints        a point of a secondary superelement (SuperelementCopyPoints) paired with an exterior
//                                 point of its primary
enum class BoundarySource : std::uint8_t
{
    Automatic,
    SuperelementConnection,
    SuperelementBoundarySearch,
    SuperelementExteriorPoints,
    SuperelementInteriorNodes,
    SuperelementExternalBoundary,
    SuperelementCopyPoints
};

// "automatic", or the card type that made the point a boundary point.
[[nodiscard]] std::string_view boundarySourceName(BoundarySource source) noexcept;

// Two points of two superelements that are one boundary point: `upstream` is a point of the superelement whose
// boundary it is (SEIDA of a SuperelementConnection), `joined` the point it is joined to (in the residual structure, in
// another part, or in the downstream superelement). Links chain: the points linked directly or through others make one
// boundary point (BoundaryPointNumbering).
struct BoundaryLink
{
    SuperelementPoint upstream;
    SuperelementPoint joined;
    BoundarySource source = BoundarySource::Automatic;
    // False when the two points are joined wherever they are: a SuperelementConnection with LOC = NO, and the links
    // that have no location (a list superelement's exterior point, which is the residual point itself; a
    // SuperelementCopyPoints pair).
    bool hasLocation = true;
};

} // namespace oor::analysis
