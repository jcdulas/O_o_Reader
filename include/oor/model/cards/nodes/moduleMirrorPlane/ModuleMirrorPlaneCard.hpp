#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One MDMPLN card: a mirror image of a module (a primary or a secondary, copied one) in a plane given by three points
// of the main Bulk Data. It is the form of the mirror that names the module directly, where an MDMIR1 is a move that an
// MDMOVE lists (the two cards have the same three points).
//
// Card layout: MDMPLN  MODID  P1  P2  P3            fields 1 to 4
// MODID is the number of the module, a plain integer (a BEGIN MODULE section is not a card of the model). P1, P2 and P3
// are GRID or POINT IDs of module 0 that are not collinear: the card can only see that the three IDs differ. The format
// reference puts the card in the main Bulk Data only, a rule of the deck.
//
// One MDMPLN per module: the MODID is unique per card type (IdSpace::PerCardType, category Other).
struct ModuleMirrorPlaneCard
{
    base::EntityId id = 0; // the module that is mirrored
    base::EntityId point1 = 0;
    base::EntityId point2 = 0;
    base::EntityId point3 = 0;
};

} // namespace oor::model
