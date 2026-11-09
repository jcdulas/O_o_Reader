#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One FORCE card: a concentrated force at a grid point.
//
// Card layout: FORCE  SID  G  CID  F  N1  N2  N3
// The force is F times the vector (N1, N2, N3), given in coordinate system CID (0 = basic).
struct PointForceLoadCard
{
    base::EntityId id = 0;
    base::EntityId node = 0;
    base::EntityId frameId = 0;
    double magnitude = 0.0;
    std::array<double, 3> direction{0.0, 0.0, 0.0};
};

} // namespace oor::model
