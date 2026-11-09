#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// One RFORCE card: a static load from an angular velocity and/or an angular acceleration about an axis.
//
// Card layout: RFORCE  SID  G  CID  A  R1  R2  R3  METHOD
//                     RACC  MB
// The axis passes through grid point G (0 or blank: through the origin of the basic system) in the direction (R1, R2,
// R3) of coordinate system CID. A is the angular velocity in revolutions per unit time, RACC the angular acceleration
// in revolutions per unit time squared. A blank A or RACC is 0.0; METHOD (1 or 2) is 1 when blank.
//
// IDRF is a field of the format (ignored for SOL 700) and read in field 11, the third field of the
// continuation line; it is kept as written and is 0 when blank.
struct RotationalForceLoadCard
{
    base::EntityId id = 0;
    base::EntityId node = 0; // G; 0 for the origin
    base::EntityId frameId = 0;
    double magnitude = 0.0;
    std::array<double, 3> axis{0.0, 0.0, 0.0}; // R1 to R3
    std::int32_t methodName = 1;
    double angularAcceleration = 0.0;
    std::int32_t mainBulkFlag = 0;
    base::EntityId regionId = 0;
};

} // namespace oor::model
