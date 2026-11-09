#pragma once

#include "oor/analysis/validation/engine/CheckRegistry.hpp"

// Family 3: element, property and material compatibility. Every check is one class in one file of
// src/analysis/validation/compatibility/, with the function that adds it to the registry.

namespace oor::analysis
{

void addShellPropertyTypeCheck(CheckRegistry& registry);
void addSolidPropertyTypeCheck(CheckRegistry& registry);
void addRodPropertyTypeCheck(CheckRegistry& registry);
void addBarPropertyTypeCheck(CheckRegistry& registry);
void addBeamPropertyTypeCheck(CheckRegistry& registry);
void addDiscretePropertyTypeCheck(CheckRegistry& registry);
void addShellMaterialTypeCheck(CheckRegistry& registry);
void addPlyMaterialTypeCheck(CheckRegistry& registry);
void addSolidMaterialTypeCheck(CheckRegistry& registry);
void addLineMaterialTypeCheck(CheckRegistry& registry);
void addFluidSolidMaterialCheck(CheckRegistry& registry);
void addHeatMaterialStructuralCheck(CheckRegistry& registry);
void addStructuralMaterialThermalCheck(CheckRegistry& registry);
void addThermalConductivityMissingCheck(CheckRegistry& registry);
void addIsoMatPoissonCheck(CheckRegistry& registry);
void addIsoMatModulusCheck(CheckRegistry& registry);
void addIsoMatShearModulusCheck(CheckRegistry& registry);
void addOrthoMat2DPoissonCheck(CheckRegistry& registry);
void addShellThicknessCheck(CheckRegistry& registry);
void addPlyThicknessCheck(CheckRegistry& registry);
void addLaminateThicknessCheck(CheckRegistry& registry);
void addZeroSectionAreaCheck(CheckRegistry& registry);
void addDensityMissingCheck(CheckRegistry& registry);
void addNoMassInDynamicsCheck(CheckRegistry& registry);
void addZeroLengthElementCheck(CheckRegistry& registry);
void addZeroAreaElementCheck(CheckRegistry& registry);
void addZeroVolumeElementCheck(CheckRegistry& registry);
void addSurfaceSideCheck(CheckRegistry& registry);
void addForcedConvectionSurfaceTypeCheck(CheckRegistry& registry);
void addSurfacePropertyCheck(CheckRegistry& registry);
void addAcousticPropertyTypeCheck(CheckRegistry& registry);
void addSpecialPropertyTypeCheck(CheckRegistry& registry);
void addCompanionPropertyTypeCheck(CheckRegistry& registry);
void addDummyEntryCheck(CheckRegistry& registry);
void addFluidMassElemListElementTypeCheck(CheckRegistry& registry);
void addAxisymmetricFlagCardCheck(CheckRegistry& registry);
void addAxisymmetricPointKindCheck(CheckRegistry& registry);
void addModuleMoveIdRepeatedCheck(CheckRegistry& registry);
void addModuleMoveOrderCheck(CheckRegistry& registry);
void addMultiTableIdRepeatedCheck(CheckRegistry& registry);
void addCosimulationGroupCheck(CheckRegistry& registry);
void addDistributionPropertyTypeCheck(CheckRegistry& registry);
void addFatigueEventIdRepeatedCheck(CheckRegistry& registry);
void addFrfComponentNameCheck(CheckRegistry& registry);
void addNonlinearBushForceTablesKindCheck(CheckRegistry& registry);
void addMonitorNameCheck(CheckRegistry& registry);
void addBushProp2DFreqPropertyTypeCheck(CheckRegistry& registry);
void addRigidAxisymConnectionPointsCheck(CheckRegistry& registry);
void addRotorIdRepeatedCheck(CheckRegistry& registry);
void addTiltingPadBearingPropServiceCheck(CheckRegistry& registry);
void addTrimRegionOrderCheck(CheckRegistry& registry);
void addWettedElementIdRepeatedCheck(CheckRegistry& registry);
void addWettedElementTypeCheck(CheckRegistry& registry);

// Adds the checks of the family, in the order they run.
void registerCompatibilityChecks(CheckRegistry& registry);

} // namespace oor::analysis
