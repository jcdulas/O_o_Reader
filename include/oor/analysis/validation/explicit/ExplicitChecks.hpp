#pragma once

#include "oor/analysis/validation/engine/CheckRegistry.hpp"

// The explicit solver family (SOL 700): what the cards of the Euler domain, the coupling, the airbags and the material
// models say that the reference checks do not: IDs that two kinds of card of one category share (the format reference
// wants the number of an equation of state unique among all the EOSxxx cards), a card that names the wrong kind of
// card, a MESH that the cards around it contradict. One class per check, as in the other families.

namespace oor::analysis
{

// An EOSxxx card with the EID of an equation of state of another card.
void addEosIdRepeatedCheck(CheckRegistry& registry);
// A SHRxxx card with the SID of a shear model of another card.
void addShearModelIdRepeatedCheck(CheckRegistry& registry);
// A YLDxxx card with the YID of a yield model of another card.
void addYieldModelIdRepeatedCheck(CheckRegistry& registry);
// A FAILxxx card with the FID of a failure model of another card.
void addFailureModelIdRepeatedCheck(CheckRegistry& registry);

// The PID of a MESH names a property that is not a PEULER or a PEULER1.
void addEulerMeshPropertyKindCheck(CheckRegistry& registry);
// A MESH of TYPE ADAPT that no COUPLE names in its MESHID.
void addEulerMeshAdaptWithoutCouplingCheck(CheckRegistry& registry);
// The MID of a PEULER names a material that is not a MATDEUL.
void addEulerMaterialKindCheck(CheckRegistry& registry);
// The PORTYPE of a LEAKAGE does not name the kind of the porosity card its PORTYPID finds.
void addGasBagLeakagePorosityTypeCheck(CheckRegistry& registry);
// The INFTYPE of an ABINFL does not name the kind of the inflator card its INFTYPID finds.
void addInflatorTypeCheck(CheckRegistry& registry);
// A FLOWC whose FID2 is not the FID of a FLOWC that names it back.
void addEulerCyclicFlowBoundaryPairCheck(CheckRegistry& registry);

// A DYPARAM setting given twice with different values.
void addExplicitParamRepeatedCheck(CheckRegistry& registry);
void addGasBagSwitchSurfaceCheck(CheckRegistry& registry);
void addGasBagHeatTransferTypeCheck(CheckRegistry& registry);
void addInitialConditionRegionShapeTypeCheck(CheckRegistry& registry);

// Adds the checks of the family, in the order they run.
void registerExplicitChecks(CheckRegistry& registry);

} // namespace oor::analysis
