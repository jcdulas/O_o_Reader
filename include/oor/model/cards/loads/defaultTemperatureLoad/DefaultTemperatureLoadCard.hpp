#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One default temperature of a TEMPD card: the temperature of every grid point of temperature set SID that no TEMP card
// gives a temperature.
//
// Card layout: TEMPD  SID1  T1  SID2  T2  SID3  T3  SID4  T4
// A card holds one to four such pairs, each for a set of its own: the pairs are separate objects here, one row each,
// and are written as cards of their own.
struct DefaultTemperatureLoadCard
{
    base::EntityId id = 0;
    double temperature = 0.0;
};

} // namespace oor::model
