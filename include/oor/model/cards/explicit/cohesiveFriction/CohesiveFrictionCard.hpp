#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One COHFRIC card (SOL 700): a cohesive friction, which lets friction and sticking act at the coupling surface while
// the Euler material pulls on it (tension). A COUCOHF names it.
//
// Card layout: COHFRIC  CID  MAXSTRS  FRIC  REFVEL            fields 1 to 4
// MAXSTRS is the largest tensile normal stress the surface carries before the friction stops acting, FRIC the friction
// stress under tension and REFVEL a reference velocity: in tension, a relative tangential velocity gives a shear stress
// that grows with it up to FRIC (a viscous-like law). The three are zero or more and 0.0 when blank.
struct CohesiveFrictionCard
{
    base::EntityId id = 0; // unique among the COHFRIC cards
    double maxStress = 0.0;
    double friction = 0.0;
    double referenceVelocity = 0.0;
};

} // namespace oor::model
