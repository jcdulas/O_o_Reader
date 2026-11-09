#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One EULFOR card (SOL 700): a body force (an acceleration per unit mass) on the Eulerian elements, in the direction of
// a vector, scaled by a number, by a table of time, or by both. A PEULER or a flow-boundary definition does not name it
// directly: its number is in the space of the body forces, which the EULFOR1 shares (category EulerBodyForce).
//
// Card layout: EULFOR  BID  CID  VALUE  TID  N1  N2  N3            fields 1 to 7
// CID is the coordinate system the vector (N1, N2, N3) is given in (0 or blank: basic). TID is a TABLED1 of time that
// gives the scale factor. The format reference asks for a nonzero VALUE or a TID (one of the two) and
// for at least one nonzero component; the card checks both. A blank VALUE is the same as 0.0 (it has no
// default, and a zero VALUE means no force), and so is a blank component.
struct EulerBodyForceCard
{
    base::EntityId id = 0;      // unique among the body forces
    base::EntityId frameId = 0; // a coordinate system, 0 for the basic one
    double scaleFactor = 0.0;   // the scale factor, not negative
    base::EntityId tableId = 0; // a TABLED1 of the scale factor over time, 0 for none
    double directionX = 0.0;    // the components of the direction in the system CID
    double directionY = 0.0;
    double directionZ = 0.0;
};

} // namespace oor::model
