#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>

namespace oor::model
{

// One CQUAD card: a fully nonlinear plane strain quadrilateral element (PLPLANE or PLCOMP property) with four corners,
// up to four edge points and an optional centre point.
//
// Card layout:
//   CQUAD  EID  PID  G1  G2  G3  G4  G5  G6
//           G7   G8   G9  THETA or MCID
//
// G1 to G4 are the corners, G5 to G8 the optional points on the edges and G9 the optional centre point: `nodes` always
// has room for all nine, and a point that is blank on the card is 0.
struct PlaneStrainQuadCard
{
    static constexpr std::size_t CORNER_COUNT = 4;
    static constexpr std::size_t NODE_COUNT = 9;
    static constexpr base::EntityId NO_MAT_FRAME_ID = -1;

    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, NODE_COUNT> nodes = {};
    // THETA and MCID share field 12; a real is an angle in degrees, an integer a coordinate system ID.
    double thetaInDegrees = 0.0;
    base::EntityId matFrameId = NO_MAT_FRAME_ID; // NO_MAT_FRAME_ID: the material direction is given by THETA

    [[nodiscard]] bool usesMaterialSystem() const noexcept { return matFrameId != NO_MAT_FRAME_ID; }
};

} // namespace oor::model
