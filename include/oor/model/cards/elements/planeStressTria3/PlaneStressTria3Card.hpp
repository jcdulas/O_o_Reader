#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// One CPLSTS3 card: a three-node plane stress element (a card of another implementation: it is not in the format
// reference).
//
// Layout:
//   CPLSTS3  EID    PID    G1  G2  G3  THETA
//                   TFLAG  T1  T2  T3
struct PlaneStressTria3Card
{
    static constexpr double NO_THICKNESS = -1.0;

    base::EntityId id = 0;
    base::EntityId propId = 0; // stored resolved: a blank PID is EID
    std::array<base::EntityId, 3> nodes = {};
    double thetaInDegrees = 0.0;    // material property orientation angle
    std::int32_t thicknessFlag = 0; // 0: Ti are thicknesses, 1: Ti are fractions of the property thickness
    // T1..T3: thicknesses at the corners; NO_THICKNESS where blank (the property thickness is used).
    std::array<double, 3> cornerThickness = {NO_THICKNESS, NO_THICKNESS, NO_THICKNESS};
};

} // namespace oor::model
