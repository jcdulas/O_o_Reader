#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <vector>

namespace oor::model
{

// One BCPFLG card: the branches and segments of a beam section that take part in the segment-to-segment beam contact
// (SOLs 101 and 400, and the permanently glued contact of the other solutions). Without it every segment is generated.
//
// Card layout: BCPFLG  PID  IBRNCH1  IOUTIN1  IBRNCH2  IOUTIN2  IBRNCH3  IOUTIN3  / IBRNCH4  IOUTIN4 ...
// (the pairs go on in the continuation lines). IBRNCHi picks a branch of a thin-wall section (0: all the branches) and
// is not needed for a solid or tube section; IOUTINi picks the segments of the branch: 0 all, 1 the first location of a
// thin wall section or the outer side of a solid or tube, 2 the second location or the inner side of a tube, 3 the
// thickness direction of a branch, 4 a segment without thickness, and up to three of these digits together (12, 13, 14,
// 23, 24, 123).
struct ContactBeamSectionFlagsCard
{
    base::EntityId id = 0;                             // the PBEAML or PBARL of the section
    std::vector<std::array<std::int32_t, 2>> segments; // (IBRNCH, IOUTIN) of each pair
};

} // namespace oor::model
