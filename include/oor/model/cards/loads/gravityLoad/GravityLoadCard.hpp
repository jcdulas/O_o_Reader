#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// One GRAV card: an acceleration (gravity) load.
//
// Card layout: GRAV  SID  CID  A  N1  N2  N3  MB
// The acceleration is A times the vector (N1, N2, N3), given in coordinate system CID (0 = basic). The vector must not
// be zero. MB tells whether CID is defined in the main Bulk Data (-1) or in the partitioned superelement (0).
struct GravityLoadCard
{
    base::EntityId id = 0;
    base::EntityId frameId = 0;
    double magnitude = 0.0; // A
    std::array<double, 3> direction{0.0, 0.0, 0.0};
    std::int32_t mainBulkFlag = 0;
};

} // namespace oor::model
