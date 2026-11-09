// The stored columns of the cards of the family "thermal", for the query engine (ColumnExport.hpp).
// One block a card type: its type index, then a getter a column.

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"
#include "oor/model/cards/thermal/acousticAbsorberHexa/AcousticAbsorberHexaTable.hpp"
#include "oor/model/cards/thermal/acousticAbsorberBrickProp/AcousticAbsorberBrickPropTable.hpp"
#include "oor/model/cards/thermal/acousticAbsorber/AcousticAbsorberTable.hpp"
#include "oor/model/cards/thermal/acousticBarrierHexa/AcousticBarrierHexaTable.hpp"
#include "oor/model/cards/thermal/acousticBarrierProp/AcousticBarrierPropTable.hpp"
#include "oor/model/cards/thermal/acousticSlotBoundary/AcousticSlotBoundaryTable.hpp"
#include "oor/model/cards/thermal/axisymFluidBoundaryList/AxisymFluidBoundaryListTable.hpp"
#include "oor/model/cards/thermal/axisymFluid2/AxisymFluid2Table.hpp"
#include "oor/model/cards/thermal/axisymFluid3/AxisymFluid3Table.hpp"
#include "oor/model/cards/thermal/axisymFluid4/AxisymFluid4Table.hpp"
#include "oor/model/cards/thermal/axisymFluidFreeSurface/AxisymFluidFreeSurfaceTable.hpp"
#include "oor/model/cards/thermal/axisymFluidSymmetry/AxisymFluidSymmetryTable.hpp"
#include "oor/model/cards/thermal/fluidMassElemList/FluidMassElemListTable.hpp"
#include "oor/model/cards/thermal/fluidStructureInterface/FluidStructureInterfaceTable.hpp"
#include "oor/model/cards/thermal/fluidStructurePanels/FluidStructurePanelsTable.hpp"
#include "oor/model/cards/thermal/fluidVirtualMass/FluidVirtualMassTable.hpp"
#include "oor/model/cards/thermal/forcedConvection/ForcedConvectionTable.hpp"
#include "oor/model/cards/thermal/forcedConvectionProp/ForcedConvectionPropTable.hpp"
#include "oor/model/cards/thermal/freeConvection/FreeConvectionTable.hpp"
#include "oor/model/cards/thermal/freeConvectionProp/FreeConvectionPropTable.hpp"
#include "oor/model/cards/thermal/heatAnisoMat/HeatAnisoMatTable.hpp"
#include "oor/model/cards/thermal/heatController/HeatControllerTable.hpp"
#include "oor/model/cards/thermal/heatFluxBoundaryLoad3/HeatFluxBoundaryLoad3Table.hpp"
#include "oor/model/cards/thermal/heatFluxNodalLoad2/HeatFluxNodalLoad2Table.hpp"
#include "oor/model/cards/thermal/heatFluxNodeSetLoad/HeatFluxNodeSetLoadTable.hpp"
#include "oor/model/cards/thermal/heatFluxSurfaceLoad1/HeatFluxSurfaceLoad1Table.hpp"
#include "oor/model/cards/thermal/heatFluxVectorLoad/HeatFluxVectorLoadTable.hpp"
#include "oor/model/cards/thermal/heatIsoMat/HeatIsoMatTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceDefaults/HeatSurfaceDefaultsTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceNodes/HeatSurfaceNodesTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceProperty/HeatSurfacePropertyTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceSide/HeatSurfaceSideTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceProp/HeatSurfacePropTable.hpp"
#include "oor/model/cards/thermal/heatVolumeSourceLoad/HeatVolumeSourceLoadTable.hpp"
#include "oor/model/cards/thermal/radiationBands/RadiationBandsTable.hpp"
#include "oor/model/cards/thermal/radiationCavity/RadiationCavityTable.hpp"
#include "oor/model/cards/thermal/radiationCavityList/RadiationCavityListTable.hpp"
#include "oor/model/cards/thermal/radiationCavitySet/RadiationCavitySetTable.hpp"
#include "oor/model/cards/thermal/radiationExchangeMatrix/RadiationExchangeMatrixTable.hpp"
#include "oor/model/cards/thermal/radiationProp/RadiationPropTable.hpp"
#include "oor/model/cards/thermal/radiationPropTemperature/RadiationPropTemperatureTable.hpp"
#include "oor/model/cards/thermal/radiationToSpace/RadiationToSpaceTable.hpp"
#include "oor/model/cards/thermal/temperatureBoundary/TemperatureBoundaryTable.hpp"
#include "oor/model/cards/thermal/trimComponentCoupling/TrimComponentCouplingTable.hpp"
#include "oor/model/cards/thermal/trimComponentCoupling1D/TrimComponentCoupling1DTable.hpp"
#include "oor/model/cards/thermal/trimCouplingTolerances/TrimCouplingTolerancesTable.hpp"
#include "oor/model/cards/thermal/trimLayeredComponent/TrimLayeredComponentTable.hpp"
#include "oor/model/cards/thermal/viewFactorFace/ViewFactorFaceTable.hpp"
#include "oor/model/cards/thermal/viewFactorIntegration/ViewFactorIntegrationTable.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "oor/query/providers/ColumnExport.hpp"

namespace oor::query::column_export
{

void registerThermalColumns(StoredColumnCatalogue& catalogue)
{
    {
        using model::FluidStructureInterfaceTable;
        const std::uint16_t type = model::typeIndexOf<FluidStructureInterfaceTable>();
        registerColumn(
            catalogue, type, "interfaceKind", [](const auto& model)
            { return exportTextColumn(model->template table<FluidStructureInterfaceTable>().interfaceKindColumn()); });
        registerColumn(
            catalogue, type, "interfaceInput", [](const auto& model)
            { return exportTextColumn(model->template table<FluidStructureInterfaceTable>().interfaceInputColumn()); });
        registerColumn(
            catalogue, type, "fluidSetId", [](const auto& model)
            { return exportColumn(model->template table<FluidStructureInterfaceTable>().fluidSetIdColumn()); });
        registerColumn(
            catalogue, type, "structureSetId", [](const auto& model)
            { return exportColumn(model->template table<FluidStructureInterfaceTable>().structureSetIdColumn()); });
        registerColumn(
            catalogue, type, "normalTolerance", [](const auto& model)
            { return exportColumn(model->template table<FluidStructureInterfaceTable>().normalToleranceColumn()); });
        registerColumn(
            catalogue, type, "methodName", [](const auto& model)
            { return exportTextColumn(model->template table<FluidStructureInterfaceTable>().methodNameColumn()); });
        registerColumn(
            catalogue, type, "skinTolerance", [](const auto& model)
            { return exportColumn(model->template table<FluidStructureInterfaceTable>().skinToleranceColumn()); });
        registerColumn(catalogue, type, "skinToleranceIncrement",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<FluidStructureInterfaceTable>().skinToleranceIncrementColumn());
                       });
        registerColumn(
            catalogue, type, "interfaceTolerance", [](const auto& model)
            { return exportColumn(model->template table<FluidStructureInterfaceTable>().interfaceToleranceColumn()); });
        registerColumn(catalogue, type, "wholeStructureSet",
                       [](const auto& model)
                       {
                           return exportTextColumn(
                               model->template table<FluidStructureInterfaceTable>().wholeStructureSetColumn());
                       });
        registerColumn(
            catalogue, type, "searchUnit", [](const auto& model)
            { return exportTextColumn(model->template table<FluidStructureInterfaceTable>().searchUnitColumn()); });
    }
    {
        using model::TrimComponentCouplingTable;
        const std::uint16_t type = model::typeIndexOf<TrimComponentCouplingTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TrimComponentCouplingTable>().idColumn()); });
    }
    {
        using model::TrimComponentCoupling1DTable;
        const std::uint16_t type = model::typeIndexOf<TrimComponentCoupling1DTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TrimComponentCoupling1DTable>().idColumn()); });
    }
    {
        using model::AxisymFluidBoundaryListTable;
        const std::uint16_t type = model::typeIndexOf<AxisymFluidBoundaryListTable>();
        registerColumn(catalogue, type, "density", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluidBoundaryListTable>().densityColumn()); });
        registerColumn(
            catalogue, type, "points", [](const auto& model)
            { return exportVariableColumn(model->template table<AxisymFluidBoundaryListTable>().pointsColumn()); });
    }
    {
        using model::HeatSurfaceDefaultsTable;
        const std::uint16_t type = model::typeIndexOf<HeatSurfaceDefaultsTable>();
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfaceDefaultsTable>().propIdColumn()); });
    }
    {
        using model::AcousticAbsorberTable;
        const std::uint16_t type = model::typeIndexOf<AcousticAbsorberTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AcousticAbsorberTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<AcousticAbsorberTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<AcousticAbsorberTable>().nodesColumn()); });
    }
    {
        using model::AxisymFluid2Table;
        const std::uint16_t type = model::typeIndexOf<AxisymFluid2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluid2Table>().idColumn()); });
        registerColumn(catalogue, type, "rings", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluid2Table>().ringsColumn()); });
        registerColumn(catalogue, type, "density", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluid2Table>().densityColumn()); });
        registerColumn(catalogue, type, "bulkModulus", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluid2Table>().bulkModulusColumn()); });
    }
    {
        using model::AxisymFluid3Table;
        const std::uint16_t type = model::typeIndexOf<AxisymFluid3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluid3Table>().idColumn()); });
        registerColumn(catalogue, type, "rings", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluid3Table>().ringsColumn()); });
        registerColumn(catalogue, type, "density", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluid3Table>().densityColumn()); });
        registerColumn(catalogue, type, "bulkModulus", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluid3Table>().bulkModulusColumn()); });
    }
    {
        using model::AxisymFluid4Table;
        const std::uint16_t type = model::typeIndexOf<AxisymFluid4Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluid4Table>().idColumn()); });
        registerColumn(catalogue, type, "rings", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluid4Table>().ringsColumn()); });
        registerColumn(catalogue, type, "density", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluid4Table>().densityColumn()); });
        registerColumn(catalogue, type, "bulkModulus", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluid4Table>().bulkModulusColumn()); });
    }
    {
        using model::AcousticAbsorberHexaTable;
        const std::uint16_t type = model::typeIndexOf<AcousticAbsorberHexaTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AcousticAbsorberHexaTable>().idColumn()); });
        registerColumn(
            catalogue, type, "propId", [](const auto& model)
            { return exportColumn(model->template table<AcousticAbsorberHexaTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<AcousticAbsorberHexaTable>().nodesColumn()); });
    }
    {
        using model::AcousticBarrierHexaTable;
        const std::uint16_t type = model::typeIndexOf<AcousticBarrierHexaTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AcousticBarrierHexaTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<AcousticBarrierHexaTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<AcousticBarrierHexaTable>().nodesColumn()); });
    }
    {
        using model::HeatSurfaceSideTable;
        const std::uint16_t type = model::typeIndexOf<HeatSurfaceSideTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfaceSideTable>().idColumn()); });
        registerColumn(
            catalogue, type, "conductionElemId", [](const auto& model)
            { return exportColumn(model->template table<HeatSurfaceSideTable>().conductionElemIdColumn()); });
        registerColumn(catalogue, type, "sideCode", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfaceSideTable>().sideCodeColumn()); });
        registerColumn(catalogue, type, "frontViewId", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfaceSideTable>().frontViewIdColumn()); });
        registerColumn(catalogue, type, "backViewId", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfaceSideTable>().backViewIdColumn()); });
        registerColumn(
            catalogue, type, "frontRadiationPropId", [](const auto& model)
            { return exportColumn(model->template table<HeatSurfaceSideTable>().frontRadiationPropIdColumn()); });
        registerColumn(
            catalogue, type, "backRadiationPropId", [](const auto& model)
            { return exportColumn(model->template table<HeatSurfaceSideTable>().backRadiationPropIdColumn()); });
    }
    {
        using model::HeatSurfaceNodesTable;
        const std::uint16_t type = model::typeIndexOf<HeatSurfaceNodesTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfaceNodesTable>().idColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportTextColumn(model->template table<HeatSurfaceNodesTable>().kindColumn()); });
        registerColumn(
            catalogue, type, "frontViewId", [](const auto& model)
            { return exportColumn(model->template table<HeatSurfaceNodesTable>().frontViewIdColumn()); });
        registerColumn(catalogue, type, "backViewId", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfaceNodesTable>().backViewIdColumn()); });
        registerColumn(
            catalogue, type, "frontRadiationPropId", [](const auto& model)
            { return exportColumn(model->template table<HeatSurfaceNodesTable>().frontRadiationPropIdColumn()); });
        registerColumn(
            catalogue, type, "backRadiationPropId", [](const auto& model)
            { return exportColumn(model->template table<HeatSurfaceNodesTable>().backRadiationPropIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfaceNodesTable>().nodesColumn()); });
    }
    {
        using model::HeatSurfacePropertyTable;
        const std::uint16_t type = model::typeIndexOf<HeatSurfacePropertyTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfacePropertyTable>().idColumn()); });
        registerColumn(
            catalogue, type, "heatSurfacePropId", [](const auto& model)
            { return exportColumn(model->template table<HeatSurfacePropertyTable>().heatSurfacePropIdColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportTextColumn(model->template table<HeatSurfacePropertyTable>().kindColumn()); });
        registerColumn(catalogue, type, "frontViewId", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfacePropertyTable>().frontViewIdColumn()); });
        registerColumn(catalogue, type, "backViewId", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfacePropertyTable>().backViewIdColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfacePropertyTable>().node1Column()); });
        registerColumn(catalogue, type, "node2", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfacePropertyTable>().node2Column()); });
        registerColumn(
            catalogue, type, "orientationNode", [](const auto& model)
            { return exportColumn(model->template table<HeatSurfacePropertyTable>().orientationNodeColumn()); });
        registerColumn(
            catalogue, type, "frontRadiationPropId", [](const auto& model)
            { return exportColumn(model->template table<HeatSurfacePropertyTable>().frontRadiationPropIdColumn()); });
        registerColumn(
            catalogue, type, "backRadiationPropId", [](const auto& model)
            { return exportColumn(model->template table<HeatSurfacePropertyTable>().backRadiationPropIdColumn()); });
        registerColumn(catalogue, type, "midsideNode", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfacePropertyTable>().midsideNodeColumn()); });
        registerColumn(
            catalogue, type, "directionFrameId", [](const auto& model)
            { return exportColumn(model->template table<HeatSurfacePropertyTable>().directionFrameIdColumn()); });
        registerColumn(catalogue, type, "direction1", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfacePropertyTable>().direction1Column()); });
        registerColumn(catalogue, type, "direction2", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfacePropertyTable>().direction2Column()); });
        registerColumn(catalogue, type, "direction3", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfacePropertyTable>().direction3Column()); });
    }
    {
        using model::HeatControllerTable;
        const std::uint16_t type = model::typeIndexOf<HeatControllerTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HeatControllerTable>().idColumn()); });
    }
    {
        using model::FreeConvectionTable;
        const std::uint16_t type = model::typeIndexOf<FreeConvectionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FreeConvectionTable>().idColumn()); });
        registerColumn(catalogue, type, "convectionPropId", [](const auto& model)
                       { return exportColumn(model->template table<FreeConvectionTable>().convectionPropIdColumn()); });
        registerColumn(catalogue, type, "filmNode", [](const auto& model)
                       { return exportColumn(model->template table<FreeConvectionTable>().filmNodeColumn()); });
        registerColumn(catalogue, type, "controlNode", [](const auto& model)
                       { return exportColumn(model->template table<FreeConvectionTable>().controlNodeColumn()); });
        registerColumn(catalogue, type, "ambientNodes", [](const auto& model)
                       { return exportColumn(model->template table<FreeConvectionTable>().ambientNodesColumn()); });
    }
    {
        using model::ForcedConvectionTable;
        const std::uint16_t type = model::typeIndexOf<ForcedConvectionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ForcedConvectionTable>().idColumn()); });
        registerColumn(
            catalogue, type, "convectionPropId", [](const auto& model)
            { return exportColumn(model->template table<ForcedConvectionTable>().convectionPropIdColumn()); });
        registerColumn(catalogue, type, "filmNode", [](const auto& model)
                       { return exportColumn(model->template table<ForcedConvectionTable>().filmNodeColumn()); });
        registerColumn(catalogue, type, "massFlowNode", [](const auto& model)
                       { return exportColumn(model->template table<ForcedConvectionTable>().massFlowNodeColumn()); });
        registerColumn(catalogue, type, "ambientNode1", [](const auto& model)
                       { return exportColumn(model->template table<ForcedConvectionTable>().ambientNode1Column()); });
        registerColumn(catalogue, type, "ambientNode2", [](const auto& model)
                       { return exportColumn(model->template table<ForcedConvectionTable>().ambientNode2Column()); });
        registerColumn(catalogue, type, "massFlowFactor", [](const auto& model)
                       { return exportColumn(model->template table<ForcedConvectionTable>().massFlowFactorColumn()); });
    }
    {
        using model::FluidMassElemListTable;
        const std::uint16_t type = model::typeIndexOf<FluidMassElemListTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FluidMassElemListTable>().idColumn()); });
    }
    {
        using model::AxisymFluidSymmetryTable;
        const std::uint16_t type = model::typeIndexOf<AxisymFluidSymmetryTable>();
        registerColumn(
            catalogue, type, "sectionCount", [](const auto& model)
            { return exportColumn(model->template table<AxisymFluidSymmetryTable>().sectionCountColumn()); });
        registerColumn(
            catalogue, type, "planeSymmetry1", [](const auto& model)
            { return exportTextColumn(model->template table<AxisymFluidSymmetryTable>().planeSymmetry1Column()); });
        registerColumn(
            catalogue, type, "planeSymmetry2", [](const auto& model)
            { return exportTextColumn(model->template table<AxisymFluidSymmetryTable>().planeSymmetry2Column()); });
    }
    {
        using model::AxisymFluidFreeSurfaceTable;
        const std::uint16_t type = model::typeIndexOf<AxisymFluidFreeSurfaceTable>();
        registerColumn(catalogue, type, "density", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluidFreeSurfaceTable>().densityColumn()); });
        registerColumn(
            catalogue, type, "points", [](const auto& model)
            { return exportVariableColumn(model->template table<AxisymFluidFreeSurfaceTable>().pointsColumn()); });
    }
    {
        using model::HeatIsoMatTable;
        const std::uint16_t type = model::typeIndexOf<HeatIsoMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HeatIsoMatTable>().idColumn()); });
        registerColumn(catalogue, type, "conductivity", [](const auto& model)
                       { return exportColumn(model->template table<HeatIsoMatTable>().conductivityColumn()); });
        registerColumn(catalogue, type, "specificHeat", [](const auto& model)
                       { return exportColumn(model->template table<HeatIsoMatTable>().specificHeatColumn()); });
        registerColumn(catalogue, type, "density", [](const auto& model)
                       { return exportColumn(model->template table<HeatIsoMatTable>().densityColumn()); });
        registerColumn(
            catalogue, type, "convectionCoefficient", [](const auto& model)
            { return exportColumn(model->template table<HeatIsoMatTable>().convectionCoefficientColumn()); });
        registerColumn(catalogue, type, "viscosity", [](const auto& model)
                       { return exportColumn(model->template table<HeatIsoMatTable>().viscosityColumn()); });
        registerColumn(catalogue, type, "heatGenerationFactor", [](const auto& model)
                       { return exportColumn(model->template table<HeatIsoMatTable>().heatGenerationFactorColumn()); });
        registerColumn(catalogue, type, "referenceEnthalpy", [](const auto& model)
                       { return exportColumn(model->template table<HeatIsoMatTable>().referenceEnthalpyColumn()); });
        registerColumn(
            catalogue, type, "phaseChangeTemperature", [](const auto& model)
            { return exportColumn(model->template table<HeatIsoMatTable>().phaseChangeTemperatureColumn()); });
        registerColumn(catalogue, type, "phaseChangeRange", [](const auto& model)
                       { return exportColumn(model->template table<HeatIsoMatTable>().phaseChangeRangeColumn()); });
        registerColumn(catalogue, type, "latentHeat", [](const auto& model)
                       { return exportColumn(model->template table<HeatIsoMatTable>().latentHeatColumn()); });
    }
    {
        using model::HeatAnisoMatTable;
        const std::uint16_t type = model::typeIndexOf<HeatAnisoMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HeatAnisoMatTable>().idColumn()); });
        registerColumn(catalogue, type, "conductivityXx", [](const auto& model)
                       { return exportColumn(model->template table<HeatAnisoMatTable>().conductivityXxColumn()); });
        registerColumn(catalogue, type, "conductivityXy", [](const auto& model)
                       { return exportColumn(model->template table<HeatAnisoMatTable>().conductivityXyColumn()); });
        registerColumn(catalogue, type, "conductivityXz", [](const auto& model)
                       { return exportColumn(model->template table<HeatAnisoMatTable>().conductivityXzColumn()); });
        registerColumn(catalogue, type, "conductivityYy", [](const auto& model)
                       { return exportColumn(model->template table<HeatAnisoMatTable>().conductivityYyColumn()); });
        registerColumn(catalogue, type, "conductivityYz", [](const auto& model)
                       { return exportColumn(model->template table<HeatAnisoMatTable>().conductivityYzColumn()); });
        registerColumn(catalogue, type, "conductivityZz", [](const auto& model)
                       { return exportColumn(model->template table<HeatAnisoMatTable>().conductivityZzColumn()); });
        registerColumn(catalogue, type, "specificHeat", [](const auto& model)
                       { return exportColumn(model->template table<HeatAnisoMatTable>().specificHeatColumn()); });
        registerColumn(catalogue, type, "density", [](const auto& model)
                       { return exportColumn(model->template table<HeatAnisoMatTable>().densityColumn()); });
        registerColumn(
            catalogue, type, "heatGenerationFactor", [](const auto& model)
            { return exportColumn(model->template table<HeatAnisoMatTable>().heatGenerationFactorColumn()); });
    }
    {
        using model::FluidVirtualMassTable;
        const std::uint16_t type = model::typeIndexOf<FluidVirtualMassTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FluidVirtualMassTable>().idColumn()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<FluidVirtualMassTable>().frameIdColumn()); });
        registerColumn(catalogue, type, "freeSurfaceZ", [](const auto& model)
                       { return exportColumn(model->template table<FluidVirtualMassTable>().freeSurfaceZColumn()); });
        registerColumn(catalogue, type, "density", [](const auto& model)
                       { return exportColumn(model->template table<FluidVirtualMassTable>().densityColumn()); });
        registerColumn(
            catalogue, type, "oneSideElemListId", [](const auto& model)
            { return exportColumn(model->template table<FluidVirtualMassTable>().oneSideElemListIdColumn()); });
        registerColumn(
            catalogue, type, "bothSidesElemListId", [](const auto& model)
            { return exportColumn(model->template table<FluidVirtualMassTable>().bothSidesElemListIdColumn()); });
        registerColumn(
            catalogue, type, "planeSymmetry1", [](const auto& model)
            { return exportTextColumn(model->template table<FluidVirtualMassTable>().planeSymmetry1Column()); });
        registerColumn(
            catalogue, type, "planeSymmetry2", [](const auto& model)
            { return exportTextColumn(model->template table<FluidVirtualMassTable>().planeSymmetry2Column()); });
        registerColumn(
            catalogue, type, "characteristicLength", [](const auto& model)
            { return exportColumn(model->template table<FluidVirtualMassTable>().characteristicLengthColumn()); });
        registerColumn(
            catalogue, type, "exactIntegrationRatio", [](const auto& model)
            { return exportColumn(model->template table<FluidVirtualMassTable>().exactIntegrationRatioColumn()); });
    }
    {
        using model::AcousticAbsorberBrickPropTable;
        const std::uint16_t type = model::typeIndexOf<AcousticAbsorberBrickPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AcousticAbsorberBrickPropTable>().idColumn()); });
        registerColumn(
            catalogue, type, "synthesize", [](const auto& model)
            { return exportTextColumn(model->template table<AcousticAbsorberBrickPropTable>().synthesizeColumn()); });
        registerColumn(
            catalogue, type, "resistanceTableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<AcousticAbsorberBrickPropTable>().resistanceTableIdColumn());
            });
        registerColumn(
            catalogue, type, "reactanceTableId", [](const auto& model)
            { return exportColumn(model->template table<AcousticAbsorberBrickPropTable>().reactanceTableIdColumn()); });
        registerColumn(
            catalogue, type, "weightingTableId", [](const auto& model)
            { return exportColumn(model->template table<AcousticAbsorberBrickPropTable>().weightingTableIdColumn()); });
        registerColumn(
            catalogue, type, "reservedValue", [](const auto& model)
            { return exportColumn(model->template table<AcousticAbsorberBrickPropTable>().reservedValueColumn()); });
        registerColumn(
            catalogue, type, "cutoffFrequency", [](const auto& model)
            { return exportColumn(model->template table<AcousticAbsorberBrickPropTable>().cutoffFrequencyColumn()); });
        registerColumn(
            catalogue, type, "damping", [](const auto& model)
            { return exportColumn(model->template table<AcousticAbsorberBrickPropTable>().dampingColumn()); });
        registerColumn(
            catalogue, type, "stiffness", [](const auto& model)
            { return exportColumn(model->template table<AcousticAbsorberBrickPropTable>().stiffnessColumn()); });
        registerColumn(catalogue, type, "mass", [](const auto& model)
                       { return exportColumn(model->template table<AcousticAbsorberBrickPropTable>().massColumn()); });
    }
    {
        using model::AcousticBarrierPropTable;
        const std::uint16_t type = model::typeIndexOf<AcousticBarrierPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AcousticBarrierPropTable>().idColumn()); });
        registerColumn(catalogue, type, "backingMass", [](const auto& model)
                       { return exportColumn(model->template table<AcousticBarrierPropTable>().backingMassColumn()); });
        registerColumn(catalogue, type, "septumMass", [](const auto& model)
                       { return exportColumn(model->template table<AcousticBarrierPropTable>().septumMassColumn()); });
        registerColumn(
            catalogue, type, "resonanceFrequency", [](const auto& model)
            { return exportColumn(model->template table<AcousticBarrierPropTable>().resonanceFrequencyColumn()); });
        registerColumn(
            catalogue, type, "resonanceStiffness", [](const auto& model)
            { return exportColumn(model->template table<AcousticBarrierPropTable>().resonanceStiffnessColumn()); });
    }
    {
        using model::FluidStructurePanelsTable;
        const std::uint16_t type = model::typeIndexOf<FluidStructurePanelsTable>();
        registerColumn(
            catalogue, type, "sets", [](const auto& model)
            { return exportVariableColumn(model->template table<FluidStructurePanelsTable>().setsColumn()); });
    }
    {
        using model::FreeConvectionPropTable;
        const std::uint16_t type = model::typeIndexOf<FreeConvectionPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FreeConvectionPropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<FreeConvectionPropTable>().matIdColumn()); });
        registerColumn(catalogue, type, "matrixForm", [](const auto& model)
                       { return exportColumn(model->template table<FreeConvectionPropTable>().matrixFormColumn()); });
        registerColumn(
            catalogue, type, "convectionExponent", [](const auto& model)
            { return exportColumn(model->template table<FreeConvectionPropTable>().convectionExponentColumn()); });
        registerColumn(catalogue, type, "formulaKind", [](const auto& model)
                       { return exportColumn(model->template table<FreeConvectionPropTable>().formulaKindColumn()); });
        registerColumn(catalogue, type, "tableId", [](const auto& model)
                       { return exportColumn(model->template table<FreeConvectionPropTable>().tableIdColumn()); });
        registerColumn(
            catalogue, type, "characteristicLength", [](const auto& model)
            { return exportColumn(model->template table<FreeConvectionPropTable>().characteristicLengthColumn()); });
        registerColumn(catalogue, type, "inletNode", [](const auto& model)
                       { return exportColumn(model->template table<FreeConvectionPropTable>().inletNodeColumn()); });
        registerColumn(
            catalogue, type, "directionFrameId", [](const auto& model)
            { return exportColumn(model->template table<FreeConvectionPropTable>().directionFrameIdColumn()); });
        registerColumn(catalogue, type, "direction1", [](const auto& model)
                       { return exportColumn(model->template table<FreeConvectionPropTable>().direction1Column()); });
        registerColumn(catalogue, type, "direction2", [](const auto& model)
                       { return exportColumn(model->template table<FreeConvectionPropTable>().direction2Column()); });
        registerColumn(catalogue, type, "direction3", [](const auto& model)
                       { return exportColumn(model->template table<FreeConvectionPropTable>().direction3Column()); });
        registerColumn(
            catalogue, type, "convectionCoefficient", [](const auto& model)
            { return exportColumn(model->template table<FreeConvectionPropTable>().convectionCoefficientColumn()); });
    }
    {
        using model::ForcedConvectionPropTable;
        const std::uint16_t type = model::typeIndexOf<ForcedConvectionPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ForcedConvectionPropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<ForcedConvectionPropTable>().matIdColumn()); });
        registerColumn(catalogue, type, "matrixForm", [](const auto& model)
                       { return exportColumn(model->template table<ForcedConvectionPropTable>().matrixFormColumn()); });
        registerColumn(
            catalogue, type, "massFlowConvection", [](const auto& model)
            { return exportColumn(model->template table<ForcedConvectionPropTable>().massFlowConvectionColumn()); });
        registerColumn(
            catalogue, type, "coefficient", [](const auto& model)
            { return exportColumn(model->template table<ForcedConvectionPropTable>().coefficientColumn()); });
        registerColumn(
            catalogue, type, "reynoldsExponent", [](const auto& model)
            { return exportColumn(model->template table<ForcedConvectionPropTable>().reynoldsExponentColumn()); });
        registerColumn(
            catalogue, type, "prandtlExponentInner", [](const auto& model)
            { return exportColumn(model->template table<ForcedConvectionPropTable>().prandtlExponentInnerColumn()); });
        registerColumn(
            catalogue, type, "prandtlExponentOuter", [](const auto& model)
            { return exportColumn(model->template table<ForcedConvectionPropTable>().prandtlExponentOuterColumn()); });
    }
    {
        using model::HeatSurfacePropTable;
        const std::uint16_t type = model::typeIndexOf<HeatSurfacePropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfacePropTable>().idColumn()); });
        registerColumn(catalogue, type, "area", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfacePropTable>().areaColumn()); });
        registerColumn(catalogue, type, "diameter1", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfacePropTable>().diameter1Column()); });
        registerColumn(catalogue, type, "diameter2", [](const auto& model)
                       { return exportColumn(model->template table<HeatSurfacePropTable>().diameter2Column()); });
    }
    {
        using model::HeatFluxSurfaceLoad1Table;
        const std::uint16_t type = model::typeIndexOf<HeatFluxSurfaceLoad1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HeatFluxSurfaceLoad1Table>().idColumn()); });
        registerColumn(catalogue, type, "heatFlux", [](const auto& model)
                       { return exportColumn(model->template table<HeatFluxSurfaceLoad1Table>().heatFluxColumn()); });
        registerColumn(
            catalogue, type, "runs", [](const auto& model)
            { return exportVariableColumn(model->template table<HeatFluxSurfaceLoad1Table>().runsColumn()); });
    }
    {
        using model::HeatFluxNodalLoad2Table;
        const std::uint16_t type = model::typeIndexOf<HeatFluxNodalLoad2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HeatFluxNodalLoad2Table>().idColumn()); });
        registerColumn(catalogue, type, "elemId", [](const auto& model)
                       { return exportColumn(model->template table<HeatFluxNodalLoad2Table>().elemIdColumn()); });
        registerColumn(catalogue, type, "heatFlux", [](const auto& model)
                       { return exportColumn(model->template table<HeatFluxNodalLoad2Table>().heatFluxColumn()); });
    }
    {
        using model::HeatFluxBoundaryLoad3Table;
        const std::uint16_t type = model::typeIndexOf<HeatFluxBoundaryLoad3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HeatFluxBoundaryLoad3Table>().idColumn()); });
        registerColumn(catalogue, type, "heatFlux", [](const auto& model)
                       { return exportColumn(model->template table<HeatFluxBoundaryLoad3Table>().heatFluxColumn()); });
        registerColumn(
            catalogue, type, "controlNode", [](const auto& model)
            { return exportColumn(model->template table<HeatFluxBoundaryLoad3Table>().controlNodeColumn()); });
        registerColumn(
            catalogue, type, "runs", [](const auto& model)
            { return exportVariableColumn(model->template table<HeatFluxBoundaryLoad3Table>().runsColumn()); });
    }
    {
        using model::HeatFluxNodeSetLoadTable;
        const std::uint16_t type = model::typeIndexOf<HeatFluxNodeSetLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HeatFluxNodeSetLoadTable>().idColumn()); });
        registerColumn(
            catalogue, type, "faceKind", [](const auto& model)
            { return exportTextColumn(model->template table<HeatFluxNodeSetLoadTable>().faceKindColumn()); });
        registerColumn(catalogue, type, "heatFlux", [](const auto& model)
                       { return exportColumn(model->template table<HeatFluxNodeSetLoadTable>().heatFluxColumn()); });
        registerColumn(catalogue, type, "area", [](const auto& model)
                       { return exportColumn(model->template table<HeatFluxNodeSetLoadTable>().areaColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<HeatFluxNodeSetLoadTable>().nodesColumn()); });
    }
    {
        using model::HeatFluxVectorLoadTable;
        const std::uint16_t type = model::typeIndexOf<HeatFluxVectorLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HeatFluxVectorLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "heatFlux", [](const auto& model)
                       { return exportColumn(model->template table<HeatFluxVectorLoadTable>().heatFluxColumn()); });
        registerColumn(
            catalogue, type, "sourceTemperature", [](const auto& model)
            { return exportColumn(model->template table<HeatFluxVectorLoadTable>().sourceTemperatureColumn()); });
        registerColumn(
            catalogue, type, "directionFrameId", [](const auto& model)
            { return exportColumn(model->template table<HeatFluxVectorLoadTable>().directionFrameIdColumn()); });
        registerColumn(catalogue, type, "direction", [](const auto& model)
                       { return exportColumn(model->template table<HeatFluxVectorLoadTable>().directionColumn()); });
        registerColumn(catalogue, type, "tableId", [](const auto& model)
                       { return exportColumn(model->template table<HeatFluxVectorLoadTable>().tableIdColumn()); });
        registerColumn(catalogue, type, "controlNode", [](const auto& model)
                       { return exportColumn(model->template table<HeatFluxVectorLoadTable>().controlNodeColumn()); });
        registerColumn(catalogue, type, "runs", [](const auto& model)
                       { return exportVariableColumn(model->template table<HeatFluxVectorLoadTable>().runsColumn()); });
    }
    {
        using model::HeatVolumeSourceLoadTable;
        const std::uint16_t type = model::typeIndexOf<HeatVolumeSourceLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HeatVolumeSourceLoadTable>().idColumn()); });
        registerColumn(
            catalogue, type, "heatGeneration", [](const auto& model)
            { return exportColumn(model->template table<HeatVolumeSourceLoadTable>().heatGenerationColumn()); });
        registerColumn(
            catalogue, type, "controlNode", [](const auto& model)
            { return exportColumn(model->template table<HeatVolumeSourceLoadTable>().controlNodeColumn()); });
        registerColumn(
            catalogue, type, "runs", [](const auto& model)
            { return exportVariableColumn(model->template table<HeatVolumeSourceLoadTable>().runsColumn()); });
    }
    {
        using model::RadiationToSpaceTable;
        const std::uint16_t type = model::typeIndexOf<RadiationToSpaceTable>();
        registerColumn(catalogue, type, "ambientNode", [](const auto& model)
                       { return exportColumn(model->template table<RadiationToSpaceTable>().ambientNodeColumn()); });
        registerColumn(
            catalogue, type, "ambientViewFactor", [](const auto& model)
            { return exportColumn(model->template table<RadiationToSpaceTable>().ambientViewFactorColumn()); });
        registerColumn(catalogue, type, "controlNode", [](const auto& model)
                       { return exportColumn(model->template table<RadiationToSpaceTable>().controlNodeColumn()); });
        registerColumn(catalogue, type, "runs", [](const auto& model)
                       { return exportVariableColumn(model->template table<RadiationToSpaceTable>().runsColumn()); });
    }
    {
        using model::RadiationBandsTable;
        const std::uint16_t type = model::typeIndexOf<RadiationBandsTable>();
        registerColumn(catalogue, type, "bandCount", [](const auto& model)
                       { return exportColumn(model->template table<RadiationBandsTable>().bandCountColumn()); });
        registerColumn(
            catalogue, type, "planckSecondConstant", [](const auto& model)
            { return exportColumn(model->template table<RadiationBandsTable>().planckSecondConstantColumn()); });
        registerColumn(
            catalogue, type, "wavelengths", [](const auto& model)
            { return exportVariableColumn(model->template table<RadiationBandsTable>().wavelengthsColumn()); });
    }
    {
        using model::RadiationCavityTable;
        const std::uint16_t type = model::typeIndexOf<RadiationCavityTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RadiationCavityTable>().idColumn()); });
        registerColumn(catalogue, type, "ambientElemId", [](const auto& model)
                       { return exportColumn(model->template table<RadiationCavityTable>().ambientElemIdColumn()); });
        registerColumn(catalogue, type, "shadowing", [](const auto& model)
                       { return exportTextColumn(model->template table<RadiationCavityTable>().shadowingColumn()); });
        registerColumn(catalogue, type, "scaleFactor", [](const auto& model)
                       { return exportColumn(model->template table<RadiationCavityTable>().scaleFactorColumn()); });
        registerColumn(
            catalogue, type, "viewFactorOutput", [](const auto& model)
            { return exportColumn(model->template table<RadiationCavityTable>().viewFactorOutputColumn()); });
        registerColumn(
            catalogue, type, "viewFactorMethod", [](const auto& model)
            { return exportTextColumn(model->template table<RadiationCavityTable>().viewFactorMethodColumn()); });
        registerColumn(
            catalogue, type, "subdivisionAreaRatio", [](const auto& model)
            { return exportColumn(model->template table<RadiationCavityTable>().subdivisionAreaRatioColumn()); });
        registerColumn(catalogue, type, "componentCount", [](const auto& model)
                       { return exportColumn(model->template table<RadiationCavityTable>().componentCountColumn()); });
        registerColumn(catalogue, type, "sets", [](const auto& model)
                       { return exportVariableColumn(model->template table<RadiationCavityTable>().setsColumn()); });
    }
    {
        using model::RadiationCavityListTable;
        const std::uint16_t type = model::typeIndexOf<RadiationCavityListTable>();
        registerColumn(catalogue, type, "cavityId", [](const auto& model)
                       { return exportColumn(model->template table<RadiationCavityListTable>().cavityIdColumn()); });
        registerColumn(catalogue, type, "matrixKind", [](const auto& model)
                       { return exportColumn(model->template table<RadiationCavityListTable>().matrixKindColumn()); });
        registerColumn(
            catalogue, type, "runs", [](const auto& model)
            { return exportVariableColumn(model->template table<RadiationCavityListTable>().runsColumn()); });
    }
    {
        using model::RadiationPropTable;
        const std::uint16_t type = model::typeIndexOf<RadiationPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RadiationPropTable>().idColumn()); });
        registerColumn(catalogue, type, "absorptivity", [](const auto& model)
                       { return exportColumn(model->template table<RadiationPropTable>().absorptivityColumn()); });
        registerColumn(
            catalogue, type, "emissivity", [](const auto& model)
            { return exportVariableColumn(model->template table<RadiationPropTable>().emissivityColumn()); });
    }
    {
        using model::RadiationPropTemperatureTable;
        const std::uint16_t type = model::typeIndexOf<RadiationPropTemperatureTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RadiationPropTemperatureTable>().idColumn()); });
        registerColumn(
            catalogue, type, "absorptivityTableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<RadiationPropTemperatureTable>().absorptivityTableIdColumn());
            });
        registerColumn(catalogue, type, "emissivityTableIds",
                       [](const auto& model)
                       {
                           return exportVariableColumn(
                               model->template table<RadiationPropTemperatureTable>().emissivityTableIdsColumn());
                       });
    }
    {
        using model::RadiationExchangeMatrixTable;
        const std::uint16_t type = model::typeIndexOf<RadiationExchangeMatrixTable>();
        registerColumn(
            catalogue, type, "cavityId", [](const auto& model)
            { return exportColumn(model->template table<RadiationExchangeMatrixTable>().cavityIdColumn()); });
        registerColumn(
            catalogue, type, "columnIndex", [](const auto& model)
            { return exportColumn(model->template table<RadiationExchangeMatrixTable>().columnIndexColumn()); });
        registerColumn(
            catalogue, type, "factors", [](const auto& model)
            { return exportVariableColumn(model->template table<RadiationExchangeMatrixTable>().factorsColumn()); });
    }
    {
        using model::RadiationCavitySetTable;
        const std::uint16_t type = model::typeIndexOf<RadiationCavitySetTable>();
        registerColumn(
            catalogue, type, "cavities", [](const auto& model)
            { return exportVariableColumn(model->template table<RadiationCavitySetTable>().cavitiesColumn()); });
    }
    {
        using model::AcousticSlotBoundaryTable;
        const std::uint16_t type = model::typeIndexOf<AcousticSlotBoundaryTable>();
        registerColumn(catalogue, type, "count", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSlotBoundaryTable>().pointCountColumn()); });
    }
    {
        using model::TrimLayeredComponentTable;
        const std::uint16_t type = model::typeIndexOf<TrimLayeredComponentTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TrimLayeredComponentTable>().idColumn()); });
    }
    {
        using model::TemperatureBoundaryTable;
        const std::uint16_t type = model::typeIndexOf<TemperatureBoundaryTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TemperatureBoundaryTable>().idColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportTextColumn(model->template table<TemperatureBoundaryTable>().kindColumn()); });
        registerColumn(
            catalogue, type, "temperatures", [](const auto& model)
            { return exportVariableColumn(model->template table<TemperatureBoundaryTable>().temperaturesColumn()); });
        registerColumn(
            catalogue, type, "runs", [](const auto& model)
            { return exportVariableColumn(model->template table<TemperatureBoundaryTable>().runsColumn()); });
    }
    {
        using model::TrimCouplingTolerancesTable;
        const std::uint16_t type = model::typeIndexOf<TrimCouplingTolerancesTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TrimCouplingTolerancesTable>().idColumn()); });
        registerColumn(
            catalogue, type, "couplingKind", [](const auto& model)
            { return exportColumn(model->template table<TrimCouplingTolerancesTable>().couplingKindColumn()); });
        registerColumn(
            catalogue, type, "relativeTolerance", [](const auto& model)
            { return exportColumn(model->template table<TrimCouplingTolerancesTable>().relativeToleranceColumn()); });
        registerColumn(
            catalogue, type, "gapTolerance1", [](const auto& model)
            { return exportColumn(model->template table<TrimCouplingTolerancesTable>().gapTolerance1Column()); });
        registerColumn(
            catalogue, type, "gapTolerance2", [](const auto& model)
            { return exportColumn(model->template table<TrimCouplingTolerancesTable>().gapTolerance2Column()); });
        registerColumn(
            catalogue, type, "gapTolerance3", [](const auto& model)
            { return exportColumn(model->template table<TrimCouplingTolerancesTable>().gapTolerance3Column()); });
        registerColumn(
            catalogue, type, "gapTolerance4", [](const auto& model)
            { return exportColumn(model->template table<TrimCouplingTolerancesTable>().gapTolerance4Column()); });
        registerColumn(catalogue, type, "regionId", [](const auto& model)
                       { return exportColumn(model->template table<TrimCouplingTolerancesTable>().regionIdColumn()); });
    }
    {
        using model::ViewFactorFaceTable;
        const std::uint16_t type = model::typeIndexOf<ViewFactorFaceTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ViewFactorFaceTable>().idColumn()); });
        registerColumn(catalogue, type, "cavityId", [](const auto& model)
                       { return exportColumn(model->template table<ViewFactorFaceTable>().cavityIdColumn()); });
        registerColumn(catalogue, type, "shading", [](const auto& model)
                       { return exportTextColumn(model->template table<ViewFactorFaceTable>().shadingColumn()); });
        registerColumn(catalogue, type, "betaSubdivisions", [](const auto& model)
                       { return exportColumn(model->template table<ViewFactorFaceTable>().betaSubdivisionsColumn()); });
        registerColumn(
            catalogue, type, "gammaSubdivisions", [](const auto& model)
            { return exportColumn(model->template table<ViewFactorFaceTable>().gammaSubdivisionsColumn()); });
        registerColumn(catalogue, type, "surfaceOffset", [](const auto& model)
                       { return exportColumn(model->template table<ViewFactorFaceTable>().surfaceOffsetColumn()); });
    }
    {
        using model::ViewFactorIntegrationTable;
        const std::uint16_t type = model::typeIndexOf<ViewFactorIntegrationTable>();
        registerColumn(catalogue, type, "cavityId", [](const auto& model)
                       { return exportColumn(model->template table<ViewFactorIntegrationTable>().cavityIdColumn()); });
        registerColumn(catalogue, type, "blockingIntegrationOrder",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<ViewFactorIntegrationTable>().blockingIntegrationOrderColumn());
                       });
        registerColumn(catalogue, type, "shadowingIntegrationOrder",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<ViewFactorIntegrationTable>().shadowingIntegrationOrderColumn());
                       });
        registerColumn(catalogue, type, "contourIntegrationLevel",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<ViewFactorIntegrationTable>().contourIntegrationLevelColumn());
                       });
        registerColumn(
            catalogue, type, "errorTolerance", [](const auto& model)
            { return exportColumn(model->template table<ViewFactorIntegrationTable>().errorToleranceColumn()); });
        registerColumn(
            catalogue, type, "zeroTolerance", [](const auto& model)
            { return exportColumn(model->template table<ViewFactorIntegrationTable>().zeroToleranceColumn()); });
        registerColumn(
            catalogue, type, "warpTolerance", [](const auto& model)
            { return exportColumn(model->template table<ViewFactorIntegrationTable>().warpToleranceColumn()); });
        registerColumn(
            catalogue, type, "radiationCheck", [](const auto& model)
            { return exportColumn(model->template table<ViewFactorIntegrationTable>().radiationCheckColumn()); });
    }
}

} // namespace oor::query::column_export
