#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One MOMENT1 card: a concentrated moment at a grid point whose axis is the line from G1 to G2.
//
// Card layout: MOMENT1  SID  G  M  G1  G2
// The moment is M times the unit vector from G1 to G2 (the two points may not be coincident).
struct PointMomentLoad1Card
{
    base::EntityId id = 0;
    base::EntityId node = 0; // G
    double magnitude = 0.0;  // M
    base::EntityId node1 = 0;
    base::EntityId node2 = 0;
};

} // namespace oor::model
