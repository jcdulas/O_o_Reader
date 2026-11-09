#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One FORCE1 card: a concentrated force at a grid point whose direction is the line from G1 to G2.
//
// Card layout: FORCE1  SID  G  F  G1  G2
// The force is F times the unit vector from G1 to G2 (the two points may not be coincident).
struct PointForceLoad1Card
{
    base::EntityId id = 0;
    base::EntityId node = 0; // G
    double magnitude = 0.0;  // F
    base::EntityId node1 = 0;
    base::EntityId node2 = 0;
};

} // namespace oor::model
