#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One CELAS1 card: a scalar spring that refers to a PELAS property.
//
// Card layout:
//   CELAS1  EID  PID  G1  C1  G2  C2
//
// G1 and G2 are grid points or scalar points; 0 (or blank) is a grounded end, a point whose displacement is zero. C1
// and C2 are the component (1 to 6) at a grid point, and 0 at a scalar point or at ground.
struct SpringDofPropertyCard
{
    base::EntityId id = 0;
    base::EntityId propId = 0; // stored resolved: a blank PID is EID
    base::EntityId node1 = 0;
    std::int32_t components1 = 0;
    base::EntityId node2 = 0;
    std::int32_t components2 = 0;
};

} // namespace oor::model
