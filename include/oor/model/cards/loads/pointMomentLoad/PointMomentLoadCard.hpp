#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One MOMENT card: a concentrated moment at a grid point.
//
// Card layout: MOMENT  SID  G  CID  M  N1  N2  N3
// The moment is M times the vector (N1, N2, N3), given in coordinate system CID (0 = basic).
struct PointMomentLoadCard
{
    base::EntityId id = 0;
    base::EntityId node = 0;
    base::EntityId frameId = 0;
    double magnitude = 0.0;
    std::array<double, 3> direction{0.0, 0.0, 0.0};
};

} // namespace oor::model
