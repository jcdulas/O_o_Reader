#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>

namespace oor::model
{

// One CQUADX8 card: an axisymmetric quadrilateral ring element with four corners and up to four edge points
// (a card of another implementation: it is not in the format reference).
//
// Layout:
//   CQUADX8  EID    PID  G1  G2  G3  G4  G5  G6
//            G7   G8   THETA
//
// G1 to G4 are the corners and G5 to G8 the optional points on the edges: `nodes` always has room for all eight, and an
// edge point that is blank on the card is 0.
struct AxisymQuad8Card
{
    static constexpr std::size_t CORNER_COUNT = 4;
    static constexpr std::size_t NODE_COUNT = 8;

    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, NODE_COUNT> nodes = {};
    double thetaInDegrees = 0.0; // material property orientation angle
};

} // namespace oor::model
