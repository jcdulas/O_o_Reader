#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One CONM2 card: a concentrated mass at a grid point, with its own offset and inertia.
//
// Card layout:
//   CONM2  EID  G    CID  M    X1   X2   X3
//          I11  I21  I22  I31  I32  I33
//
// CID is a coordinate system ID, 0 for the basic system, or -1: X1, X2, X3 are then the coordinates of the center of
// gravity in the basic system instead of an offset from the grid point. The format reference gives no default for M and
// the offsets; a blank one is 0.0.
struct PointMassInertiaCard
{
    static constexpr base::EntityId BASIC_COORDINATES = -1; // CID = -1

    base::EntityId id = 0;
    base::EntityId node = 0;
    base::EntityId frameId = 0;
    double mass = 0.0;
    std::array<double, 3> offset = {};  // X1 X2 X3
    std::array<double, 6> inertia = {}; // I11 I21 I22 I31 I32 I33
};

} // namespace oor::model
