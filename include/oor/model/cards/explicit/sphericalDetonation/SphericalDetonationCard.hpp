#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One DETSPH card (SOL 700): the ignition point of a spherical detonation wave. An Euler element of a high explosive
// detonates when the wave that left the point at the given time reaches it.
//
// Card layout: DETSPH  DID  MID  X  Y  Z  VEL  TIME            fields 1 to 7
// DID numbers the detonation. MID is the MATDEUL of the exploding material (required). X, Y and Z are the coordinates
// of the point, VEL the speed of the wave and TIME the detonation time; each is zero when blank, and VEL and TIME are
// not negative (the format draws the limit as zero or more: the default of both is zero).
struct SphericalDetonationCard
{
    base::EntityId id = 0;    // unique among the DETSPH cards
    base::EntityId matId = 0; // a MATDEUL
    double pointX = 0.0;
    double pointY = 0.0;
    double pointZ = 0.0;
    double detonationSpeed = 0.0; // speed of the detonation wave, not negative
    double detonationTime = 0.0;  // detonation time, not negative
};

} // namespace oor::model
