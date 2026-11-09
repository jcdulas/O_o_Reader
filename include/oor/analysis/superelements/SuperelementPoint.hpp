#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <compare>

namespace oor::analysis
{

// A point of a superelement: the superelement whose ID space it is named in (0 for the residual structure) and its ID
// there. A part (BEGIN SUPER) has an ID space of its own; a list superelement (SESET, the SEID of GRID) shares the one
// of the main Bulk Data; an image (REPEAT, MIRROR, CSUPER) uses the IDs of its primary under its own number (the solver's
// GPLS table of an image lists the primary's IDs).
struct SuperelementPoint
{
    base::EntityId seid = 0;
    base::EntityId nid = 0;

    [[nodiscard]] auto operator<=>(const SuperelementPoint&) const noexcept = default;
};

} // namespace oor::analysis
