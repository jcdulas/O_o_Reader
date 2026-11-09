#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One SEMPLN card: the mirror plane of a part superelement, by three points.
//
// Card layout: SEMPLN  SEID  PLANE  P1  P2  P3                                    fields 1 to 5
// SEID is the partitioned superelement to mirror. The second field is the flag PLANE, the only one the format reference
// has: the plane is defined by three noncolinear points (written as the word PLANE, which the card always writes). P1
// to P3 are GRID or POINT cards of the main Bulk Data (the grids or points are defined in the main Bulk Data,
// not in a part). The card belongs in the main Bulk Data and is a fatal error after a BEGIN SUPER. A MIRROR
// superelement (SEBULK) needs one.
struct SuperelementMirrorPlaneCard
{
    base::EntityId superelementId = 0;
    std::array<base::EntityId, 3> points{}; // P1 to P3

    [[nodiscard]] bool operator==(const SuperelementMirrorPlaneCard&) const noexcept = default;
};

} // namespace oor::model
