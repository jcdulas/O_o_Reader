#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// One CQUADR card: a four-node plate element with a drilling rotation.
//
// Card layout:
//   CQUADR  EID  PID  G1  G2  G3  G4  THETA or MCID  ZOFFS
//                TFLAG  T1  T2  T3  T4
struct ShellQuad4DrillingCard
{
    static constexpr base::EntityId NO_MAT_FRAME_ID = -1;
    static constexpr double NO_THICKNESS = -1.0;

    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, 4> nodes = {};

    // format reference: THETA and MCID share field 7; a real is an angle in degrees, an integer a coordinate system ID.
    double thetaInDegrees = 0.0;
    base::EntityId matFrameId = NO_MAT_FRAME_ID; // NO_MAT_FRAME_ID: the material direction is given by THETA

    double offset = 0.0;
    std::int32_t thicknessFlag = 0;
    // T1..T4: thicknesses at the corners; NO_THICKNESS where blank (the PSHELL thickness is used).
    std::array<double, 4> cornerThickness = {NO_THICKNESS, NO_THICKNESS, NO_THICKNESS, NO_THICKNESS};

    [[nodiscard]] bool usesMaterialSystem() const noexcept { return matFrameId != NO_MAT_FRAME_ID; }
};

} // namespace oor::model
