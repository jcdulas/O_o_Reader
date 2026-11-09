#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>

namespace oor::model
{

// One CTRIAX6 card: an axisymmetric triangular cross-section ring element with up to six grid points.
//
// Layout:
//   CTRIAX6  EID  MID  G1  G2  G3  G4  G5  G6
//            THETA
//
// The corners are G1, G3 and G5 (the format reference: the corner and edge points alternate around the perimeter,
// unlike the CTRIA6) and G2, G4 and G6 are the optional points between them. `nodes` always has room for all six, in
// the order of the card, and a point that is blank on the card is 0. The grid points lie in the x-z plane of the basic
// system.
struct AxisymTria6Card
{
    static constexpr std::size_t NODE_COUNT = 6;
    // Is the position (0 for G1) one of the three corners, which are required?
    [[nodiscard]] static constexpr bool isCorner(std::size_t position) noexcept { return position % 2 == 0; }

    base::EntityId id = 0;
    base::EntityId matId = 0;
    std::array<base::EntityId, NODE_COUNT> nodes = {};
    double thetaInDegrees = 0.0; // material property orientation angle
};

} // namespace oor::model
