#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One CDAMP5 card: a damper (heat capacity, for heat transfer analysis) between two points, with a PDAMP5 property.
//
// Card layout:
//   CDAMP5  EID  PID  G1  G2
//
// G1 and G2 are grid points or scalar points; 0 (or blank) is a grounded end.
struct HeatCapacity2Card
{
    base::EntityId id = 0;
    base::EntityId propId = 0; // stored resolved: a blank PID is EID
    base::EntityId node1 = 0;
    base::EntityId node2 = 0;
};

} // namespace oor::model
