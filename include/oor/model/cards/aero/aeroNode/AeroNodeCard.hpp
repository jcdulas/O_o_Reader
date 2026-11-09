#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One AEGRID card: a grid point of an aerodynamic mesh that the AEQUAD4 and AETRIA3 cards connect. The point has
// nothing to do with the structural grid points (GRID): its own ID space is the one of the AEGRID cards.
//
// Card layout: AEGRID  GID  CP  X1  X2  X3  CD  ASID
// The point is at (X1, X2, X3) in the coordinate system CP (the meaning of the three numbers depends on the type of CP,
// as for a GRID); the degrees of freedom of the point are in the system CD. A zero or blank CP or CD is the basic
// system. A point that names an AESCALE through ASID has its coordinates scaled first and transformed to the basic
// system afterwards.
//
// The format reference gives no default for X1, X2 and X3, but points the reader to the GRID entry for their meaning: a
// blank one is read as 0.0 (as on a GRID), and the card always writes the three of them. The AEGRID, AETRIA3, AEQUAD4
// and AESCALE cards describe an aerodynamic mesh in a portable form; no aerodynamics are built on it.
struct AeroNodeCard
{
    base::EntityId id = 0;              // unique among the AEGRID cards
    base::EntityId positionFrameId = 0; // the system of X1, X2 and X3 (0: basic)
    double xCoord1 = 0.0;
    double xCoord2 = 0.0;
    double xCoord3 = 0.0;
    base::EntityId displacementFrameId = 0; // the system of the degrees of freedom (0: basic)
    base::EntityId scaleId = 0;             // an AESCALE, 0 for none
};

} // namespace oor::model
