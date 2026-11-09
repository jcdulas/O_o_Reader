#include "oor/analysis/validation/superelement/SuperelementChecks.hpp"

namespace oor::analysis
{

// The checks that need the geometry of the parts (an SECONCT whose points do not match within the tolerance) or the
// solver's log of the reduction are not registered here.
void registerSuperelementChecks(CheckRegistry& registry)
{
    addDuplicatePartCheck(registry);
    addEmptyPartCheck(registry);
    addCardInPartCheck(registry);
    addSuperelementExternalBoundaryInMainCheck(registry);
    addSuperelementInteriorNodesMissingNodeCheck(registry);
    addNodeInTwoCheck(registry);
    addTreeCycleCheck(registry);
    addTreeUpstreamTwiceCheck(registry);
    addMirrorPlaneCheck(registry);
    addManualWithoutConnectionCheck(registry);
    addManualSearchCardsCheck(registry);
    addMassIdRepeatedCheck(registry);
    addAxisymConeSectorRingCheck(registry);
}

} // namespace oor::analysis
