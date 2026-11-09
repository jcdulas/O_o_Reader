#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One MDROT1 card: a rotation of a module about an axis given by two points of the main Bulk Data, optionally turning
// about a reference point. The move takes effect only when an MDMOVE names its MVID.
//
// Card layout: MDROT1  MVID  RID  IDS  IDE  MAG            fields 1 to 5
// IDS and IDE are the GRID or POINT IDs of the start and the end of the rotation vector (the axis); RID is the point
// the module turns about, and a blank or a zero means that the module turns about the vector from IDS to IDE itself.
// MAG is the angle in degrees, any real but zero (a rotation by nothing is not a move); it has no default, so the card
// requires it. All the points are in module 0. The card refuses
// an IDS and an IDE that are the same point (the format reference does not say it, but an axis between a point and
// itself has no direction).
//
// The MVID is one number of the move kinds (see ModuleTranslationCard): IdSpace::PerCardType in the category
// ModuleMove.
struct ModuleRotation1Card
{
    base::EntityId id = 0;            // the move, referenced by an MDMOVE
    base::EntityId referenceNode = 0; // the reference point of the rotation, 0 when blank
    base::EntityId startNode = 0;     // the point where the rotation vector starts
    base::EntityId endNode = 0;       // the point where it ends
    double magnitude = 0.0;           // the angle in degrees, never zero
};

} // namespace oor::model
