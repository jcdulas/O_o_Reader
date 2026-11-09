#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One CVISC card: a viscous damper between two grid points, with a PVISC property.
//
// Card layout:
//   CVISC  EID  PID  G1  G2
//
// G1 and G2 are two different grid points.
struct ViscousDamper2Card
{
    base::EntityId id = 0;
    base::EntityId propId = 0; // stored resolved: a blank PID is EID
    base::EntityId node1 = 0;
    base::EntityId node2 = 0;
};

} // namespace oor::model
