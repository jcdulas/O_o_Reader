#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/aero/AeroSupport.hpp"

namespace oor::model
{

// One SPLINE2 card: a beam spline, a line along which the structure bends and twists, that carries the displacements of
// a set of structural grid points to a row of aerodynamic boxes (or the elements of a body) and the forces back.
//
// Card layout: SPLINE2  EID   CAERO  ID1  ID2  SETG  DZ  DTOR  CID
//                      DTHX  DTHY        USAGE
// Field 11 (the blank cell before USAGE) is not used. CAERO is the aerodynamic element (a CAERO1 to CAERO5,
// ReferenceTarget::AeroElement) that is interpolated: a panel, or a slender body (CAERO2). ID1 to ID2 are the first and
// last box of the panel, or body element, that the spline moves (ReferenceTarget::AeroBox: a number that no CAERO owns
// is not an error here, because with the Mach box method the numbers count the (x, y) pairs of an AEFACT). ID2 is not
// below ID1 (the format reference field table says ID2 >= ID1; its text loses the sign). SETG is the SET1 or SET3 of
// structural grid points. CID is the rectangular coordinate system whose y axis, projected on the plane of the panel,
// is the axis of the beam; a body takes its axis from the aerodynamic x axis and ignores it. The basic system (0) is
// the default.
//
// The three flexibilities smooth the fit: DZ (zero or more, 0.0 by default) for the linear attachment, DTHX and DTHY
// for the rotations about the x and y axes of the spline. A zero value attaches rigidly and a NEGATIVE DTHX or DTHY
// removes the attachment of that rotation altogether, so their sign is meaningful and is kept. They have
// no default and are often left blank: a blank is 0.0, a rigid attachment. DTOR is the ratio of the
// torsional to the bending flexibility (EI/GJ, positive, 1.0 by default; 1.0 for a body).
//
// The continuation line is required, so the card always writes DTHX and DTHY, zeros included, and reads
// a card without it as two zeros.
struct AeroBeamSpline2Card
{
    base::EntityId id = 0;
    base::EntityId aeroPanelId = 0;
    base::EntityId firstBox = 0;
    base::EntityId lastBox = 0;
    base::EntityId nodeSetId = 0;
    double linearFlexibility = 0.0;
    double bendingTorsionRatio = 1.0;
    base::EntityId frameId = 0;
    double rotationFlexibilityX = 0.0;
    double rotationFlexibilityY = 0.0;
    SplineUsage usage = SplineUsage::Both;
};

} // namespace oor::model
