#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace oor::model
{

// One CTRIA6 card: a curved triangular shell element with three corners and up to three edge points.
//
// Card layout:
//   CTRIA6  EID         PID    G1  G2  G3  G4  G5  G6
//           THETA/MCID  ZOFFS  T1  T2  T3  TFLAG
//
// G1 to G3 are the corners and G4 to G6 the optional points on the edges G1-G2, G2-G3 and G3-G1: `nodes` always has
// room for all six, and an edge point that is blank on the card is 0, so that the edge a point belongs to is its
// position.
struct ShellTria6Card
{
    static constexpr std::size_t CORNER_COUNT = 3;
    static constexpr std::size_t NODE_COUNT = 6;
    static constexpr base::EntityId NO_MAT_FRAME_ID = -1;
    static constexpr double NO_THICKNESS = -1.0;

    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, NODE_COUNT> nodes = {};

    // format reference: THETA and MCID share field 9; a real is an angle in degrees, an integer a coordinate system ID.
    double thetaInDegrees = 0.0;
    base::EntityId matFrameId = NO_MAT_FRAME_ID; // NO_MAT_FRAME_ID: the material direction is given by THETA

    double offset = 0.0;
    std::int32_t thicknessFlag = 0;
    // T1..T3: thicknesses at the corners; NO_THICKNESS where blank (the PSHELL thickness is used).
    std::array<double, CORNER_COUNT> cornerThickness = {NO_THICKNESS, NO_THICKNESS, NO_THICKNESS};

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
