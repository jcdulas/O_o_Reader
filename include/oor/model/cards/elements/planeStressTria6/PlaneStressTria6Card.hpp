#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>

namespace oor::model
{

// One CPLSTS6 card: a plane stress triangle with three corners and up to three edge points (a card of another
// implementation: it is not in the format reference).
//
// Layout:
//   CPLSTS6  EID    PID  G1  G2  G3  G4  G5  G6
//            THETA
//
// G1 to G3 are the corners and G4 to G6 the optional points on the edges: `nodes` always has room for all six, and an
// edge point that is blank on the card is 0.
struct PlaneStressTria6Card
{
    static constexpr std::size_t CORNER_COUNT = 3;
    static constexpr std::size_t NODE_COUNT = 6;

    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, NODE_COUNT> nodes = {};
    double thetaInDegrees = 0.0; // material property orientation angle
};

} // namespace oor::model
