#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One PRAC3D card: the property of a CRAC3D crack tip element.
//
// Card layout:
//   PRAC3D  PID  MID  GAMMA  PHI
//
// MID is a MAT1 or a MAT9. GAMMA is the exponent of the displacement field (0.5 by default) and PHI the angle in
// degrees, from the element x axis, along which the stress intensity factors are found (180.0 by default); the format
// reference says other values than the defaults of these two have not been tested.
struct CrackTipProp3DCard
{
    static constexpr double DEFAULT_GAMMA = 0.5;
    static constexpr double DEFAULT_PHI = 180.0;

    base::EntityId id = 0;
    base::EntityId matId = 0;
    double gammaCoefficient = DEFAULT_GAMMA;
    double phiAngle = DEFAULT_PHI;
};

} // namespace oor::model
