#pragma once

#include "oor/model/cards/sets/DofLists.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One BNDGRID card: the grid points on the boundary of a shape optimisation (SOL 200), with the components for which
// the boundary motion is prescribed (they form the s-set).
//
// Card layout: BNDGRID  C  GP1  GP2  GP3  GP4  GP5  GP6  GP7
//                      GP8  GP9  -etc.-
// or:         BNDGRID  C  GP1  "THRU"  GP2
// The list is kept as written: single points and THRU ranges, in order. C holds the digits 1 to 6 of the components as
// one number (it is required). The card has no ID.
struct ShapeBoundaryNodeSetCard
{
    std::int32_t components = 0;
    std::vector<IdRun> nodes;
};

} // namespace oor::model
