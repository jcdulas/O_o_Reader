#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One EULFOR1 card (SOL 700): the body force of the Eulerian regions of a set. It is the alternative to an EULFOR for
// the multi-material Euler solver: the acceleration is applied where the region and the material of an
// EULFREG say, so the card only names the set of regions.
//
// Card layout: EULFOR1  SID  ESID            fields 1 and 2
// SID is the number a PEULER or PEULER1 names in its SID2 (the category EulerBodyForce, shared with EULFOR). ESID is
// the number of the set of EULFREG cards that give the regions and the acceleration tables: several EULFREG share it.
struct EulerBodyForceSetCard
{
    base::EntityId id = 0;          // unique among the body forces
    base::EntityId regionSetId = 0; // a set of EULFREG cards
};

} // namespace oor::model
