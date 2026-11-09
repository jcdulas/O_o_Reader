#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>

namespace oor::model
{

// One CTRIAX card: an axisymmetric triangular element with three corners and up to three edge points.
//
// Layout:
//   CTRIAX  EID  PID  G1  G2  G3  G4  G5  G6
//           THETA or MCID
//
// G1 to G3 are the corners and G4 to G6 the optional points on the edges: `nodes` always has room for all six, and an
// edge point that is blank on the card is 0.
struct AxisymTriaCard
{
    static constexpr std::size_t CORNER_COUNT = 3;
    static constexpr std::size_t NODE_COUNT = 6;
    static constexpr base::EntityId NO_MAT_FRAME_ID = -1;

    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, NODE_COUNT> nodes = {};
    // THETA and MCID share field 9; a real is an angle in degrees, an integer a coordinate system ID.
    double thetaInDegrees = 0.0;
    base::EntityId matFrameId = NO_MAT_FRAME_ID; // NO_MAT_FRAME_ID: the material direction is given by THETA

    [[nodiscard]] bool usesMaterialSystem() const noexcept { return matFrameId != NO_MAT_FRAME_ID; }
};

} // namespace oor::model
