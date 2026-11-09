#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One pressure point of a PRESPT card: a point of an axisymmetric fluid at which the pressure is recovered, and that
// may also measure pressure for a control device (it needs an AXIF card in the deck, which is a rule between cards).
//
// Card layout: the card defines up to three points on one fluid point,
//   PRESPT  IDF  IDP1  PHI1  IDP2  PHI2  IDP3  PHI3
// (any pair may be blank, the second one too while the third is given), so the library keeps one point per row
// (`AxisymFluidPressurePointFormat::readAll` gives them) and writes each as a card of its own with the layout above and
// one pair.
//
// IDF is the number of the RINGFL fluid point the points are on, IDP the number of the pressure point, in the node ID
// space (the format reference: unique with respect to the scalar, structural and fluid points), and PHI its azimuthal
// position in the fluid coordinate system (a real with no default: both fields of a pair are required).
//
// The format draws the IDP1 in field 2, right after the IDF. A public demonstration deck leaves field 2
// blank and starts the pairs in field 3 (`PRESPT 7 (blank) 27 30. 28 60.`). A blank IDP1 is an error in the form of the
// format, so the reader takes a blank field 2 for that other layout (the pairs then fill fields 3 to 8): no card of the
// format is read differently, and the writer always writes the layout of the format.
struct AxisymFluidPressurePointCard
{
    base::EntityId fluidPointId = 0; // the RINGFL it is on
    base::EntityId pointId = 0;      // the pressure point
    double phiAngle = 0.0;           // azimuthal position, in degrees
};

} // namespace oor::model
