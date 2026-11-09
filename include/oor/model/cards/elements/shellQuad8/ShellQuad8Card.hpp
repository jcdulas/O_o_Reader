#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace oor::model
{

// One CQUAD8 card: a curved quadrilateral shell element with four corners and up to four edge points.
//
// Card layout:
//   CQUAD8  EID  PID  G1  G2  G3  G4  G5  G6
//           G7   G8   T1  T2  T3  T4  THETA/MCID  ZOFFS
//           TFLAG
//
// G1 to G4 are the corners and G5 to G8 the optional points on the edges G1-G2, G2-G3, G3-G4 and G4-G1: `nodes` always
// has room for all eight, and an edge point that is blank on the card is 0, so that the edge a point belongs to is its
// position.
struct ShellQuad8Card
{
    static constexpr std::size_t CORNER_COUNT = 4;
    static constexpr std::size_t NODE_COUNT = 8;
    static constexpr base::EntityId NO_MAT_FRAME_ID = -1;
    static constexpr double NO_THICKNESS = -1.0;

    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, NODE_COUNT> nodes = {};

    // format reference: THETA and MCID share field 15; a real is an angle in degrees, an integer a coordinate system
    // ID.
    double thetaInDegrees = 0.0;
    base::EntityId matFrameId = NO_MAT_FRAME_ID; // NO_MAT_FRAME_ID: the material direction is given by THETA

    double offset = 0.0;
    std::int32_t thicknessFlag = 0;
    // T1..T4: thicknesses at the corners; NO_THICKNESS where blank (the PSHELL thickness is used).
    std::array<double, CORNER_COUNT> cornerThickness = {NO_THICKNESS, NO_THICKNESS, NO_THICKNESS, NO_THICKNESS};

    [[nodiscard]] bool usesMaterialSystem() const noexcept { return matFrameId != NO_MAT_FRAME_ID; }
    [[nodiscard]] bool hasEdgeNodes() const noexcept
    {
        for (std::size_t index = CORNER_COUNT; index < NODE_COUNT; ++index)
        {
            if (nodes[index] != 0)
                return true;
        }
        return false;
    }
};

} // namespace oor::model
