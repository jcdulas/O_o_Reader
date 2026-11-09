#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One PFAST card: the property of a fastener element (CFAST).
//
// Card layout:
//   PFAST  PID  D    MCID  MFLAG  KT1  KT2  KT3  KR1
//          KR2  KR3  MASS  GE     ALPHA TREF COINL
//
// MCID is the element stiffness coordinate system (-1, the default, is none); MFLAG says whether it is relative (0) or
// absolute (1). The translational stiffnesses KT1 to KT3 are required, the rotational ones are 0.0 when blank.
struct FastenerPropCard
{
    static constexpr base::EntityId NO_FRAME = -1;

    base::EntityId id = 0;
    double diameter = 0.0; // diameter of the fastener
    base::EntityId matFrameId = NO_FRAME;
    int absoluteFrame = 0; // 0: MCID is relative, 1: absolute
    std::array<double, 3> translationalStiffness = {};
    std::array<double, 3> rotationalStiffness = {};
    double mass = 0.0;                 // lumped mass of the fastener
    double structuralDamping = 0.0;    // structural damping
    double alphaCoefficient = 0.0;     // thermal expansion coefficient
    double referenceTemperature = 0.0; // reference temperature of the thermal loads
    double coincidentLength = 0.0;     // length of a fastener whose grid points coincide
};

} // namespace oor::model
