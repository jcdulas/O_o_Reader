#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One BCSCAP card: whether a cap is added to a beam and how many segments model a rod or tube section, for the
// segment-to-segment beam contact (SOLs 101 and 400, and the permanently glued contact of the other solutions). Without
// it every cap is ignored.
//
// Card layout: BCSCAP  EID  IESCAP  NSEG
struct ContactBeamCapCard
{
    base::EntityId id = 0; // the beam element
    std::int32_t capKind =
        0; // 0 no cap, 1 end cap, 2 side cap, 3 averaged side cap, 4 end and side cap, 5 end and averaged side cap
    std::int32_t segmentCount = 32; // the segments of a circular section (ROD, TUBE or TUBE2): at least 3
};

} // namespace oor::model
