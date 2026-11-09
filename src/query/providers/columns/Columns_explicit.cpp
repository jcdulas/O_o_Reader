// The stored columns of the cards of the family "explicit", for the query engine (ColumnExport.hpp).
// One block a card type: its type index, then a getter a column.

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/beltProp/BeltPropTable.hpp"
#include "oor/model/cards/explicit/cohesiveFriction/CohesiveFrictionTable.hpp"
#include "oor/model/cards/explicit/coldGasInflator/ColdGasInflatorTable.hpp"
#include "oor/model/cards/explicit/constantSpallation/ConstantSpallationTable.hpp"
#include "oor/model/cards/explicit/couplingCohesiveFriction/CouplingCohesiveFrictionTable.hpp"
#include "oor/model/cards/explicit/couplingFailedSegmentFlow/CouplingFailedSegmentFlowTable.hpp"
#include "oor/model/cards/explicit/couplingHoleToCoupling/CouplingHoleToCouplingTable.hpp"
#include "oor/model/cards/explicit/couplingHoleToGasBag/CouplingHoleToGasBagTable.hpp"
#include "oor/model/cards/explicit/couplingInteraction/CouplingInteractionTable.hpp"
#include "oor/model/cards/explicit/couplingOptions/CouplingOptionsTable.hpp"
#include "oor/model/cards/explicit/cylinderShape/CylinderShapeTable.hpp"
#include "oor/model/cards/explicit/deflagrationStateEquation/DeflagrationStateEquationTable.hpp"
#include "oor/model/cards/explicit/elasticShearModel/ElasticShearModelTable.hpp"
#include "oor/model/cards/explicit/eulerBarrier/EulerBarrierTable.hpp"
#include "oor/model/cards/explicit/eulerBodyForce/EulerBodyForceTable.hpp"
#include "oor/model/cards/explicit/eulerBodyForceSet/EulerBodyForceSetTable.hpp"
#include "oor/model/cards/explicit/eulerCyclicFlowBoundary/EulerCyclicFlowBoundaryTable.hpp"
#include "oor/model/cards/explicit/eulerDefaultFlowBoundary/EulerDefaultFlowBoundaryTable.hpp"
#include "oor/model/cards/explicit/eulerFlowBoundary/EulerFlowBoundaryTable.hpp"
#include "oor/model/cards/explicit/eulerForceRegion/EulerForceRegionTable.hpp"
#include "oor/model/cards/explicit/eulerHydrostaticPreset/EulerHydrostaticPresetTable.hpp"
#include "oor/model/cards/explicit/eulerInitialRegions/EulerInitialRegionsTable.hpp"
#include "oor/model/cards/explicit/eulerLagrangeCoupling/EulerLagrangeCouplingTable.hpp"
#include "oor/model/cards/explicit/eulerMeshBias/EulerMeshBiasTable.hpp"
#include "oor/model/cards/explicit/eulerMeshGenerator/EulerMeshGeneratorTable.hpp"
#include "oor/model/cards/explicit/eulerProp/EulerPropTable.hpp"
#include "oor/model/cards/explicit/eulerRegionProp/EulerRegionPropTable.hpp"
#include "oor/model/cards/explicit/eulerTimeFlowBoundary/EulerTimeFlowBoundaryTable.hpp"
#include "oor/model/cards/explicit/eulerUserFlowBoundary/EulerUserFlowBoundaryTable.hpp"
#include "oor/model/cards/explicit/explicitParam/ExplicitParamCatalogue.hpp"
#include "oor/model/cards/explicit/explicitParam/ExplicitParamTable.hpp"
#include "oor/model/cards/explicit/explicitParam/ExplicitParamValues.hpp"
#include "oor/model/cards/explicit/fluidFilledContainer/FluidFilledContainerTable.hpp"
#include "oor/model/cards/explicit/fluidStructureSwitch/FluidStructureSwitchTable.hpp"
#include "oor/model/cards/explicit/gammaLawStateEquation/GammaLawStateEquationTable.hpp"
#include "oor/model/cards/explicit/gasBag/GasBagTable.hpp"
#include "oor/model/cards/explicit/gasBagConvection/GasBagConvectionTable.hpp"
#include "oor/model/cards/explicit/gasBagHeatTransfer/GasBagHeatTransferTable.hpp"
#include "oor/model/cards/explicit/gasBagLeakage/GasBagLeakageTable.hpp"
#include "oor/model/cards/explicit/gasBagPermeableLink/GasBagPermeableLinkTable.hpp"
#include "oor/model/cards/explicit/gasBagRadiation/GasBagRadiationTable.hpp"
#include "oor/model/cards/explicit/gasBagSwitch/GasBagSwitchTable.hpp"
#include "oor/model/cards/explicit/gasBagVentHole/GasBagVentHoleTable.hpp"
#include "oor/model/cards/explicit/hourglassSuppression/HourglassSuppressionTable.hpp"
#include "oor/model/cards/explicit/hybridInflator/HybridInflatorTable.hpp"
#include "oor/model/cards/explicit/hybridInflatorFractions/HybridInflatorFractionsTable.hpp"
#include "oor/model/cards/explicit/hydrodynamicYield/HydrodynamicYieldTable.hpp"
#include "oor/model/cards/explicit/ignitionGrowthStateEquation/IgnitionGrowthStateEquationTable.hpp"
#include "oor/model/cards/explicit/inflatorGas/InflatorGasTable.hpp"
#include "oor/model/cards/explicit/initialConditionRegion/InitialConditionRegionTable.hpp"
#include "oor/model/cards/explicit/initialConditionValues/InitialConditionValuesTable.hpp"
#include "oor/model/cards/explicit/initialGasComposition/InitialGasCompositionTable.hpp"
#include "oor/model/cards/explicit/initialSurfaceVolume/InitialSurfaceVolumeTable.hpp"
#include "oor/model/cards/explicit/johnsonCookFailure/JohnsonCookFailureTable.hpp"
#include "oor/model/cards/explicit/johnsonCookYield/JohnsonCookYieldTable.hpp"
#include "oor/model/cards/explicit/jonesWilkinsLeeStateEquation/JonesWilkinsLeeStateEquationTable.hpp"
#include "oor/model/cards/explicit/markerNode/MarkerNodeTable.hpp"
#include "oor/model/cards/explicit/markerBody/MarkerBodyTable.hpp"
#include "oor/model/cards/explicit/markerProp/MarkerPropTable.hpp"
#include "oor/model/cards/explicit/maxPlasticStrainFailure/MaxPlasticStrainFailureTable.hpp"
#include "oor/model/cards/explicit/mieGruneisenStateEquation/MieGruneisenStateEquationTable.hpp"
#include "oor/model/cards/explicit/mohrCoulombYield/MohrCoulombYieldTable.hpp"
#include "oor/model/cards/explicit/nobleAbelStateEquation/NobleAbelStateEquationTable.hpp"
#include "oor/model/cards/explicit/occupantAccelField/OccupantAccelFieldTable.hpp"
#include "oor/model/cards/explicit/occupantJoint/OccupantJointTable.hpp"
#include "oor/model/cards/explicit/occupantSegment/OccupantSegmentTable.hpp"
#include "oor/model/cards/explicit/polynomialShearModel/PolynomialShearModelTable.hpp"
#include "oor/model/cards/explicit/polynomialStateEquation/PolynomialStateEquationTable.hpp"
#include "oor/model/cards/explicit/polynomialYield/PolynomialYieldTable.hpp"
#include "oor/model/cards/explicit/porousFlowBoundary/PorousFlowBoundaryTable.hpp"
#include "oor/model/cards/explicit/porousHydrostaticBoundary/PorousHydrostaticBoundaryTable.hpp"
#include "oor/model/cards/explicit/porousTimeFlowBoundary/PorousTimeFlowBoundaryTable.hpp"
#include "oor/model/cards/explicit/porousUserBoundary/PorousUserBoundaryTable.hpp"
#include "oor/model/cards/explicit/ratePowerLawYield/RatePowerLawYieldTable.hpp"
#include "oor/model/cards/explicit/rigidBodyJoint/RigidBodyJointTable.hpp"
#include "oor/model/cards/explicit/rigidEllipsoidExternal/RigidEllipsoidExternalTable.hpp"
#include "oor/model/cards/explicit/rigidWall/RigidWallTable.hpp"
#include "oor/model/cards/explicit/snowMultiSurfaceYield/SnowMultiSurfaceYieldTable.hpp"
#include "oor/model/cards/explicit/sphereShape/SphereShapeTable.hpp"
#include "oor/model/cards/explicit/sphericalDetonation/SphericalDetonationTable.hpp"
#include "oor/model/cards/explicit/spotweldJoinPairs/SpotweldJoinPairsTable.hpp"
#include "oor/model/cards/explicit/standardInflator/StandardInflatorTable.hpp"
#include "oor/model/cards/explicit/steinbergGuinanYield/SteinbergGuinanYieldTable.hpp"
#include "oor/model/cards/explicit/surfacePermeability/SurfacePermeabilityTable.hpp"
#include "oor/model/cards/explicit/tanimuraMimuraYield/TanimuraMimuraYieldTable.hpp"
#include "oor/model/cards/explicit/tankTestInflator/TankTestInflatorTable.hpp"
#include "oor/model/cards/explicit/userEnforcedVelocity/UserEnforcedVelocityTable.hpp"
#include "oor/model/cards/explicit/userFailureModel/UserFailureModelTable.hpp"
#include "oor/model/cards/explicit/userServiceData/UserServiceDataTable.hpp"
#include "oor/model/cards/explicit/userShearModel/UserShearModelTable.hpp"
#include "oor/model/cards/explicit/userStateEquation/UserStateEquationTable.hpp"
#include "oor/model/cards/explicit/userStateVariables/UserStateVariablesTable.hpp"
#include "oor/model/cards/explicit/userYield/UserYieldTable.hpp"
#include "oor/model/cards/explicit/vonMisesYield/VonMisesYieldTable.hpp"
#include "oor/model/cards/explicit/zerilliArmstrongYield/ZerilliArmstrongYieldTable.hpp"
#include "oor/query/providers/ColumnExport.hpp"

namespace oor::query::column_export
{

void registerExplicitColumns(StoredColumnCatalogue& catalogue)
{
    {
        using model::OccupantAccelFieldTable;
        const std::uint16_t type = model::typeIndexOf<OccupantAccelFieldTable>();
        registerColumn(catalogue, type, "setId", [](const auto& model)
                       { return exportColumn(model->template table<OccupantAccelFieldTable>().setIdColumn()); });
    }
    {
        using model::OccupantJointTable;
        const std::uint16_t type = model::typeIndexOf<OccupantJointTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<OccupantJointTable>().idColumn()); });
    }
    {
        using model::OccupantSegmentTable;
        const std::uint16_t type = model::typeIndexOf<OccupantSegmentTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<OccupantSegmentTable>().idColumn()); });
    }
    {
        using model::EulerBarrierTable;
        const std::uint16_t type = model::typeIndexOf<EulerBarrierTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<EulerBarrierTable>().idColumn()); });
    }
    {
        using model::EulerMeshBiasTable;
        const std::uint16_t type = model::typeIndexOf<EulerMeshBiasTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<EulerMeshBiasTable>().idColumn()); });
    }
    {
        using model::SpotweldJoinPairsTable;
        const std::uint16_t type = model::typeIndexOf<SpotweldJoinPairsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SpotweldJoinPairsTable>().idColumn()); });
    }
    {
        using model::MarkerBodyTable;
        const std::uint16_t type = model::typeIndexOf<MarkerBodyTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<MarkerBodyTable>().idColumn()); });
    }
    {
        using model::MarkerNodeTable;
        const std::uint16_t type = model::typeIndexOf<MarkerNodeTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<MarkerNodeTable>().idColumn()); });
    }
    {
        using model::CohesiveFrictionTable;
        const std::uint16_t type = model::typeIndexOf<CohesiveFrictionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CohesiveFrictionTable>().idColumn()); });
    }
    {
        using model::CouplingCohesiveFrictionTable;
        const std::uint16_t type = model::typeIndexOf<CouplingCohesiveFrictionTable>();
        registerColumn(catalogue, type, "setId", [](const auto& model)
                       { return exportColumn(model->template table<CouplingCohesiveFrictionTable>().setIdColumn()); });
    }
    {
        using model::CouplingOptionsTable;
        const std::uint16_t type = model::typeIndexOf<CouplingOptionsTable>();
        registerColumn(catalogue, type, "setId", [](const auto& model)
                       { return exportColumn(model->template table<CouplingOptionsTable>().setIdColumn()); });
    }
    {
        using model::CouplingFailedSegmentFlowTable;
        const std::uint16_t type = model::typeIndexOf<CouplingFailedSegmentFlowTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CouplingFailedSegmentFlowTable>().idColumn()); });
    }
    {
        using model::CouplingInteractionTable;
        const std::uint16_t type = model::typeIndexOf<CouplingInteractionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CouplingInteractionTable>().idColumn()); });
    }
    {
        using model::EulerLagrangeCouplingTable;
        const std::uint16_t type = model::typeIndexOf<EulerLagrangeCouplingTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<EulerLagrangeCouplingTable>().idColumn()); });
    }
    {
        using model::CylinderShapeTable;
        const std::uint16_t type = model::typeIndexOf<CylinderShapeTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CylinderShapeTable>().idColumn()); });
    }
    {
        using model::SphericalDetonationTable;
        const std::uint16_t type = model::typeIndexOf<SphericalDetonationTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SphericalDetonationTable>().idColumn()); });
    }
    {
        using model::FluidStructureSwitchTable;
        const std::uint16_t type = model::typeIndexOf<FluidStructureSwitchTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FluidStructureSwitchTable>().idColumn()); });
    }
    {
        using model::ExplicitParamTable;
        const std::uint16_t type = model::typeIndexOf<ExplicitParamTable>();
        registerColumn(catalogue, type, "entryName", [](const auto& model)
                       { return exportTextColumn(model->template table<ExplicitParamTable>().entryNameColumn()); });
    }
    {
        using model::DeflagrationStateEquationTable;
        const std::uint16_t type = model::typeIndexOf<DeflagrationStateEquationTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DeflagrationStateEquationTable>().idColumn()); });
    }
    {
        using model::GammaLawStateEquationTable;
        const std::uint16_t type = model::typeIndexOf<GammaLawStateEquationTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<GammaLawStateEquationTable>().idColumn()); });
    }
    {
        using model::IgnitionGrowthStateEquationTable;
        const std::uint16_t type = model::typeIndexOf<IgnitionGrowthStateEquationTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<IgnitionGrowthStateEquationTable>().idColumn()); });
    }
    {
        using model::JonesWilkinsLeeStateEquationTable;
        const std::uint16_t type = model::typeIndexOf<JonesWilkinsLeeStateEquationTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<JonesWilkinsLeeStateEquationTable>().idColumn()); });
    }
    {
        using model::MieGruneisenStateEquationTable;
        const std::uint16_t type = model::typeIndexOf<MieGruneisenStateEquationTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<MieGruneisenStateEquationTable>().idColumn()); });
    }
    {
        using model::NobleAbelStateEquationTable;
        const std::uint16_t type = model::typeIndexOf<NobleAbelStateEquationTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NobleAbelStateEquationTable>().idColumn()); });
    }
    {
        using model::PolynomialStateEquationTable;
        const std::uint16_t type = model::typeIndexOf<PolynomialStateEquationTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PolynomialStateEquationTable>().idColumn()); });
    }
    {
        using model::UserStateEquationTable;
        const std::uint16_t type = model::typeIndexOf<UserStateEquationTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserStateEquationTable>().idColumn()); });
    }
    {
        using model::EulerBodyForceTable;
        const std::uint16_t type = model::typeIndexOf<EulerBodyForceTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<EulerBodyForceTable>().idColumn()); });
    }
    {
        using model::EulerBodyForceSetTable;
        const std::uint16_t type = model::typeIndexOf<EulerBodyForceSetTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<EulerBodyForceSetTable>().idColumn()); });
    }
    {
        using model::EulerForceRegionTable;
        const std::uint16_t type = model::typeIndexOf<EulerForceRegionTable>();
        registerColumn(catalogue, type, "setId", [](const auto& model)
                       { return exportColumn(model->template table<EulerForceRegionTable>().setIdColumn()); });
    }
    {
        using model::JohnsonCookFailureTable;
        const std::uint16_t type = model::typeIndexOf<JohnsonCookFailureTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<JohnsonCookFailureTable>().idColumn()); });
    }
    {
        using model::MaxPlasticStrainFailureTable;
        const std::uint16_t type = model::typeIndexOf<MaxPlasticStrainFailureTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<MaxPlasticStrainFailureTable>().idColumn()); });
    }
    {
        using model::UserFailureModelTable;
        const std::uint16_t type = model::typeIndexOf<UserFailureModelTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserFailureModelTable>().idColumn()); });
    }
    {
        using model::FluidFilledContainerTable;
        const std::uint16_t type = model::typeIndexOf<FluidFilledContainerTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FluidFilledContainerTable>().idColumn()); });
    }
    {
        using model::EulerFlowBoundaryTable;
        const std::uint16_t type = model::typeIndexOf<EulerFlowBoundaryTable>();
        registerColumn(catalogue, type, "setId", [](const auto& model)
                       { return exportColumn(model->template table<EulerFlowBoundaryTable>().setIdColumn()); });
    }
    {
        using model::EulerCyclicFlowBoundaryTable;
        const std::uint16_t type = model::typeIndexOf<EulerCyclicFlowBoundaryTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<EulerCyclicFlowBoundaryTable>().idColumn()); });
    }
    {
        using model::EulerDefaultFlowBoundaryTable;
        const std::uint16_t type = model::typeIndexOf<EulerDefaultFlowBoundaryTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<EulerDefaultFlowBoundaryTable>().idColumn()); });
    }
    {
        using model::EulerTimeFlowBoundaryTable;
        const std::uint16_t type = model::typeIndexOf<EulerTimeFlowBoundaryTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<EulerTimeFlowBoundaryTable>().idColumn()); });
    }
    {
        using model::EulerUserFlowBoundaryTable;
        const std::uint16_t type = model::typeIndexOf<EulerUserFlowBoundaryTable>();
        registerColumn(catalogue, type, "setId", [](const auto& model)
                       { return exportColumn(model->template table<EulerUserFlowBoundaryTable>().setIdColumn()); });
    }
    {
        using model::UserEnforcedVelocityTable;
        const std::uint16_t type = model::typeIndexOf<UserEnforcedVelocityTable>();
        registerColumn(catalogue, type, "setId", [](const auto& model)
                       { return exportColumn(model->template table<UserEnforcedVelocityTable>().setIdColumn()); });
    }
    {
        using model::GasBagTable;
        const std::uint16_t type = model::typeIndexOf<GasBagTable>();
        registerColumn(catalogue, type, "id",
                       [](const auto& model) { return exportColumn(model->template table<GasBagTable>().idColumn()); });
    }
    {
        using model::GasBagSwitchTable;
        const std::uint16_t type = model::typeIndexOf<GasBagSwitchTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<GasBagSwitchTable>().idColumn()); });
    }
    {
        using model::UserServiceDataTable;
        const std::uint16_t type = model::typeIndexOf<UserServiceDataTable>();
        registerColumn(catalogue, type, "serviceId", [](const auto& model)
                       { return exportTextColumn(model->template table<UserServiceDataTable>().serviceIdColumn()); });
    }
    {
        using model::GasBagHeatTransferTable;
        const std::uint16_t type = model::typeIndexOf<GasBagHeatTransferTable>();
        registerColumn(catalogue, type, "setId", [](const auto& model)
                       { return exportColumn(model->template table<GasBagHeatTransferTable>().setIdColumn()); });
    }
    {
        using model::HourglassSuppressionTable;
        const std::uint16_t type = model::typeIndexOf<HourglassSuppressionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HourglassSuppressionTable>().idColumn()); });
    }
    {
        using model::GasBagConvectionTable;
        const std::uint16_t type = model::typeIndexOf<GasBagConvectionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<GasBagConvectionTable>().idColumn()); });
    }
    {
        using model::GasBagRadiationTable;
        const std::uint16_t type = model::typeIndexOf<GasBagRadiationTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<GasBagRadiationTable>().idColumn()); });
    }
    {
        using model::EulerHydrostaticPresetTable;
        const std::uint16_t type = model::typeIndexOf<EulerHydrostaticPresetTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<EulerHydrostaticPresetTable>().idColumn()); });
    }
    {
        using model::ColdGasInflatorTable;
        const std::uint16_t type = model::typeIndexOf<ColdGasInflatorTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ColdGasInflatorTable>().idColumn()); });
    }
    {
        using model::HybridInflatorFractionsTable;
        const std::uint16_t type = model::typeIndexOf<HybridInflatorFractionsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HybridInflatorFractionsTable>().idColumn()); });
    }
    {
        using model::InflatorGasTable;
        const std::uint16_t type = model::typeIndexOf<InflatorGasTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<InflatorGasTable>().idColumn()); });
    }
    {
        using model::HybridInflatorTable;
        const std::uint16_t type = model::typeIndexOf<HybridInflatorTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HybridInflatorTable>().idColumn()); });
    }
    {
        using model::TankTestInflatorTable;
        const std::uint16_t type = model::typeIndexOf<TankTestInflatorTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TankTestInflatorTable>().idColumn()); });
    }
    {
        using model::StandardInflatorTable;
        const std::uint16_t type = model::typeIndexOf<StandardInflatorTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<StandardInflatorTable>().idColumn()); });
    }
    {
        using model::InitialGasCompositionTable;
        const std::uint16_t type = model::typeIndexOf<InitialGasCompositionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<InitialGasCompositionTable>().idColumn()); });
    }
    {
        using model::GasBagLeakageTable;
        const std::uint16_t type = model::typeIndexOf<GasBagLeakageTable>();
        registerColumn(catalogue, type, "setId", [](const auto& model)
                       { return exportColumn(model->template table<GasBagLeakageTable>().setIdColumn()); });
    }
    {
        using model::EulerMeshGeneratorTable;
        const std::uint16_t type = model::typeIndexOf<EulerMeshGeneratorTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<EulerMeshGeneratorTable>().idColumn()); });
    }
    {
        using model::BeltPropTable;
        const std::uint16_t type = model::typeIndexOf<BeltPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BeltPropTable>().idColumn()); });
    }
    {
        using model::SurfacePermeabilityTable;
        const std::uint16_t type = model::typeIndexOf<SurfacePermeabilityTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SurfacePermeabilityTable>().idColumn()); });
    }
    {
        using model::GasBagPermeableLinkTable;
        const std::uint16_t type = model::typeIndexOf<GasBagPermeableLinkTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<GasBagPermeableLinkTable>().idColumn()); });
    }
    {
        using model::EulerPropTable;
        const std::uint16_t type = model::typeIndexOf<EulerPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<EulerPropTable>().idColumn()); });
    }
    {
        using model::EulerRegionPropTable;
        const std::uint16_t type = model::typeIndexOf<EulerRegionPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<EulerRegionPropTable>().idColumn()); });
    }
    {
        using model::MarkerPropTable;
        const std::uint16_t type = model::typeIndexOf<MarkerPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<MarkerPropTable>().idColumn()); });
    }
    {
        using model::ConstantSpallationTable;
        const std::uint16_t type = model::typeIndexOf<ConstantSpallationTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ConstantSpallationTable>().idColumn()); });
    }
    {
        using model::CouplingHoleToCouplingTable;
        const std::uint16_t type = model::typeIndexOf<CouplingHoleToCouplingTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CouplingHoleToCouplingTable>().idColumn()); });
    }
    {
        using model::CouplingHoleToGasBagTable;
        const std::uint16_t type = model::typeIndexOf<CouplingHoleToGasBagTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CouplingHoleToGasBagTable>().idColumn()); });
    }
    {
        using model::PorousFlowBoundaryTable;
        const std::uint16_t type = model::typeIndexOf<PorousFlowBoundaryTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PorousFlowBoundaryTable>().idColumn()); });
    }
    {
        using model::PorousTimeFlowBoundaryTable;
        const std::uint16_t type = model::typeIndexOf<PorousTimeFlowBoundaryTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PorousTimeFlowBoundaryTable>().idColumn()); });
    }
    {
        using model::GasBagVentHoleTable;
        const std::uint16_t type = model::typeIndexOf<GasBagVentHoleTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<GasBagVentHoleTable>().idColumn()); });
    }
    {
        using model::PorousHydrostaticBoundaryTable;
        const std::uint16_t type = model::typeIndexOf<PorousHydrostaticBoundaryTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PorousHydrostaticBoundaryTable>().idColumn()); });
    }
    {
        using model::PorousUserBoundaryTable;
        const std::uint16_t type = model::typeIndexOf<PorousUserBoundaryTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PorousUserBoundaryTable>().idColumn()); });
    }
    {
        using model::RigidBodyJointTable;
        const std::uint16_t type = model::typeIndexOf<RigidBodyJointTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RigidBodyJointTable>().idColumn()); });
    }
    {
        using model::RigidEllipsoidExternalTable;
        const std::uint16_t type = model::typeIndexOf<RigidEllipsoidExternalTable>();
        registerColumn(catalogue, type, "name", [](const auto& model)
                       { return exportTextColumn(model->template table<RigidEllipsoidExternalTable>().nameColumn()); });
    }
    {
        using model::ElasticShearModelTable;
        const std::uint16_t type = model::typeIndexOf<ElasticShearModelTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ElasticShearModelTable>().idColumn()); });
    }
    {
        using model::PolynomialShearModelTable;
        const std::uint16_t type = model::typeIndexOf<PolynomialShearModelTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PolynomialShearModelTable>().idColumn()); });
    }
    {
        using model::UserShearModelTable;
        const std::uint16_t type = model::typeIndexOf<UserShearModelTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserShearModelTable>().idColumn()); });
    }
    {
        using model::SphereShapeTable;
        const std::uint16_t type = model::typeIndexOf<SphereShapeTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SphereShapeTable>().idColumn()); });
    }
    {
        using model::InitialSurfaceVolumeTable;
        const std::uint16_t type = model::typeIndexOf<InitialSurfaceVolumeTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<InitialSurfaceVolumeTable>().idColumn()); });
    }
    {
        using model::EulerInitialRegionsTable;
        const std::uint16_t type = model::typeIndexOf<EulerInitialRegionsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<EulerInitialRegionsTable>().idColumn()); });
    }
    {
        using model::InitialConditionRegionTable;
        const std::uint16_t type = model::typeIndexOf<InitialConditionRegionTable>();
        registerColumn(catalogue, type, "groupId", [](const auto& model)
                       { return exportColumn(model->template table<InitialConditionRegionTable>().groupIdColumn()); });
    }
    {
        using model::InitialConditionValuesTable;
        const std::uint16_t type = model::typeIndexOf<InitialConditionValuesTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<InitialConditionValuesTable>().idColumn()); });
    }
    {
        using model::UserStateVariablesTable;
        const std::uint16_t type = model::typeIndexOf<UserStateVariablesTable>();
        registerColumn(
            catalogue, type, "variableCount", [](const auto& model)
            { return exportColumn(model->template table<UserStateVariablesTable>().variableCountColumn()); });
    }
    {
        using model::RigidWallTable;
        const std::uint16_t type = model::typeIndexOf<RigidWallTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RigidWallTable>().idColumn()); });
    }
    {
        using model::HydrodynamicYieldTable;
        const std::uint16_t type = model::typeIndexOf<HydrodynamicYieldTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HydrodynamicYieldTable>().idColumn()); });
    }
    {
        using model::JohnsonCookYieldTable;
        const std::uint16_t type = model::typeIndexOf<JohnsonCookYieldTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<JohnsonCookYieldTable>().idColumn()); });
    }
    {
        using model::MohrCoulombYieldTable;
        const std::uint16_t type = model::typeIndexOf<MohrCoulombYieldTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<MohrCoulombYieldTable>().idColumn()); });
    }
    {
        using model::SnowMultiSurfaceYieldTable;
        const std::uint16_t type = model::typeIndexOf<SnowMultiSurfaceYieldTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SnowMultiSurfaceYieldTable>().idColumn()); });
    }
    {
        using model::PolynomialYieldTable;
        const std::uint16_t type = model::typeIndexOf<PolynomialYieldTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PolynomialYieldTable>().idColumn()); });
    }
    {
        using model::RatePowerLawYieldTable;
        const std::uint16_t type = model::typeIndexOf<RatePowerLawYieldTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RatePowerLawYieldTable>().idColumn()); });
    }
    {
        using model::SteinbergGuinanYieldTable;
        const std::uint16_t type = model::typeIndexOf<SteinbergGuinanYieldTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SteinbergGuinanYieldTable>().idColumn()); });
    }
    {
        using model::TanimuraMimuraYieldTable;
        const std::uint16_t type = model::typeIndexOf<TanimuraMimuraYieldTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TanimuraMimuraYieldTable>().idColumn()); });
    }
    {
        using model::UserYieldTable;
        const std::uint16_t type = model::typeIndexOf<UserYieldTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserYieldTable>().idColumn()); });
    }
    {
        using model::VonMisesYieldTable;
        const std::uint16_t type = model::typeIndexOf<VonMisesYieldTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<VonMisesYieldTable>().idColumn()); });
    }
    {
        using model::ZerilliArmstrongYieldTable;
        const std::uint16_t type = model::typeIndexOf<ZerilliArmstrongYieldTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ZerilliArmstrongYieldTable>().idColumn()); });
    }
}

} // namespace oor::query::column_export
