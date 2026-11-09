#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One free surface point of a FREEPT card: a point on the free surface of an axisymmetric fluid at which the surface
// displacements are recovered (it needs an AXIF card in the deck, which is a rule between cards).
//
// Card layout: the card defines up to three points on one fluid point,
//   FREEPT  IDF  IDP1  PHI1  IDP2  PHI2  IDP3  PHI3
// so the library keeps one point per row (`AxisymFluidFreePointFormat::readAll` gives them) and writes each as a card
// of its own with the layout above and one pair.
//
// IDF is the number of the RINGFL fluid point the points are on, IDP the number of the free surface point, in the node
// ID space (the format reference: unique with respect to the scalar, structural and fluid points), and PHI its
// azimuthal position in the fluid coordinate system (a real with no default: both fields of a pair are required). The
// format reference asks the IDF to be in a free surface list (FSLIST), which is a rule between cards.
//
// The format draws the IDP1 in field 2, right after the IDF. A public demonstration deck leaves field 2
// blank and starts the pairs in field 3 (`FREEPT 1 (blank) 40 15. 41 30. 42 45.`). A blank IDP1 is an error in the form
// of the format, so the reader takes a blank field 2 for that other layout (the pairs then fill fields 3 to 8): no card
// of the format is read differently, and the writer always writes the layout of the format.
struct AxisymFluidFreePointCard
{
    base::EntityId fluidPointId = 0; // the RINGFL it is on
    base::EntityId pointId = 0;      // the free surface point
    double phiAngle = 0.0;           // azimuthal position, in degrees
};

} // namespace oor::model
