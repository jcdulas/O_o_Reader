#include "oor/analysis/validation/loads/LoadChecks.hpp"

namespace oor::analysis
{

void registerLoadChecks(CheckRegistry& registry)
{
    addForceOnScalarPointCheck(registry);
    addZeroMagnitudeCheck(registry);
    addZeroGravityCheck(registry);
    addLoadCombinationCycleCheck(registry);
    addPressureOnUnsupportedElementCheck(registry);
    addFacePressureLoadSolidFaceCheck(registry);
    addShellPressureLoadNoElementCheck(registry);
    addHeatZeroFluxCheck(registry);
    addTransientTableLoad1NoFunctionCheck(registry);
    addUnusedLoadSetCheck(registry);
    addMultiPointConstrValueWithoutEquationCheck(registry);
    addFreeBodySubsystemCheck(registry);
    addHydrostaticSphericalSystemCheck(registry);
    addImperfectionGeometryUnitCheck(registry);
}

} // namespace oor::analysis
