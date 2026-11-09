#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One ATBACC card (SOL 700): an acceleration field, usually a crash pulse, that is applied to segments of the dummy of
// an external occupant model. The acceleration at time t is the direction (NX, NY, NZ) times SCALE times the value
// of the table that a TLOADn of the set interpolates at t; the solver multiplies it by the mass of each segment named
// on the card and adds the force as an external load.
//
// Card layout: ATBACC  LID  (blank)  SCALE  NX  NY  NZ  (blank)  (blank)  / NAME1  NAME2 ... NAME8  /  more names
// fields 1 to 6, then the names from field 9 on, eight to a line. LID is the number of a set of loads: a TLOADn names
// it in its EXCITEID, with a TYPE of zero, which is why the table says it defines an excitation
// set. Several ATBACC cards may exist, and the cards that share a LID are one set. The names are those of
// the B.2 entries of the input file of the occupant model, which is not part of the deck: they refer to no card.
//
// a common deck has a zero in fields 7 and 8 although the format leaves them blank; those two fields are read when
// they hold a zero and are not kept, so such a card reads without an error and is written without them.
struct OccupantAccelFieldCard
{
    base::EntityId setId = 0; // the set a TLOADn excites
    double scaleFactor = 1.0; // not negative
    double directionX = 0.0;  // the direction of the acceleration, at least one component is not zero
    double directionY = 0.0;
    double directionZ = 0.0;
    std::vector<std::string> names; // the occupant segments the field acts on, in upper case, at least one
};

} // namespace oor::model
