#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One MOMENT2 card: a concentrated moment at a grid point whose axis is the cross product of the vector from G1 to
// G2 with the vector from G3 to G4.
//
// Card layout: MOMENT2  SID  G  M  G1  G2  G3  G4
// G1 and G2 may not be coincident, nor may G3 and G4.
struct PointMomentLoad2Card
{
    base::EntityId id = 0;
    base::EntityId node = 0; // G
    double magnitude = 0.0;  // M
    base::EntityId node1 = 0;
    base::EntityId node2 = 0;
    base::EntityId node3 = 0;
    base::EntityId node4 = 0;
};

} // namespace oor::model
