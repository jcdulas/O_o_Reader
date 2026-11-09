#pragma once

#include "oor/analysis/validation/engine/CheckRegistry.hpp"

// Family 5: loads. A load on a grid point or element that does not exist is reported by ref.unresolved; these are the
// loads that are not wrong about a reference but cannot act, act on the wrong kind of thing, or do nothing.

namespace oor::analysis
{

void addForceOnScalarPointCheck(CheckRegistry& registry);
void addZeroMagnitudeCheck(CheckRegistry& registry);
void addZeroGravityCheck(CheckRegistry& registry);
void addLoadCombinationCycleCheck(CheckRegistry& registry);
void addPressureOnUnsupportedElementCheck(CheckRegistry& registry);
void addFacePressureLoadSolidFaceCheck(CheckRegistry& registry);
void addShellPressureLoadNoElementCheck(CheckRegistry& registry);
void addUnusedLoadSetCheck(CheckRegistry& registry);
void addHeatZeroFluxCheck(CheckRegistry& registry);
void addTransientTableLoad1NoFunctionCheck(CheckRegistry& registry);
void addMultiPointConstrValueWithoutEquationCheck(CheckRegistry& registry);
void addFreeBodySubsystemCheck(CheckRegistry& registry);
void addHydrostaticSphericalSystemCheck(CheckRegistry& registry);
void addImperfectionGeometryUnitCheck(CheckRegistry& registry);

// Adds the checks of the family, in the order they run.
void registerLoadChecks(CheckRegistry& registry);

} // namespace oor::analysis
