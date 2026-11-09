#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One GUST card: a stationary vertical gust for the aeroelastic response analyses, selected by the Case Control command
// GUST = SID.
//
// Card layout: GUST  SID  DLOAD  WG  X0  V
// The gust angle at the streamwise position X and time t is WG times the function of the load DLOAD at the time
// t - (X - X0) / V: the gust reaches the reference point X0 at the time the function says and travels at the velocity V
// of the vehicle. DLOAD is the set ID of a TLOADi or RLOADi (the time or frequency dependence). WG is the gust velocity
// over the forward velocity, any number but zero; a random analysis takes 1 / velocity and puts the rms value in the
// TABRNDG. X0 is a coordinate of the aerodynamic system and has no default, so all five fields are required.
//
// V must equal the VELOCITY of the AERO card and X0 and V may not change between the subcases of a run:
// rules between cards, not checked here. V is a positive real that, unlike the VELOCITY of the AERO card,
// may not be left blank, so it is required here.
struct AeroGustCard
{
    base::EntityId id = 0;
    base::EntityId dynamicLoadId = 0; // the set ID of a TLOADi or RLOADi
    double gustScale = 0.0;           // not zero
    double gustOriginX = 0.0;
    double velocity = 0.0; // positive
};

} // namespace oor::model
