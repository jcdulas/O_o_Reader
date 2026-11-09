// The stored columns of the cards of the family "loads", for the query engine (ColumnExport.hpp).
// One block a card type: its type index, then a getter a column.

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/loads/LoadClosureSupport.hpp"
#include "oor/model/cards/loads/accelFieldLoad/AccelFieldLoadTable.hpp"
#include "oor/model/cards/loads/accelNodeLoad/AccelNodeLoadTable.hpp"
#include "oor/model/cards/loads/acousticPressureLoadMatrices/AcousticPressureLoadMatricesTable.hpp"
#include "oor/model/cards/loads/axialDeformationLoad/AxialDeformationLoadTable.hpp"
#include "oor/model/cards/loads/axisymConePressureLoad/AxisymConePressureLoadTable.hpp"
#include "oor/model/cards/loads/axisymConeTemperatureLoad/AxisymConeTemperatureLoadTable.hpp"
#include "oor/model/cards/loads/axisymRingForceLoad/AxisymRingForceLoadTable.hpp"
#include "oor/model/cards/loads/axisymRingMomentLoad/AxisymRingMomentLoadTable.hpp"
#include "oor/model/cards/loads/axisymTractionLoad/AxisymTractionLoadTable.hpp"
#include "oor/model/cards/loads/beam3DistributedLoad/Beam3DistributedLoadTable.hpp"
#include "oor/model/cards/loads/beam3TemperatureLoad/Beam3TemperatureLoadTable.hpp"
#include "oor/model/cards/loads/beamDistributedLoad/BeamDistributedLoadTable.hpp"
#include "oor/model/cards/loads/cyclicAzimuthTableLoad/CyclicAzimuthTableLoadTable.hpp"
#include "oor/model/cards/loads/cyclicHarmonicLoad/CyclicHarmonicLoadTable.hpp"
#include "oor/model/cards/loads/cyclicSegmentLoad/CyclicSegmentLoadTable.hpp"
#include "oor/model/cards/loads/defaultTemperatureLoad/DefaultTemperatureLoadTable.hpp"
#include "oor/model/cards/loads/facePressureLoad/FacePressureLoadTable.hpp"
#include "oor/model/cards/loads/freeBodyLoad/FreeBodyLoadTable.hpp"
#include "oor/model/cards/loads/freeBodyLoadSubcase/FreeBodyLoadSubcaseTable.hpp"
#include "oor/model/cards/loads/gravityLoad/GravityLoadTable.hpp"
#include "oor/model/cards/loads/heatShellInitialTemperature/HeatShellInitialTemperatureTable.hpp"
#include "oor/model/cards/loads/hydrostaticCavityLoad/HydrostaticCavityLoadTable.hpp"
#include "oor/model/cards/loads/hydrostaticLoad/HydrostaticLoadTable.hpp"
#include "oor/model/cards/loads/imperfectionCase/ImperfectionCaseTable.hpp"
#include "oor/model/cards/loads/imperfectionGeometry/ImperfectionGeometryTable.hpp"
#include "oor/model/cards/loads/inflatorInlet/InflatorInletTable.hpp"
#include "oor/model/cards/loads/lineElemTemperatureLoad/LineElemTemperatureLoadTable.hpp"
#include "oor/model/cards/loads/loadCaseIdLabel/LoadCaseIdLabelTable.hpp"
#include "oor/model/cards/loads/loadCaseNameLabel/LoadCaseNameLabelTable.hpp"
#include "oor/model/cards/loads/loadCaseSubcaseLabel/LoadCaseSubcaseLabelTable.hpp"
#include "oor/model/cards/loads/loadCombination/LoadCombinationTable.hpp"
#include "oor/model/cards/loads/metadataBooleanPairs/MetadataBooleanPairsTable.hpp"
#include "oor/model/cards/loads/metadataGroup/MetadataGroupTable.hpp"
#include "oor/model/cards/loads/metadataIntegerPairs/MetadataIntegerPairsTable.hpp"
#include "oor/model/cards/loads/metadataRealPairs/MetadataRealPairsTable.hpp"
#include "oor/model/cards/loads/metadataTextPairs/MetadataTextPairsTable.hpp"
#include "oor/model/cards/loads/nodeTemperatureLoad/NodeTemperatureLoadTable.hpp"
#include "oor/model/cards/loads/plateTemperatureLoad/PlateTemperatureLoadTable.hpp"
#include "oor/model/cards/loads/pointForceLoad/PointForceLoadTable.hpp"
#include "oor/model/cards/loads/pointForceLoad1/PointForceLoad1Table.hpp"
#include "oor/model/cards/loads/pointForceLoad2/PointForceLoad2Table.hpp"
#include "oor/model/cards/loads/pointMomentLoad/PointMomentLoadTable.hpp"
#include "oor/model/cards/loads/pointMomentLoad1/PointMomentLoad1Table.hpp"
#include "oor/model/cards/loads/pointMomentLoad2/PointMomentLoad2Table.hpp"
#include "oor/model/cards/loads/pointsPressureLoad/PointsPressureLoadTable.hpp"
#include "oor/model/cards/loads/rotationalForceLoad/RotationalForceLoadTable.hpp"
#include "oor/model/cards/loads/scalarPointLoad/ScalarPointLoadTable.hpp"
#include "oor/model/cards/loads/shellPressureLoad/ShellPressureLoadTable.hpp"
#include "oor/model/cards/loads/staticLoadSequence/StaticLoadSequenceTable.hpp"
#include "oor/model/cards/loads/superelementLoadCombination/SuperelementLoadCombinationTable.hpp"
#include "oor/model/cards/loads/temperatureTimeGroup/TemperatureTimeGroupTable.hpp"
#include "oor/model/cards/loads/timeTemperatureLoad/TimeTemperatureLoadTable.hpp"
#include "oor/model/cards/loads/wettedSurfaceLoad/WettedSurfaceLoadTable.hpp"
#include "oor/query/providers/ColumnExport.hpp"

namespace oor::query::column_export
{

void registerLoadsColumns(StoredColumnCatalogue& catalogue)
{
    {
        using model::InflatorInletTable;
        const std::uint16_t type = model::typeIndexOf<InflatorInletTable>();
        registerColumn(catalogue, type, "inflatorId", [](const auto& model)
                       { return exportColumn(model->template table<InflatorInletTable>().inflatorIdColumn()); });
    }
    {
        using model::MetadataBooleanPairsTable;
        const std::uint16_t type = model::typeIndexOf<MetadataBooleanPairsTable>();
        registerColumn(
            catalogue, type, "metadataGroupId", [](const auto& model)
            { return exportColumn(model->template table<MetadataBooleanPairsTable>().metadataGroupIdColumn()); });
    }
    {
        using model::MetadataIntegerPairsTable;
        const std::uint16_t type = model::typeIndexOf<MetadataIntegerPairsTable>();
        registerColumn(
            catalogue, type, "metadataGroupId", [](const auto& model)
            { return exportColumn(model->template table<MetadataIntegerPairsTable>().metadataGroupIdColumn()); });
    }
    {
        using model::MetadataRealPairsTable;
        const std::uint16_t type = model::typeIndexOf<MetadataRealPairsTable>();
        registerColumn(
            catalogue, type, "metadataGroupId", [](const auto& model)
            { return exportColumn(model->template table<MetadataRealPairsTable>().metadataGroupIdColumn()); });
    }
    {
        using model::MetadataTextPairsTable;
        const std::uint16_t type = model::typeIndexOf<MetadataTextPairsTable>();
        registerColumn(
            catalogue, type, "metadataGroupId", [](const auto& model)
            { return exportColumn(model->template table<MetadataTextPairsTable>().metadataGroupIdColumn()); });
    }
    {
        using model::MetadataGroupTable;
        const std::uint16_t type = model::typeIndexOf<MetadataGroupTable>();
        registerColumn(catalogue, type, "metadataGroupName", [](const auto& model)
                       { return exportTextColumn(model->template table<MetadataGroupTable>().groupNameColumn()); });
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<MetadataGroupTable>().idColumn()); });
        registerColumn(catalogue, type, "entityName", [](const auto& model)
                       { return exportTextColumn(model->template table<MetadataGroupTable>().entityNameColumn()); });
        registerColumn(catalogue, type, "entityIds", [](const auto& model)
                       { return exportVariableColumn(model->template table<MetadataGroupTable>().entitiesColumn()); });
    }
    {
        using model::AccelFieldLoadTable;
        const std::uint16_t type = model::typeIndexOf<AccelFieldLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AccelFieldLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<AccelFieldLoadTable>().frameIdColumn()); });
        registerColumn(catalogue, type, "directionX", [](const auto& model)
                       { return exportColumn(model->template table<AccelFieldLoadTable>().directionXColumn()); });
        registerColumn(catalogue, type, "directionY", [](const auto& model)
                       { return exportColumn(model->template table<AccelFieldLoadTable>().directionYColumn()); });
        registerColumn(catalogue, type, "directionZ", [](const auto& model)
                       { return exportColumn(model->template table<AccelFieldLoadTable>().directionZColumn()); });
        registerColumn(catalogue, type, "direction", [](const auto& model)
                       { return exportColumn(model->template table<AccelFieldLoadTable>().directionColumn()); });
        registerColumn(catalogue, type, "points", [](const auto& model)
                       { return exportVariableColumn(model->template table<AccelFieldLoadTable>().pointsColumn()); });
    }
    {
        using model::AccelNodeLoadTable;
        const std::uint16_t type = model::typeIndexOf<AccelNodeLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AccelNodeLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<AccelNodeLoadTable>().frameIdColumn()); });
        registerColumn(catalogue, type, "magnitude", [](const auto& model)
                       { return exportColumn(model->template table<AccelNodeLoadTable>().magnitudeColumn()); });
        registerColumn(catalogue, type, "directionX", [](const auto& model)
                       { return exportColumn(model->template table<AccelNodeLoadTable>().directionXColumn()); });
        registerColumn(catalogue, type, "directionY", [](const auto& model)
                       { return exportColumn(model->template table<AccelNodeLoadTable>().directionYColumn()); });
        registerColumn(catalogue, type, "directionZ", [](const auto& model)
                       { return exportColumn(model->template table<AccelNodeLoadTable>().directionZColumn()); });
        registerColumn(catalogue, type, "grids", [](const auto& model)
                       { return exportVariableColumn(model->template table<AccelNodeLoadTable>().gridsColumn()); });
    }
    {
        using model::AcousticPressureLoadMatricesTable;
        const std::uint16_t type = model::typeIndexOf<AcousticPressureLoadMatricesTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AcousticPressureLoadMatricesTable>().idColumn()); });
        registerColumn(
            catalogue, type, "dataFileUnit", [](const auto& model)
            { return exportColumn(model->template table<AcousticPressureLoadMatricesTable>().dataFileUnitColumn()); });
        registerColumn(catalogue, type, "propertyFileUnit",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<AcousticPressureLoadMatricesTable>().propertyFileUnitColumn());
                       });
        registerColumn(
            catalogue, type, "scaleReal", [](const auto& model)
            { return exportColumn(model->template table<AcousticPressureLoadMatricesTable>().scaleRealColumn()); });
        registerColumn(
            catalogue, type, "scaleImaginary",
            [](const auto& model)
            {
                return exportColumn(model->template table<AcousticPressureLoadMatricesTable>().scaleImaginaryColumn());
            });
        registerColumn(
            catalogue, type, "fileLoadCaseId",
            [](const auto& model)
            {
                return exportColumn(model->template table<AcousticPressureLoadMatricesTable>().fileLoadCaseIdColumn());
            });
    }
    {
        using model::SuperelementLoadCombinationTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementLoadCombinationTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SuperelementLoadCombinationTable>().idColumn()); });
    }
    {
        using model::AxialDeformationLoadTable;
        const std::uint16_t type = model::typeIndexOf<AxialDeformationLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxialDeformationLoadTable>().idColumn()); });
        registerColumn(
            catalogue, type, "element", [](const auto& model)
            { return exportVariableColumn(model->template table<AxialDeformationLoadTable>().elementColumn()); });
        registerColumn(
            catalogue, type, "deformation", [](const auto& model)
            { return exportVariableColumn(model->template table<AxialDeformationLoadTable>().deformationColumn()); });
    }
    {
        using model::FreeBodyLoadTable;
        const std::uint16_t type = model::typeIndexOf<FreeBodyLoadTable>();
        registerColumn(catalogue, type, "name", [](const auto& model)
                       { return exportTextColumn(model->template table<FreeBodyLoadTable>().nameColumn()); });
        registerColumn(catalogue, type, "subsystemName", [](const auto& model)
                       { return exportTextColumn(model->template table<FreeBodyLoadTable>().subsystemNameColumn()); });
        registerColumn(catalogue, type, "label", [](const auto& model)
                       { return exportTextColumn(model->template table<FreeBodyLoadTable>().labelColumn()); });
    }
    {
        using model::FreeBodyLoadSubcaseTable;
        const std::uint16_t type = model::typeIndexOf<FreeBodyLoadSubcaseTable>();
        registerColumn(catalogue, type, "names", [](const auto& model)
                       { return exportTextColumn(model->template table<FreeBodyLoadSubcaseTable>().namesColumn()); });
        registerColumn(catalogue, type, "nodeSetId", [](const auto& model)
                       { return exportColumn(model->template table<FreeBodyLoadSubcaseTable>().nodeSetIdColumn()); });
        registerColumn(
            catalogue, type, "elementSetId", [](const auto& model)
            { return exportColumn(model->template table<FreeBodyLoadSubcaseTable>().elementSetIdColumn()); });
        registerColumn(
            catalogue, type, "excludedForces", [](const auto& model)
            { return exportTextColumn(model->template table<FreeBodyLoadSubcaseTable>().excludedForcesColumn()); });
        registerColumn(catalogue, type, "label", [](const auto& model)
                       { return exportTextColumn(model->template table<FreeBodyLoadSubcaseTable>().labelColumn()); });
    }
    {
        using model::PointForceLoadTable;
        const std::uint16_t type = model::typeIndexOf<PointForceLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PointForceLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportColumn(model->template table<PointForceLoadTable>().nodeColumn()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<PointForceLoadTable>().frameIdColumn()); });
        registerColumn(catalogue, type, "magnitude", [](const auto& model)
                       { return exportColumn(model->template table<PointForceLoadTable>().magnitudeColumn()); });
        registerColumn(catalogue, type, "direction", [](const auto& model)
                       { return exportColumn(model->template table<PointForceLoadTable>().directionColumn()); });
    }
    {
        using model::PointForceLoad1Table;
        const std::uint16_t type = model::typeIndexOf<PointForceLoad1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PointForceLoad1Table>().idColumn()); });
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportColumn(model->template table<PointForceLoad1Table>().nodeColumn()); });
        registerColumn(catalogue, type, "magnitude", [](const auto& model)
                       { return exportColumn(model->template table<PointForceLoad1Table>().magnitudeColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<PointForceLoad1Table>().node1Column()); });
        registerColumn(catalogue, type, "node2", [](const auto& model)
                       { return exportColumn(model->template table<PointForceLoad1Table>().node2Column()); });
    }
    {
        using model::PointForceLoad2Table;
        const std::uint16_t type = model::typeIndexOf<PointForceLoad2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PointForceLoad2Table>().idColumn()); });
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportColumn(model->template table<PointForceLoad2Table>().nodeColumn()); });
        registerColumn(catalogue, type, "magnitude", [](const auto& model)
                       { return exportColumn(model->template table<PointForceLoad2Table>().magnitudeColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<PointForceLoad2Table>().node1Column()); });
        registerColumn(catalogue, type, "node2", [](const auto& model)
                       { return exportColumn(model->template table<PointForceLoad2Table>().node2Column()); });
        registerColumn(catalogue, type, "node3", [](const auto& model)
                       { return exportColumn(model->template table<PointForceLoad2Table>().node3Column()); });
        registerColumn(catalogue, type, "node4", [](const auto& model)
                       { return exportColumn(model->template table<PointForceLoad2Table>().node4Column()); });
    }
    {
        using model::AxisymRingForceLoadTable;
        const std::uint16_t type = model::typeIndexOf<AxisymRingForceLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymRingForceLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "ringId", [](const auto& model)
                       { return exportColumn(model->template table<AxisymRingForceLoadTable>().ringIdColumn()); });
        registerColumn(catalogue, type, "harmonicId", [](const auto& model)
                       { return exportColumn(model->template table<AxisymRingForceLoadTable>().harmonicIdColumn()); });
        registerColumn(catalogue, type, "harmonicEnd", [](const auto& model)
                       { return exportColumn(model->template table<AxisymRingForceLoadTable>().harmonicEndColumn()); });
        registerColumn(catalogue, type, "scaleFactor", [](const auto& model)
                       { return exportColumn(model->template table<AxisymRingForceLoadTable>().scaleFactorColumn()); });
        registerColumn(catalogue, type, "radialForce", [](const auto& model)
                       { return exportColumn(model->template table<AxisymRingForceLoadTable>().radialForceColumn()); });
        registerColumn(
            catalogue, type, "azimuthalForce", [](const auto& model)
            { return exportColumn(model->template table<AxisymRingForceLoadTable>().azimuthalForceColumn()); });
        registerColumn(catalogue, type, "axialForce", [](const auto& model)
                       { return exportColumn(model->template table<AxisymRingForceLoadTable>().axialForceColumn()); });
    }
    {
        using model::GravityLoadTable;
        const std::uint16_t type = model::typeIndexOf<GravityLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<GravityLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<GravityLoadTable>().frameIdColumn()); });
        registerColumn(catalogue, type, "magnitude", [](const auto& model)
                       { return exportColumn(model->template table<GravityLoadTable>().magnitudeColumn()); });
        registerColumn(catalogue, type, "direction", [](const auto& model)
                       { return exportColumn(model->template table<GravityLoadTable>().directionColumn()); });
        registerColumn(catalogue, type, "mainBulkFlag", [](const auto& model)
                       { return exportColumn(model->template table<GravityLoadTable>().mainBulkFlagColumn()); });
    }
    {
        using model::HydrostaticCavityLoadTable;
        const std::uint16_t type = model::typeIndexOf<HydrostaticCavityLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HydrostaticCavityLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<HydrostaticCavityLoadTable>().propIdColumn()); });
        registerColumn(
            catalogue, type, "nominalPressure", [](const auto& model)
            { return exportColumn(model->template table<HydrostaticCavityLoadTable>().nominalPressureColumn()); });
        registerColumn(catalogue, type, "gravity", [](const auto& model)
                       { return exportColumn(model->template table<HydrostaticCavityLoadTable>().gravityColumn()); });
        registerColumn(
            catalogue, type, "punchOption", [](const auto& model)
            { return exportColumn(model->template table<HydrostaticCavityLoadTable>().punchOptionColumn()); });
        registerColumn(
            catalogue, type, "punchPressureLoads", [](const auto& model)
            { return exportColumn(model->template table<HydrostaticCavityLoadTable>().punchPressureLoadsColumn()); });
        registerColumn(
            catalogue, type, "overrideOption", [](const auto& model)
            { return exportColumn(model->template table<HydrostaticCavityLoadTable>().overrideOptionColumn()); });
        registerColumn(
            catalogue, type, "frameOrNode", [](const auto& model)
            { return exportColumn(model->template table<HydrostaticCavityLoadTable>().frameOrNodeColumn()); });
        registerColumn(
            catalogue, type, "baseHeight", [](const auto& model)
            { return exportColumn(model->template table<HydrostaticCavityLoadTable>().baseHeightColumn()); });
        registerColumn(
            catalogue, type, "heightAbove", [](const auto& model)
            { return exportColumn(model->template table<HydrostaticCavityLoadTable>().heightAboveColumn()); });
    }
    {
        using model::HydrostaticLoadTable;
        const std::uint16_t type = model::typeIndexOf<HydrostaticLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HydrostaticLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "nominalPressure", [](const auto& model)
                       { return exportColumn(model->template table<HydrostaticLoadTable>().nominalPressureColumn()); });
        registerColumn(catalogue, type, "density", [](const auto& model)
                       { return exportColumn(model->template table<HydrostaticLoadTable>().densityColumn()); });
        registerColumn(catalogue, type, "gravity", [](const auto& model)
                       { return exportColumn(model->template table<HydrostaticLoadTable>().gravityColumn()); });
        registerColumn(catalogue, type, "elementSetId", [](const auto& model)
                       { return exportColumn(model->template table<HydrostaticLoadTable>().elementSetIdColumn()); });
        registerColumn(catalogue, type, "nodeSetId", [](const auto& model)
                       { return exportColumn(model->template table<HydrostaticLoadTable>().nodeSetIdColumn()); });
        registerColumn(
            catalogue, type, "punchPressureLoads", [](const auto& model)
            { return exportColumn(model->template table<HydrostaticLoadTable>().punchPressureLoadsColumn()); });
        registerColumn(catalogue, type, "overrideOption", [](const auto& model)
                       { return exportColumn(model->template table<HydrostaticLoadTable>().overrideOptionColumn()); });
        registerColumn(catalogue, type, "frameOrNode", [](const auto& model)
                       { return exportColumn(model->template table<HydrostaticLoadTable>().frameOrNodeColumn()); });
        registerColumn(catalogue, type, "baseHeight", [](const auto& model)
                       { return exportColumn(model->template table<HydrostaticLoadTable>().baseHeightColumn()); });
        registerColumn(catalogue, type, "heightAbove", [](const auto& model)
                       { return exportColumn(model->template table<HydrostaticLoadTable>().heightAboveColumn()); });
        registerColumn(
            catalogue, type, "controlNodeSetId", [](const auto& model)
            { return exportColumn(model->template table<HydrostaticLoadTable>().controlNodeSetIdColumn()); });
    }
    {
        using model::ImperfectionCaseTable;
        const std::uint16_t type = model::typeIndexOf<ImperfectionCaseTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ImperfectionCaseTable>().idColumn()); });
        registerColumn(catalogue, type, "runs", [](const auto& model)
                       { return exportVariableColumn(model->template table<ImperfectionCaseTable>().runsColumn()); });
    }
    {
        using model::ImperfectionGeometryTable;
        const std::uint16_t type = model::typeIndexOf<ImperfectionGeometryTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ImperfectionGeometryTable>().idColumn()); });
    }
    {
        using model::LoadCombinationTable;
        const std::uint16_t type = model::typeIndexOf<LoadCombinationTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<LoadCombinationTable>().idColumn()); });
        registerColumn(catalogue, type, "scaleFactor", [](const auto& model)
                       { return exportColumn(model->template table<LoadCombinationTable>().scaleColumn()); });
        registerColumn(
            catalogue, type, "termScale", [](const auto& model)
            { return exportVariableColumn(model->template table<LoadCombinationTable>().termScaleColumn()); });
        registerColumn(
            catalogue, type, "termLoad", [](const auto& model)
            { return exportVariableColumn(model->template table<LoadCombinationTable>().termLoadColumn()); });
    }
    {
        using model::LoadCaseIdLabelTable;
        const std::uint16_t type = model::typeIndexOf<LoadCaseIdLabelTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<LoadCaseIdLabelTable>().idColumn()); });
    }
    {
        using model::LoadCaseNameLabelTable;
        const std::uint16_t type = model::typeIndexOf<LoadCaseNameLabelTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<LoadCaseNameLabelTable>().idColumn()); });
    }
    {
        using model::LoadCaseSubcaseLabelTable;
        const std::uint16_t type = model::typeIndexOf<LoadCaseSubcaseLabelTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<LoadCaseSubcaseLabelTable>().idColumn()); });
    }
    {
        using model::CyclicHarmonicLoadTable;
        const std::uint16_t type = model::typeIndexOf<CyclicHarmonicLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CyclicHarmonicLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "scaleFactor", [](const auto& model)
                       { return exportColumn(model->template table<CyclicHarmonicLoadTable>().scaleFactorColumn()); });
        registerColumn(catalogue, type, "harmonicId", [](const auto& model)
                       { return exportColumn(model->template table<CyclicHarmonicLoadTable>().harmonicIdColumn()); });
        registerColumn(catalogue, type, "harmonicType", [](const auto& model)
                       { return exportColumn(model->template table<CyclicHarmonicLoadTable>().harmonicTypeColumn()); });
        registerColumn(catalogue, type, "scaleFactor1", [](const auto& model)
                       { return exportColumn(model->template table<CyclicHarmonicLoadTable>().scaleFactor1Column()); });
        registerColumn(catalogue, type, "loadSetId1", [](const auto& model)
                       { return exportColumn(model->template table<CyclicHarmonicLoadTable>().loadSetId1Column()); });
        registerColumn(catalogue, type, "scaleFactor2", [](const auto& model)
                       { return exportColumn(model->template table<CyclicHarmonicLoadTable>().scaleFactor2Column()); });
        registerColumn(catalogue, type, "loadSetId2", [](const auto& model)
                       { return exportColumn(model->template table<CyclicHarmonicLoadTable>().loadSetId2Column()); });
    }
    {
        using model::CyclicSegmentLoadTable;
        const std::uint16_t type = model::typeIndexOf<CyclicSegmentLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CyclicSegmentLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "scaleFactor", [](const auto& model)
                       { return exportColumn(model->template table<CyclicSegmentLoadTable>().scaleFactorColumn()); });
        registerColumn(catalogue, type, "segmentId", [](const auto& model)
                       { return exportColumn(model->template table<CyclicSegmentLoadTable>().segmentIdColumn()); });
        registerColumn(catalogue, type, "segmentType", [](const auto& model)
                       { return exportColumn(model->template table<CyclicSegmentLoadTable>().segmentTypeColumn()); });
        registerColumn(catalogue, type, "scaleFactor1", [](const auto& model)
                       { return exportColumn(model->template table<CyclicSegmentLoadTable>().scaleFactor1Column()); });
        registerColumn(catalogue, type, "loadSetId1", [](const auto& model)
                       { return exportColumn(model->template table<CyclicSegmentLoadTable>().loadSetId1Column()); });
        registerColumn(catalogue, type, "scaleFactor2", [](const auto& model)
                       { return exportColumn(model->template table<CyclicSegmentLoadTable>().scaleFactor2Column()); });
        registerColumn(catalogue, type, "loadSetId2", [](const auto& model)
                       { return exportColumn(model->template table<CyclicSegmentLoadTable>().loadSetId2Column()); });
    }
    {
        using model::CyclicAzimuthTableLoadTable;
        const std::uint16_t type = model::typeIndexOf<CyclicAzimuthTableLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CyclicAzimuthTableLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "tableId1", [](const auto& model)
                       { return exportColumn(model->template table<CyclicAzimuthTableLoadTable>().tableId1Column()); });
        registerColumn(
            catalogue, type, "loadSetId1", [](const auto& model)
            { return exportColumn(model->template table<CyclicAzimuthTableLoadTable>().loadSetId1Column()); });
        registerColumn(
            catalogue, type, "methodName1", [](const auto& model)
            { return exportColumn(model->template table<CyclicAzimuthTableLoadTable>().methodName1Column()); });
        registerColumn(catalogue, type, "tableId2", [](const auto& model)
                       { return exportColumn(model->template table<CyclicAzimuthTableLoadTable>().tableId2Column()); });
        registerColumn(
            catalogue, type, "loadSetId2", [](const auto& model)
            { return exportColumn(model->template table<CyclicAzimuthTableLoadTable>().loadSetId2Column()); });
        registerColumn(
            catalogue, type, "methodName2", [](const auto& model)
            { return exportColumn(model->template table<CyclicAzimuthTableLoadTable>().methodName2Column()); });
    }
    {
        using model::StaticLoadSequenceTable;
        const std::uint16_t type = model::typeIndexOf<StaticLoadSequenceTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<StaticLoadSequenceTable>().idColumn()); });
        registerColumn(catalogue, type, "excitationId", [](const auto& model)
                       { return exportColumn(model->template table<StaticLoadSequenceTable>().excitationIdColumn()); });
        registerColumn(catalogue, type, "loadId", [](const auto& model)
                       { return exportColumn(model->template table<StaticLoadSequenceTable>().loadIdColumn()); });
        registerColumn(catalogue, type, "tableId", [](const auto& model)
                       { return exportColumn(model->template table<StaticLoadSequenceTable>().tableIdColumn()); });
    }
    {
        using model::AxisymRingMomentLoadTable;
        const std::uint16_t type = model::typeIndexOf<AxisymRingMomentLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymRingMomentLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "ringId", [](const auto& model)
                       { return exportColumn(model->template table<AxisymRingMomentLoadTable>().ringIdColumn()); });
        registerColumn(catalogue, type, "harmonicId", [](const auto& model)
                       { return exportColumn(model->template table<AxisymRingMomentLoadTable>().harmonicIdColumn()); });
        registerColumn(
            catalogue, type, "harmonicEnd", [](const auto& model)
            { return exportColumn(model->template table<AxisymRingMomentLoadTable>().harmonicEndColumn()); });
        registerColumn(
            catalogue, type, "scaleFactor", [](const auto& model)
            { return exportColumn(model->template table<AxisymRingMomentLoadTable>().scaleFactorColumn()); });
        registerColumn(
            catalogue, type, "radialMoment", [](const auto& model)
            { return exportColumn(model->template table<AxisymRingMomentLoadTable>().radialMomentColumn()); });
        registerColumn(
            catalogue, type, "azimuthalMoment", [](const auto& model)
            { return exportColumn(model->template table<AxisymRingMomentLoadTable>().azimuthalMomentColumn()); });
        registerColumn(
            catalogue, type, "axialMoment", [](const auto& model)
            { return exportColumn(model->template table<AxisymRingMomentLoadTable>().axialMomentColumn()); });
    }
    {
        using model::PointMomentLoadTable;
        const std::uint16_t type = model::typeIndexOf<PointMomentLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PointMomentLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportColumn(model->template table<PointMomentLoadTable>().nodeColumn()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<PointMomentLoadTable>().frameIdColumn()); });
        registerColumn(catalogue, type, "magnitude", [](const auto& model)
                       { return exportColumn(model->template table<PointMomentLoadTable>().magnitudeColumn()); });
        registerColumn(catalogue, type, "direction", [](const auto& model)
                       { return exportColumn(model->template table<PointMomentLoadTable>().directionColumn()); });
    }
    {
        using model::PointMomentLoad1Table;
        const std::uint16_t type = model::typeIndexOf<PointMomentLoad1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PointMomentLoad1Table>().idColumn()); });
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportColumn(model->template table<PointMomentLoad1Table>().nodeColumn()); });
        registerColumn(catalogue, type, "magnitude", [](const auto& model)
                       { return exportColumn(model->template table<PointMomentLoad1Table>().magnitudeColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<PointMomentLoad1Table>().node1Column()); });
        registerColumn(catalogue, type, "node2", [](const auto& model)
                       { return exportColumn(model->template table<PointMomentLoad1Table>().node2Column()); });
    }
    {
        using model::PointMomentLoad2Table;
        const std::uint16_t type = model::typeIndexOf<PointMomentLoad2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PointMomentLoad2Table>().idColumn()); });
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportColumn(model->template table<PointMomentLoad2Table>().nodeColumn()); });
        registerColumn(catalogue, type, "magnitude", [](const auto& model)
                       { return exportColumn(model->template table<PointMomentLoad2Table>().magnitudeColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<PointMomentLoad2Table>().node1Column()); });
        registerColumn(catalogue, type, "node2", [](const auto& model)
                       { return exportColumn(model->template table<PointMomentLoad2Table>().node2Column()); });
        registerColumn(catalogue, type, "node3", [](const auto& model)
                       { return exportColumn(model->template table<PointMomentLoad2Table>().node3Column()); });
        registerColumn(catalogue, type, "node4", [](const auto& model)
                       { return exportColumn(model->template table<PointMomentLoad2Table>().node4Column()); });
    }
    {
        using model::PointsPressureLoadTable;
        const std::uint16_t type = model::typeIndexOf<PointsPressureLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PointsPressureLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "pressure", [](const auto& model)
                       { return exportColumn(model->template table<PointsPressureLoadTable>().pressureColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<PointsPressureLoadTable>().nodesColumn()); });
    }
    {
        using model::BeamDistributedLoadTable;
        const std::uint16_t type = model::typeIndexOf<BeamDistributedLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BeamDistributedLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "elemId", [](const auto& model)
                       { return exportColumn(model->template table<BeamDistributedLoadTable>().elemIdColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportColumn(model->template table<BeamDistributedLoadTable>().kindColumn()); });
        registerColumn(catalogue, type, "scaleFactor", [](const auto& model)
                       { return exportColumn(model->template table<BeamDistributedLoadTable>().scaleFactorColumn()); });
        registerColumn(catalogue, type, "xCoord1", [](const auto& model)
                       { return exportColumn(model->template table<BeamDistributedLoadTable>().xCoord1Column()); });
        registerColumn(catalogue, type, "pressure1", [](const auto& model)
                       { return exportColumn(model->template table<BeamDistributedLoadTable>().pressure1Column()); });
        registerColumn(catalogue, type, "xCoord2", [](const auto& model)
                       { return exportColumn(model->template table<BeamDistributedLoadTable>().xCoord2Column()); });
        registerColumn(catalogue, type, "pressure2", [](const auto& model)
                       { return exportColumn(model->template table<BeamDistributedLoadTable>().pressure2Column()); });
    }
    {
        using model::ShellPressureLoadTable;
        const std::uint16_t type = model::typeIndexOf<ShellPressureLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ShellPressureLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "pressure", [](const auto& model)
                       { return exportColumn(model->template table<ShellPressureLoadTable>().pressureColumn()); });
        registerColumn(catalogue, type, "runs", [](const auto& model)
                       { return exportVariableColumn(model->template table<ShellPressureLoadTable>().runsColumn()); });
    }
    {
        using model::FacePressureLoadTable;
        const std::uint16_t type = model::typeIndexOf<FacePressureLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FacePressureLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "elemId", [](const auto& model)
                       { return exportColumn(model->template table<FacePressureLoadTable>().elemIdColumn()); });
        registerColumn(catalogue, type, "elemId2", [](const auto& model)
                       { return exportColumn(model->template table<FacePressureLoadTable>().elemId2Column()); });
        registerColumn(catalogue, type, "pressures", [](const auto& model)
                       { return exportColumn(model->template table<FacePressureLoadTable>().pressuresColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<FacePressureLoadTable>().node1Column()); });
        registerColumn(catalogue, type, "node3Or4", [](const auto& model)
                       { return exportColumn(model->template table<FacePressureLoadTable>().node3Or4Column()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<FacePressureLoadTable>().frameIdColumn()); });
        registerColumn(catalogue, type, "direction", [](const auto& model)
                       { return exportColumn(model->template table<FacePressureLoadTable>().directionColumn()); });
        registerColumn(catalogue, type, "surfaceOrLine", [](const auto& model)
                       { return exportColumn(model->template table<FacePressureLoadTable>().kindColumn()); });
        registerColumn(catalogue, type, "lineLoadDirection", [](const auto& model)
                       { return exportColumn(model->template table<FacePressureLoadTable>().lineDirectionColumn()); });
    }
    {
        using model::Beam3DistributedLoadTable;
        const std::uint16_t type = model::typeIndexOf<Beam3DistributedLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<Beam3DistributedLoadTable>().idColumn()); });
    }
    {
        using model::AxisymTractionLoadTable;
        const std::uint16_t type = model::typeIndexOf<AxisymTractionLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymTractionLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "elemId", [](const auto& model)
                       { return exportColumn(model->template table<AxisymTractionLoadTable>().elemIdColumn()); });
        registerColumn(catalogue, type, "tractionA", [](const auto& model)
                       { return exportColumn(model->template table<AxisymTractionLoadTable>().tractionAColumn()); });
        registerColumn(catalogue, type, "tractionB", [](const auto& model)
                       { return exportColumn(model->template table<AxisymTractionLoadTable>().tractionBColumn()); });
        registerColumn(catalogue, type, "nodeA", [](const auto& model)
                       { return exportColumn(model->template table<AxisymTractionLoadTable>().nodeAColumn()); });
        registerColumn(catalogue, type, "nodeB", [](const auto& model)
                       { return exportColumn(model->template table<AxisymTractionLoadTable>().nodeBColumn()); });
        registerColumn(
            catalogue, type, "tractionAngle", [](const auto& model)
            { return exportColumn(model->template table<AxisymTractionLoadTable>().tractionAngleColumn()); });
    }
    {
        using model::AxisymConePressureLoadTable;
        const std::uint16_t type = model::typeIndexOf<AxisymConePressureLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymConePressureLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "pressure", [](const auto& model)
                       { return exportColumn(model->template table<AxisymConePressureLoadTable>().pressureColumn()); });
        registerColumn(catalogue, type, "ringId1", [](const auto& model)
                       { return exportColumn(model->template table<AxisymConePressureLoadTable>().ringId1Column()); });
        registerColumn(catalogue, type, "ringId2", [](const auto& model)
                       { return exportColumn(model->template table<AxisymConePressureLoadTable>().ringId2Column()); });
        registerColumn(
            catalogue, type, "phiAngle1", [](const auto& model)
            { return exportColumn(model->template table<AxisymConePressureLoadTable>().phiAngle1Column()); });
        registerColumn(
            catalogue, type, "phiAngle2", [](const auto& model)
            { return exportColumn(model->template table<AxisymConePressureLoadTable>().phiAngle2Column()); });
    }
    {
        using model::RotationalForceLoadTable;
        const std::uint16_t type = model::typeIndexOf<RotationalForceLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RotationalForceLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportColumn(model->template table<RotationalForceLoadTable>().nodeColumn()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<RotationalForceLoadTable>().frameIdColumn()); });
        registerColumn(catalogue, type, "magnitude", [](const auto& model)
                       { return exportColumn(model->template table<RotationalForceLoadTable>().magnitudeColumn()); });
        registerColumn(catalogue, type, "axis", [](const auto& model)
                       { return exportColumn(model->template table<RotationalForceLoadTable>().axisColumn()); });
        registerColumn(catalogue, type, "methodName", [](const auto& model)
                       { return exportColumn(model->template table<RotationalForceLoadTable>().methodNameColumn()); });
        registerColumn(
            catalogue, type, "angularAcceleration", [](const auto& model)
            { return exportColumn(model->template table<RotationalForceLoadTable>().angularAccelerationColumn()); });
        registerColumn(
            catalogue, type, "mainBulkFlag", [](const auto& model)
            { return exportColumn(model->template table<RotationalForceLoadTable>().mainBulkFlagColumn()); });
        registerColumn(catalogue, type, "regionId", [](const auto& model)
                       { return exportColumn(model->template table<RotationalForceLoadTable>().regionIdColumn()); });
    }
    {
        using model::ScalarPointLoadTable;
        const std::uint16_t type = model::typeIndexOf<ScalarPointLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ScalarPointLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportVariableColumn(model->template table<ScalarPointLoadTable>().nodeColumn()); });
        registerColumn(
            catalogue, type, "magnitude", [](const auto& model)
            { return exportVariableColumn(model->template table<ScalarPointLoadTable>().magnitudeColumn()); });
    }
    {
        using model::NodeTemperatureLoadTable;
        const std::uint16_t type = model::typeIndexOf<NodeTemperatureLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NodeTemperatureLoadTable>().idColumn()); });
        registerColumn(
            catalogue, type, "node", [](const auto& model)
            { return exportVariableColumn(model->template table<NodeTemperatureLoadTable>().nodeColumn()); });
        registerColumn(
            catalogue, type, "temperature", [](const auto& model)
            { return exportVariableColumn(model->template table<NodeTemperatureLoadTable>().temperatureColumn()); });
    }
    {
        using model::AxisymConeTemperatureLoadTable;
        const std::uint16_t type = model::typeIndexOf<AxisymConeTemperatureLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymConeTemperatureLoadTable>().idColumn()); });
        registerColumn(
            catalogue, type, "ringId", [](const auto& model)
            { return exportColumn(model->template table<AxisymConeTemperatureLoadTable>().ringIdColumn()); });
        registerColumn(catalogue, type, "phi", [](const auto& model)
                       { return exportColumn(model->template table<AxisymConeTemperatureLoadTable>().phiColumn()); });
        registerColumn(
            catalogue, type, "temperature", [](const auto& model)
            { return exportColumn(model->template table<AxisymConeTemperatureLoadTable>().temperatureColumn()); });
    }
    {
        using model::Beam3TemperatureLoadTable;
        const std::uint16_t type = model::typeIndexOf<Beam3TemperatureLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<Beam3TemperatureLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "elemId", [](const auto& model)
                       { return exportColumn(model->template table<Beam3TemperatureLoadTable>().elemIdColumn()); });
        registerColumn(
            catalogue, type, "temperature", [](const auto& model)
            { return exportColumn(model->template table<Beam3TemperatureLoadTable>().temperatureColumn()); });
        registerColumn(catalogue, type, "gradientY", [](const auto& model)
                       { return exportColumn(model->template table<Beam3TemperatureLoadTable>().gradientYColumn()); });
        registerColumn(catalogue, type, "gradientZ", [](const auto& model)
                       { return exportColumn(model->template table<Beam3TemperatureLoadTable>().gradientZColumn()); });
        registerColumn(
            catalogue, type, "temperatureC", [](const auto& model)
            { return exportColumn(model->template table<Beam3TemperatureLoadTable>().temperatureCColumn()); });
        registerColumn(
            catalogue, type, "temperatureD", [](const auto& model)
            { return exportColumn(model->template table<Beam3TemperatureLoadTable>().temperatureDColumn()); });
        registerColumn(
            catalogue, type, "temperatureE", [](const auto& model)
            { return exportColumn(model->template table<Beam3TemperatureLoadTable>().temperatureEColumn()); });
        registerColumn(
            catalogue, type, "temperatureF", [](const auto& model)
            { return exportColumn(model->template table<Beam3TemperatureLoadTable>().temperatureFColumn()); });
        registerColumn(
            catalogue, type, "runs", [](const auto& model)
            { return exportVariableColumn(model->template table<Beam3TemperatureLoadTable>().runsColumn()); });
    }
    {
        using model::DefaultTemperatureLoadTable;
        const std::uint16_t type = model::typeIndexOf<DefaultTemperatureLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DefaultTemperatureLoadTable>().idColumn()); });
        registerColumn(
            catalogue, type, "temperature", [](const auto& model)
            { return exportColumn(model->template table<DefaultTemperatureLoadTable>().temperatureColumn()); });
    }
    {
        using model::HeatShellInitialTemperatureTable;
        const std::uint16_t type = model::typeIndexOf<HeatShellInitialTemperatureTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HeatShellInitialTemperatureTable>().idColumn()); });
        registerColumn(
            catalogue, type, "node", [](const auto& model)
            { return exportVariableColumn(model->template table<HeatShellInitialTemperatureTable>().nodeColumn()); });
        registerColumn(catalogue, type, "components",
                       [](const auto& model)
                       {
                           return exportVariableColumn(
                               model->template table<HeatShellInitialTemperatureTable>().componentsColumn());
                       });
        registerColumn(catalogue, type, "temperature",
                       [](const auto& model)
                       {
                           return exportVariableColumn(
                               model->template table<HeatShellInitialTemperatureTable>().temperatureColumn());
                       });
    }
    {
        using model::PlateTemperatureLoadTable;
        const std::uint16_t type = model::typeIndexOf<PlateTemperatureLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PlateTemperatureLoadTable>().idColumn()); });
        registerColumn(
            catalogue, type, "meanTemperature", [](const auto& model)
            { return exportColumn(model->template table<PlateTemperatureLoadTable>().meanTemperatureColumn()); });
        registerColumn(
            catalogue, type, "temperatureGradient", [](const auto& model)
            { return exportColumn(model->template table<PlateTemperatureLoadTable>().temperatureGradientColumn()); });
        registerColumn(
            catalogue, type, "temperature1", [](const auto& model)
            { return exportColumn(model->template table<PlateTemperatureLoadTable>().temperature1Column()); });
        registerColumn(
            catalogue, type, "temperature2", [](const auto& model)
            { return exportColumn(model->template table<PlateTemperatureLoadTable>().temperature2Column()); });
        registerColumn(
            catalogue, type, "runs", [](const auto& model)
            { return exportVariableColumn(model->template table<PlateTemperatureLoadTable>().runsColumn()); });
    }
    {
        using model::LineElemTemperatureLoadTable;
        const std::uint16_t type = model::typeIndexOf<LineElemTemperatureLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<LineElemTemperatureLoadTable>().idColumn()); });
        registerColumn(
            catalogue, type, "temperatureA", [](const auto& model)
            { return exportColumn(model->template table<LineElemTemperatureLoadTable>().temperatureAColumn()); });
        registerColumn(
            catalogue, type, "temperatureB", [](const auto& model)
            { return exportColumn(model->template table<LineElemTemperatureLoadTable>().temperatureBColumn()); });
        registerColumn(
            catalogue, type, "gradientY", [](const auto& model)
            { return exportColumn(model->template table<LineElemTemperatureLoadTable>().gradientYColumn()); });
        registerColumn(
            catalogue, type, "gradientZ", [](const auto& model)
            { return exportColumn(model->template table<LineElemTemperatureLoadTable>().gradientZColumn()); });
        registerColumn(catalogue, type, "pointsA", [](const auto& model)
                       { return exportColumn(model->template table<LineElemTemperatureLoadTable>().pointsAColumn()); });
        registerColumn(catalogue, type, "pointsB", [](const auto& model)
                       { return exportColumn(model->template table<LineElemTemperatureLoadTable>().pointsBColumn()); });
        registerColumn(
            catalogue, type, "runs", [](const auto& model)
            { return exportVariableColumn(model->template table<LineElemTemperatureLoadTable>().runsColumn()); });
    }
    {
        using model::TemperatureTimeGroupTable;
        const std::uint16_t type = model::typeIndexOf<TemperatureTimeGroupTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TemperatureTimeGroupTable>().idColumn()); });
    }
    {
        using model::TimeTemperatureLoadTable;
        const std::uint16_t type = model::typeIndexOf<TimeTemperatureLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TimeTemperatureLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "groupId", [](const auto& model)
                       { return exportColumn(model->template table<TimeTemperatureLoadTable>().groupIdColumn()); });
        registerColumn(catalogue, type, "tableId", [](const auto& model)
                       { return exportColumn(model->template table<TimeTemperatureLoadTable>().tableIdColumn()); });
    }
    {
        using model::WettedSurfaceLoadTable;
        const std::uint16_t type = model::typeIndexOf<WettedSurfaceLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<WettedSurfaceLoadTable>().idColumn()); });
    }
}

} // namespace oor::query::column_export
