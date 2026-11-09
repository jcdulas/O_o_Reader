#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One CMASS3 card: a scalar mass element between two scalar points, with a PMASS property.
//
// Card layout:
//   CMASS3  EID  PID  S1  S2
//
// S1 and S2 are scalar points; 0 (or blank) is a grounded end.
struct MassScalarPropertyCard
{
    base::EntityId id = 0;
    base::EntityId propId = 0; // stored resolved: a blank PID is EID
    base::EntityId scalarPoint1 = 0;
    base::EntityId scalarPoint2 = 0;
};

} // namespace oor::model
