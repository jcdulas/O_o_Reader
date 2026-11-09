#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One POINTAX card: a point on a ring of a conical shell model, where FORCE or MOMENT loads may be applied and
// displacements requested. The point is not subject to constraints of an MPCAX, SPCAX or OMITAX. The card needs an AXIC
// card in the deck, which is a rule between cards.
//
// Card layout:
//   POINTAX  ID  RID  PHI
//
// The ID is in the node ID space (the format reference asks it to be unique among the POINTAX, RINGAX and SECTAX
// numbers: a RINGAX shares the space, the SECTAX, which is typed apart, is a rule between cards). RID is the number of
// the RINGAX the point is on (a node reference) and PHI the azimuthal angle in degrees, a real the format gives no
// default for, so it is required.
struct AxisymConePointCard
{
    base::EntityId id = 0;
    base::EntityId ringId = 0; // the RINGAX the point is on
    double phiAngle = 0.0;     // azimuthal angle, in degrees
};

} // namespace oor::model
