#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One CMASS2 card: a scalar mass element that holds its own mass value (no property).
//
// Card layout:
//   CMASS2  EID  M  G1  C1  G2  C2
//
// G1 and G2 are grid points or scalar points; 0 (or blank) is a grounded end, a point whose displacement is zero. C1
// and C2 are the component (1 to 6) at a grid point, and 0 at a scalar point or at ground.
struct MassDofInlineCard
{
    base::EntityId id = 0;
    double mass = 0.0; // mass
    base::EntityId node1 = 0;
    std::int32_t components1 = 0;
    base::EntityId node2 = 0;
    std::int32_t components2 = 0;
};

} // namespace oor::model
