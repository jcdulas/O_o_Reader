#pragma once

#include "oor/analysis/validation/engine/CheckRegistry.hpp"

// Family 9: superelements. The structure of the deck (the parts opened by BEGIN SUPER, BEGIN AUXMODEL, ...) and the
// rules of the superelement cards that the format reference states (SESET, SEBULK, SECONCT, SETREE, ...): which cards
// belong in the main Bulk Data, the points of a list, the tree, the mirror plane, the manual search of the boundary.

namespace oor::analysis
{

void addDuplicatePartCheck(CheckRegistry& registry);
void addEmptyPartCheck(CheckRegistry& registry);
void addCardInPartCheck(CheckRegistry& registry);
void addSuperelementExternalBoundaryInMainCheck(CheckRegistry& registry);
void addSuperelementInteriorNodesMissingNodeCheck(CheckRegistry& registry);
void addNodeInTwoCheck(CheckRegistry& registry);
void addTreeCycleCheck(CheckRegistry& registry);
void addTreeUpstreamTwiceCheck(CheckRegistry& registry);
void addMirrorPlaneCheck(CheckRegistry& registry);
void addManualWithoutConnectionCheck(CheckRegistry& registry);
void addManualSearchCardsCheck(CheckRegistry& registry);
void addMassIdRepeatedCheck(CheckRegistry& registry);
void addAxisymConeSectorRingCheck(CheckRegistry& registry);

// Adds the checks of the family, in the order they run.
void registerSuperelementChecks(CheckRegistry& registry);

} // namespace oor::analysis
