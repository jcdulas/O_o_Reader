#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One scalar mass property of a PMASS card.
//
// Card layout (a card defines one to four properties, each of two fields):
//   PMASS  PID1  M1  PID2  M2  PID3  M3  PID4  M4
//
// A card with several properties gives one MassPropCard value each, written as a card of its own. A blank mass is 0.0
// (the format reference gives no default).
struct MassPropCard
{
    base::EntityId id = 0;
    double mass = 0.0; // the mass
};

} // namespace oor::model
