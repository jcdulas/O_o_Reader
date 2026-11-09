#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One fluid point of a RINGFL card: a circle of an axisymmetric fluid model (it needs an AXIF card in the deck, which
// is a rule between cards).
//
// Card layout: the card defines one or two fluid points,
//   RINGFL  IDFA  XA1  XA2  XA3  IDFB  XB1  XB2  XB3
// so the library keeps one point per row (`AxisymFluidRingFormat::readAll` gives them) and writes each as a card of its
// own.
//
// IDF is a number below 500000 in the node ID space (the format reference: unique with respect to the scalar,
// structural and fluid points). X1, X2 and X3 are the coordinates in the system of the CID of the AXIF card: (r, phi,
// z) for a cylindrical system and (rho, theta, phi) for a spherical one, the angle theta in degrees. X1 is above zero.
// The format reference says phi must be blank or zero, which is the second field of the cylindrical system and the
// third of the spherical one; only the AXIF tells which, so the card keeps both blanks as 0.0 and the check of phi is a
// rule between cards. A blank X2 or X3 is 0.0.
struct AxisymFluidRingCard
{
    base::EntityId id = 0;
    double position1 = 0.0;
    double position2 = 0.0;
    double position3 = 0.0;
};

} // namespace oor::model
