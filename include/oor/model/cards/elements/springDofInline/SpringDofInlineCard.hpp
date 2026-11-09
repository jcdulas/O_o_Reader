#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One CELAS2 card: a scalar spring that holds its own stiffness (no property).
//
// Card layout:
//   CELAS2  EID  K  G1  C1  G2  C2  GE  S
//
// G1 and G2 are grid points or scalar points; 0 (or blank) is a grounded end, a point whose displacement is zero. C1
// and C2 are the component (1 to 6) at a grid point, and 0 at a scalar point or at ground.
struct SpringDofInlineCard
{
    base::EntityId id = 0;
    double stiffness = 0.0; // stiffness
    base::EntityId node1 = 0;
    std::int32_t components1 = 0;
    base::EntityId node2 = 0;
    std::int32_t components2 = 0;
    double structuralDamping = 0.0; // damping coefficient
    double stressCoefficient = 0.0; // stress coefficient
};

} // namespace oor::model
