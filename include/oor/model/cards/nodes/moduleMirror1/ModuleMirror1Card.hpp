#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One MDMIR1 card: a mirror image of a module in a plane given by three points of the main Bulk Data. The move takes
// effect only when an MDMOVE names its MVID, and then in its first MVID field (a mirror may not come
// after another move in the list, which a card cannot check: the MDMOVE does not know the kinds of the cards it names).
//
// Card layout: MDMIR1  MVID  P1  P2  P3            fields 1 to 4
// P1, P2 and P3 are GRID or POINT IDs of module 0 that are not collinear. The card can only see that the three IDs are
// different; whether the points are collinear needs their coordinates.
//
// The MVID is one number of the move kinds (see ModuleTranslationCard): IdSpace::PerCardType in the category
// ModuleMove.
struct ModuleMirror1Card
{
    base::EntityId id = 0; // the move, referenced by an MDMOVE
    base::EntityId point1 = 0;
    base::EntityId point2 = 0;
    base::EntityId point3 = 0;
};

} // namespace oor::model
