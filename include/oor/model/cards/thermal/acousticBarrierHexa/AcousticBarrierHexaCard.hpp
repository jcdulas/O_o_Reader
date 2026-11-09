#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>

namespace oor::model
{

// One CHACBR card: an acoustic barrier element, a brick of the coupled fluid-structure analysis.
//
// Card layout:
//   CHACBR  EID  PID  G1   G2   G3   G4   G5   G6
//                     G7   G8   G9   G10  G11  G12
//                                    G17  G18  G19  G20
// (the fields are 1 to 14, then 19 to 22: the edge points G13 to G16 do not exist). G1 to G4 go round one quadrilateral
// face, in order, and G5 to G8 are on the opposite face with G5 opposite G1; the edge points G9 to G12 (the face of G1
// to G4) and G17 to G20 (the face of G5 to G8) are optional, and the corners are not. PID is a PACBAR. `nodes` has G1
// to G12 and then G17 to G20, 0 where an edge point is absent.
struct AcousticBarrierHexaCard
{
    static constexpr std::size_t NODE_COUNT = 16;

    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, NODE_COUNT> nodes = {};
};

} // namespace oor::model
