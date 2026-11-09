#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One AETRIA3 card: a triangular element of an aerodynamic mesh, given by three aerodynamic grid points (AEGRID).
//
// Card layout: AETRIA3  EID  CMPID  G1  G2  G3
// CMPID is the number of the aerodynamic component the element belongs to. No card of the model owns that number (the
// AECOMP of LISTTYPE CMPID lists such numbers), so it is a plain ID: it is not looked up and a renumbering does not
// touch it. The format reference does not say that the three points are different (it says so for the AEQUAD4); a
// triangle that names one point twice has no area, so this card refuses it as the AEQUAD4 does.
//
// The element IDs are those of the aerodynamic mesh, shared by the AEQUAD4 and AETRIA3 cards (a check of the model, not
// of the card, reports an ID that both define).
struct AeroTria3Card
{
    base::EntityId id = 0;          // unique among the AETRIA3 cards
    base::EntityId componentId = 0; // the aerodynamic component (plain ID)
    base::EntityId node1 = 0;       // AEGRID points
    base::EntityId node2 = 0;
    base::EntityId node3 = 0;
};

} // namespace oor::model
