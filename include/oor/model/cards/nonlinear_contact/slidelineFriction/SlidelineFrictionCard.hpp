#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One BFRIC card: the friction between two bodies of a slideline contact region (SOL 106 and 129; SOL 400 has the
// contact tables instead). The FRICID of a BCONP names it.
//
// Card layout: BFRIC  FID  (blank)  FSTIF  MU1
struct SlidelineFrictionCard
{
    base::EntityId id = 0;       // FID: unique among the friction IDs
    double stickStiffness = 0.0; // frictional stiffness in stick; 0.0 when blank: the program chooses it
    double staticFriction = 0.0; // the coefficient of static friction
};

} // namespace oor::model
