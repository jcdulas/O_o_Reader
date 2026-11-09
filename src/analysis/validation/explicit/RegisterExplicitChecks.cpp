#include "oor/analysis/validation/explicit/ExplicitChecks.hpp"

namespace oor::analysis
{

void registerExplicitChecks(CheckRegistry& registry)
{
    addEosIdRepeatedCheck(registry);
    addShearModelIdRepeatedCheck(registry);
    addYieldModelIdRepeatedCheck(registry);
    addFailureModelIdRepeatedCheck(registry);
    addEulerMeshPropertyKindCheck(registry);
    addEulerMeshAdaptWithoutCouplingCheck(registry);
    addEulerMaterialKindCheck(registry);
    addGasBagLeakagePorosityTypeCheck(registry);
    addInflatorTypeCheck(registry);
    addEulerCyclicFlowBoundaryPairCheck(registry);
    addExplicitParamRepeatedCheck(registry);
    addGasBagSwitchSurfaceCheck(registry);
    addGasBagHeatTransferTypeCheck(registry);
    addInitialConditionRegionShapeTypeCheck(registry);
}

} // namespace oor::analysis
