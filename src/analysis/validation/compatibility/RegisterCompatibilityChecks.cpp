#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"

namespace oor::analysis
{

void registerCompatibilityChecks(CheckRegistry& registry)
{
    addShellPropertyTypeCheck(registry);
    addSolidPropertyTypeCheck(registry);
    addRodPropertyTypeCheck(registry);
    addBarPropertyTypeCheck(registry);
    addBeamPropertyTypeCheck(registry);
    addDiscretePropertyTypeCheck(registry);
    addShellMaterialTypeCheck(registry);
    addPlyMaterialTypeCheck(registry);
    addSolidMaterialTypeCheck(registry);
    addLineMaterialTypeCheck(registry);
    addFluidSolidMaterialCheck(registry);
    addHeatMaterialStructuralCheck(registry);
    addStructuralMaterialThermalCheck(registry);
    addThermalConductivityMissingCheck(registry);
    addIsoMatPoissonCheck(registry);
    addIsoMatModulusCheck(registry);
    addIsoMatShearModulusCheck(registry);
    addOrthoMat2DPoissonCheck(registry);
    addShellThicknessCheck(registry);
    addPlyThicknessCheck(registry);
    addLaminateThicknessCheck(registry);
    addZeroSectionAreaCheck(registry);
    addDensityMissingCheck(registry);
    addNoMassInDynamicsCheck(registry);
    addZeroLengthElementCheck(registry);
    addZeroAreaElementCheck(registry);
    addZeroVolumeElementCheck(registry);
    addSurfaceSideCheck(registry);
    addForcedConvectionSurfaceTypeCheck(registry);
    addSurfacePropertyCheck(registry);
    addAcousticPropertyTypeCheck(registry);
    addSpecialPropertyTypeCheck(registry);
    addCompanionPropertyTypeCheck(registry);
    addDummyEntryCheck(registry);
    addFluidMassElemListElementTypeCheck(registry);
    addAxisymmetricFlagCardCheck(registry);
    addAxisymmetricPointKindCheck(registry);
    addModuleMoveIdRepeatedCheck(registry);
    addModuleMoveOrderCheck(registry);
    addMultiTableIdRepeatedCheck(registry);
    addCosimulationGroupCheck(registry);
    addDistributionPropertyTypeCheck(registry);
    addFatigueEventIdRepeatedCheck(registry);
    addFrfComponentNameCheck(registry);
    addNonlinearBushForceTablesKindCheck(registry);
    addMonitorNameCheck(registry);
    addBushProp2DFreqPropertyTypeCheck(registry);
    addRigidAxisymConnectionPointsCheck(registry);
    addRotorIdRepeatedCheck(registry);
    addTiltingPadBearingPropServiceCheck(registry);
    addTrimRegionOrderCheck(registry);
    addWettedElementIdRepeatedCheck(registry);
    addWettedElementTypeCheck(registry);
}

} // namespace oor::analysis
