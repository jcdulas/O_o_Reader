#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// The points of an EXTRN and the components of them that connect the external superelement: one point and its
// components, or a THRU range that has the same components.
struct SuperelementExternalBoundaryItem
{
    IdRun points;                // one point (first == last) or "first THRU last"
    std::int32_t components = 0; // the digits of the components as one number; 0 (blank) or 1 for a scalar point

    [[nodiscard]] bool operator==(const SuperelementExternalBoundaryItem&) const noexcept = default;
};

// One EXTRN card: the boundary connection of an external superelement.
//
// Card layout: EXTRN  GID1  C1  GID2  C2  GID3  C3  GID4  C4
//                    -etc.-
// or, in the pairs:  GID6  "THRU"  GID7  C6   (a THRU takes the place of the C of its first point, the last point comes
// in the next GID field and the components after it; it may stand in fields 3, 5 or 7 of the format reference
// numbering, never in the last pair of a line: the writer leaves that pair blank) The pairs fill the first line and the
// continuation lines four to a line, and a pair of blank fields may be left for easier editing, which the
// card does not keep. The order of the pairs is the internal order of the grid points of the creation run,
// so the card keeps it as written. EXTRN is for partitioned Bulk Data sections and is ignored in the main Bulk Data:
// the points are those of the part, which has its own ID space; the external superelement has no grid card
// for them, so the card is what says they exist (the points of an EXTRN resolve in the part).
struct SuperelementExternalBoundaryCard
{
    std::vector<SuperelementExternalBoundaryItem> items;

    [[nodiscard]] bool operator==(const SuperelementExternalBoundaryCard&) const noexcept = default;
};

} // namespace oor::model
