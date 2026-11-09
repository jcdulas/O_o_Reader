#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One AEQUAD4 card: a quadrilateral element of an aerodynamic mesh, given by four aerodynamic grid points (AEGRID).
//
// Card layout: AEQUAD4  EID  CMPID  G1  G2  G3  G4
// The four grid points are different: the quadrilateral may collapse to a triangle when two of them are at the same
// place, but never by naming one point twice. CMPID is the number of the aerodynamic component the
// element belongs to. No card of the model owns that number (the AECOMP of LISTTYPE CMPID lists such numbers, and an
// AETRIA3 has one too), so it is a plain ID: it is not looked up and a renumbering does not touch it.
//
// The element IDs are those of the aerodynamic mesh, shared by the AEQUAD4 and AETRIA3 cards (a check of the model, not
// of the card, reports an ID that both define). They are not the structural elements and not the aerodynamic panels
// (CAERO).
struct AeroQuad4Card
{
    base::EntityId id = 0;          // unique among the AEQUAD4 cards
    base::EntityId componentId = 0; // the aerodynamic component (plain ID)
    base::EntityId node1 = 0;       // AEGRID points
    base::EntityId node2 = 0;
    base::EntityId node3 = 0;
    base::EntityId node4 = 0;
};

} // namespace oor::model
