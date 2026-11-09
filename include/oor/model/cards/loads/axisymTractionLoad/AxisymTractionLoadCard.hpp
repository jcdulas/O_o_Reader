#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One PLOADX1 card: a surface traction on the side of an axisymmetric element (CAXISYM, CQUADX, CTRIAX, CTRIAX6).
//
// Card layout: PLOADX1  SID  EID  PA  PB  GA  GB  THETA
// The traction varies linearly from PA at GA to PB at GB. PB defaults to PA, so it is stored resolved and always
// written. THETA is the angle in degrees between the traction and the inward normal of the side (0.0 when blank).
struct AxisymTractionLoadCard
{
    base::EntityId id = 0;
    base::EntityId elemId = 0;
    double tractionA = 0.0;
    double tractionB = 0.0;
    base::EntityId nodeA = 0;
    base::EntityId nodeB = 0;
    double tractionAngle = 0.0;
};

} // namespace oor::model
