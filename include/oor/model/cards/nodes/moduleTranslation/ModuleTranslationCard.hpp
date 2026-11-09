#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One MDTRAN card: a translation of a module, given as a vector between two points of the main Bulk Data. The move
// takes effect only when an MDMOVE names its MVID (an MDTRAN nobody names moves nothing).
//
// Card layout: MDTRAN  MVID  IDS  IDE  MAG            fields 1 to 4
// IDS and IDE are the GRID or POINT IDs of the start and the end of the vector, both in module 0. MAG is the length of
// the translation: a blank or a zero means the length of the vector itself, so the field is a real with the default 0.0
// and the card keeps the zero (a MAG of zero is the default, not "no move"). The format reference puts the card in the
// main Bulk Data only (a fatal error after BEGIN MODULE): a rule of the deck, which the reader leaves to the checks.
// The card refuses an IDS and an IDE that are the same point: the format reference does not say it, but a vector
// between a point and itself has no direction.
//
// The MVID is a number of the move kinds (MDTRAN, MDROT1, MDROT2, MDMIR1, MDMIR2), which an MDMOVE names: one number
// space for the five kinds, IdSpace::PerCardType in the category ModuleMove.
struct ModuleTranslationCard
{
    base::EntityId id = 0;        // the move, referenced by an MDMOVE
    base::EntityId startNode = 0; // the point where the translation vector starts
    base::EntityId endNode = 0;   // the point where it ends
    double magnitude = 0.0;       // the magnitude; 0.0 means the length of the vector
};

} // namespace oor::model
