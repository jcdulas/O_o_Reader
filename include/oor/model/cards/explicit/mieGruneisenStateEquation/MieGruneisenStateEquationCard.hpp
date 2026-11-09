#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One EOSMG card (SOL 700): the Mie-Gruneisen equation of state, used for the pressure of solids under high strain
// rates (shock Hugoniot form). A MATDEUL names it in its EID field.
//
// Card layout: EOSMG  EID  C  S  R1  RM            fields 1 to 5
// C is the sound speed at the reference density, S the slope of the linear shock velocity against particle velocity,
// R1 the Gruneisen parameter at the reference density (the format reference grid prints it R1 and its list prints the
// Greek letter), and RM the cut-off value of the density, which only limits the pressure. All four are required reals;
// the format states no range for them, so the card checks that they are finite. The format says that RM must stay below
// s / (s - 1) times the reference density to avoid a division by zero: the reference density is the one of the
// material, a rule of the model and not of this card.
struct MieGruneisenStateEquationCard
{
    base::EntityId id = 0;           // unique among the equations of state
    double soundSpeed = 0.0;         // sound speed at the reference density
    double shockSlope = 0.0;         // linear shock velocity slope
    double gruneisenParameter = 0.0; // Gruneisen parameter at the reference density
    double densityCutoff = 0.0;      // cut-off value of the density
};

} // namespace oor::model
