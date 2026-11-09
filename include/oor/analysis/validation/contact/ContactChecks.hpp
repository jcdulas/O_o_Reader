#pragma once

#include "oor/analysis/validation/engine/CheckRegistry.hpp"

// The contact family: what the cards of contact (BCTABLE, BCBODY, BCBODY1, BCSURF, BSURF, BCONP, BFRIC, ...) say that
// the reference checks do not: IDs that two kinds of card share, a body made of the wrong kind of surface, a card that
// the solution of the deck does not run. One class per check, as in the other families.

namespace oor::analysis
{

// Two contact bodies of different cards (BCBODY, BCBODY1, BCSURF, BCGRID) with the same BID.
void addBodyIdRepeatedCheck(CheckRegistry& registry);
// Two contact surfaces of different cards (BSURF, BCPROP, BCMATL, BCBOX, the rigid surfaces) with the same ID.
void addSurfaceIdRepeatedCheck(CheckRegistry& registry);
// A BCTABL1 and a BCTABLE with the same ID.
void addTableIdRepeatedCheck(CheckRegistry& registry);
// A deformable body whose BSID names a rigid surface.
void addBodySurfaceKindCheck(CheckRegistry& registry);
// A BCONP or a BFRIC in a deck whose solution is not the one of the slideline contact (SOL 106 and SOL 129).
void addSlidelineSolutionCheck(CheckRegistry& registry);
void addBrakeBodyKindCheck(CheckRegistry& registry);
void addBrakeSquealIdRepeatedCheck(CheckRegistry& registry);
// A pair of contact bodies that a contact table names and whose surfaces do not meet (ContactSearch).
void addPairApartCheck(CheckRegistry& registry);

// Adds the checks of the family, in the order they run.
void registerContactChecks(CheckRegistry& registry);

} // namespace oor::analysis
