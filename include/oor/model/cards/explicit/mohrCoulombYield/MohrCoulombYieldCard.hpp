#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One YLDMC card (SOL 700): the Mohr-Coulomb yield model. The yield stress follows the pressure P up to a cap:
// y = min(Y1, Y2 + Y3 P). Only Eulerian materials with shear strength can use it. A MATDEUL names the model in its YID
// field.
//
// Card layout: YLDMC  YID  Y1  Y2  Y3            fields 1 to 4
// Y1 is the cap of the yield stress, Y2 the cohesion (the yield stress at zero pressure) and Y3 the internal friction
// (the slope of the line against the pressure, drawn as an angle). The three reals are required and the format
// reference gives no sign for them, so the card only asks for finite numbers. The format reference lists YLDMC under
// the family of the nonlinear materials; it is a yield model like the others (category YieldModel), whose ID is shared
// with them.
struct MohrCoulombYieldCard
{
    base::EntityId id = 0;         // unique among the yield models
    double maxYieldStress = 0.0;   // cap of the yield stress
    double cohesion = 0.0;         // cohesion
    double internalFriction = 0.0; // internal friction
};

} // namespace oor::model
