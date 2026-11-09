// The stored columns of the cards of the family "nodes", for the query engine (ColumnExport.hpp).
// One block a card type: its type index, then a getter a column.

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/nodes/acousticSlotControl/AcousticSlotControlTable.hpp"
#include "oor/model/cards/nodes/acousticSlotFluidPoint/AcousticSlotFluidPointTable.hpp"
#include "oor/model/cards/nodes/acousticSlotSurfacePoint/AcousticSlotSurfacePointTable.hpp"
#include "oor/model/cards/nodes/adaptiveRefineControl/AdaptiveRefineControlTable.hpp"
#include "oor/model/cards/nodes/adaptiveRefineCriterion/AdaptiveRefineCriterionTable.hpp"
#include "oor/model/cards/nodes/associativeNode/AssociativeNodeTable.hpp"
#include "oor/model/cards/nodes/axisymConeControl/AxisymConeControlTable.hpp"
#include "oor/model/cards/nodes/axisymConePoint/AxisymConePointTable.hpp"
#include "oor/model/cards/nodes/axisymConeRing/AxisymConeRingTable.hpp"
#include "oor/model/cards/nodes/axisymFluidControl/AxisymFluidControlTable.hpp"
#include "oor/model/cards/nodes/axisymFluidFreePoint/AxisymFluidFreePointTable.hpp"
#include "oor/model/cards/nodes/axisymFluidPressurePoint/AxisymFluidPressurePointTable.hpp"
#include "oor/model/cards/nodes/axisymFluidRing/AxisymFluidRingTable.hpp"
#include "oor/model/cards/nodes/axisymFluidRingNode/AxisymFluidRingNodeTable.hpp"
#include "oor/model/cards/nodes/extraPoint/ExtraPointTable.hpp"
#include "oor/model/cards/nodes/geomPoint/GeomPointTable.hpp"
#include "oor/model/cards/nodes/moduleBolt/ModuleBoltTable.hpp"
#include "oor/model/cards/nodes/moduleBoundaryNodes/ModuleBoundaryNodesTable.hpp"
#include "oor/model/cards/nodes/moduleBrakeSystem/ModuleBrakeSystemTable.hpp"
#include "oor/model/cards/nodes/moduleBulkOptions/ModuleBulkOptionsTable.hpp"
#include "oor/model/cards/nodes/moduleBushing/ModuleBushingTable.hpp"
#include "oor/model/cards/nodes/moduleConnection/ModuleConnectionTable.hpp"
#include "oor/model/cards/nodes/moduleContactPair/ModuleContactPairTable.hpp"
#include "oor/model/cards/nodes/moduleContactTable/ModuleContactTableTable.hpp"
#include "oor/model/cards/nodes/moduleDirectMatrix/ModuleDirectMatrixTable.hpp"
#include "oor/model/cards/nodes/moduleExcludedNodes/ModuleExcludedNodesTable.hpp"
#include "oor/model/cards/nodes/moduleFastener/ModuleFastenerTable.hpp"
#include "oor/model/cards/nodes/moduleFluidSet/ModuleFluidSetTable.hpp"
#include "oor/model/cards/nodes/moduleLabel/ModuleLabelTable.hpp"
#include "oor/model/cards/nodes/moduleLocation/ModuleLocationTable.hpp"
#include "oor/model/cards/nodes/moduleMirror1/ModuleMirror1Table.hpp"
#include "oor/model/cards/nodes/moduleMirror2/ModuleMirror2Table.hpp"
#include "oor/model/cards/nodes/moduleMirrorPlane/ModuleMirrorPlaneTable.hpp"
#include "oor/model/cards/nodes/moduleMoveSequence/ModuleMoveSequenceTable.hpp"
#include "oor/model/cards/nodes/moduleMultiPointConstr/ModuleMultiPointConstrTable.hpp"
#include "oor/model/cards/nodes/moduleRigidBodyStar/ModuleRigidBodyStarTable.hpp"
#include "oor/model/cards/nodes/moduleRigidInterpolation/ModuleRigidInterpolationTable.hpp"
#include "oor/model/cards/nodes/moduleRigidJoint/ModuleRigidJointTable.hpp"
#include "oor/model/cards/nodes/moduleRigidRod/ModuleRigidRodTable.hpp"
#include "oor/model/cards/nodes/moduleRotation1/ModuleRotation1Table.hpp"
#include "oor/model/cards/nodes/moduleRotation2/ModuleRotation2Table.hpp"
#include "oor/model/cards/nodes/moduleSeam/ModuleSeamTable.hpp"
#include "oor/model/cards/nodes/moduleStructureSet/ModuleStructureSetTable.hpp"
#include "oor/model/cards/nodes/moduleTranslation/ModuleTranslationTable.hpp"
#include "oor/model/cards/nodes/moduleWeld/ModuleWeldTable.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/nodes/nodeDefaults/NodeDefaultsTable.hpp"
#include "oor/model/cards/nodes/nodeSequence/NodeSequenceTable.hpp"
#include "oor/model/cards/nodes/scalarPoint/ScalarPointTable.hpp"
#include "oor/model/cards/nodes/shellNormal/ShellNormalTable.hpp"
#include "oor/model/cards/rigid_elements/ComponentSets.hpp"
#include "oor/query/providers/ColumnExport.hpp"

namespace oor::query::column_export
{

void registerNodesColumns(StoredColumnCatalogue& catalogue)
{
    {
        using model::AxisymConeControlTable;
        const std::uint16_t type = model::typeIndexOf<AxisymConeControlTable>();
        registerColumn(
            catalogue, type, "highestHarmonic", [](const auto& model)
            { return exportColumn(model->template table<AxisymConeControlTable>().highestHarmonicColumn()); });
    }
    {
        using model::AxisymFluidControlTable;
        const std::uint16_t type = model::typeIndexOf<AxisymFluidControlTable>();
        registerColumn(
            catalogue, type, "harmonicCount", [](const auto& model)
            { return exportColumn(model->template table<AxisymFluidControlTable>().harmonicCountColumn()); });
    }
    {
        using model::AcousticSlotControlTable;
        const std::uint16_t type = model::typeIndexOf<AcousticSlotControlTable>();
        registerColumn(
            catalogue, type, "defaultDensity", [](const auto& model)
            { return exportColumn(model->template table<AcousticSlotControlTable>().defaultDensityColumn()); });
        registerColumn(
            catalogue, type, "defaultBulkModulus", [](const auto& model)
            { return exportColumn(model->template table<AcousticSlotControlTable>().defaultBulkModulusColumn()); });
        registerColumn(catalogue, type, "harmonic", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSlotControlTable>().harmonicColumn()); });
        registerColumn(
            catalogue, type, "defaultSlotWidth", [](const auto& model)
            { return exportColumn(model->template table<AcousticSlotControlTable>().defaultSlotWidthColumn()); });
        registerColumn(
            catalogue, type, "defaultSlotCount", [](const auto& model)
            { return exportColumn(model->template table<AcousticSlotControlTable>().defaultSlotCountColumn()); });
    }
    {
        using model::ExtraPointTable;
        const std::uint16_t type = model::typeIndexOf<ExtraPointTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ExtraPointTable>().idColumn()); });
    }
    {
        using model::AxisymFluidFreePointTable;
        const std::uint16_t type = model::typeIndexOf<AxisymFluidFreePointTable>();
        registerColumn(
            catalogue, type, "fluidPointId", [](const auto& model)
            { return exportColumn(model->template table<AxisymFluidFreePointTable>().fluidPointIdColumn()); });
        registerColumn(catalogue, type, "pointId", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluidFreePointTable>().pointIdColumn()); });
        registerColumn(catalogue, type, "phiAngle", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluidFreePointTable>().phiAngleColumn()); });
    }
    {
        using model::NodeDefaultsTable;
        const std::uint16_t type = model::typeIndexOf<NodeDefaultsTable>();
        registerColumn(catalogue, type, "positionFrameId", [](const auto& model)
                       { return exportColumn(model->template table<NodeDefaultsTable>().positionFrameIdColumn()); });
        registerColumn(
            catalogue, type, "displacementFrameId", [](const auto& model)
            { return exportColumn(model->template table<NodeDefaultsTable>().displacementFrameIdColumn()); });
        registerColumn(
            catalogue, type, "permanentConstraints", [](const auto& model)
            { return exportColumn(model->template table<NodeDefaultsTable>().permanentConstraintsColumn()); });
        registerColumn(catalogue, type, "superelementId", [](const auto& model)
                       { return exportColumn(model->template table<NodeDefaultsTable>().superelementIdColumn()); });
    }
    {
        using model::NodeTable;
        const std::uint16_t type = model::typeIndexOf<NodeTable>();
        registerColumn(catalogue, type, "id",
                       [](const auto& model) { return exportColumn(model->template table<NodeTable>().idColumn()); });
        registerColumn(catalogue, type, "positionFrameId", [](const auto& model)
                       { return exportColumn(model->template table<NodeTable>().positionFrameIdColumn()); });
        registerColumn(catalogue, type, "xyz",
                       [](const auto& model) { return exportColumn(model->template table<NodeTable>().xyzColumn()); });
        registerColumn(catalogue, type, "displacementFrameId", [](const auto& model)
                       { return exportColumn(model->template table<NodeTable>().displacementFrameIdColumn()); });
        registerColumn(catalogue, type, "permanentConstraints", [](const auto& model)
                       { return exportColumn(model->template table<NodeTable>().permanentConstraintsColumn()); });
        registerColumn(catalogue, type, "superelementId", [](const auto& model)
                       { return exportColumn(model->template table<NodeTable>().superelementIdColumn()); });
    }
    {
        using model::AssociativeNodeTable;
        const std::uint16_t type = model::typeIndexOf<AssociativeNodeTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AssociativeNodeTable>().idColumn()); });
        registerColumn(catalogue, type, "origNodeId", [](const auto& model)
                       { return exportColumn(model->template table<AssociativeNodeTable>().origNodeIdColumn()); });
        registerColumn(
            catalogue, type, "partitionName", [](const auto& model)
            { return exportTextColumn(model->template table<AssociativeNodeTable>().partitionNameColumn()); });
    }
    {
        using model::AxisymFluidRingNodeTable;
        const std::uint16_t type = model::typeIndexOf<AxisymFluidRingNodeTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluidRingNodeTable>().idColumn()); });
        registerColumn(catalogue, type, "phiAngle", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluidRingNodeTable>().phiAngleColumn()); });
        registerColumn(
            catalogue, type, "displacementFrameId", [](const auto& model)
            { return exportColumn(model->template table<AxisymFluidRingNodeTable>().displacementFrameIdColumn()); });
        registerColumn(
            catalogue, type, "permanentConstraints", [](const auto& model)
            { return exportColumn(model->template table<AxisymFluidRingNodeTable>().permanentConstraintsColumn()); });
        registerColumn(
            catalogue, type, "fluidPointId", [](const auto& model)
            { return exportColumn(model->template table<AxisymFluidRingNodeTable>().fluidPointIdColumn()); });
    }
    {
        using model::AcousticSlotFluidPointTable;
        const std::uint16_t type = model::typeIndexOf<AcousticSlotFluidPointTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSlotFluidPointTable>().idColumn()); });
        registerColumn(catalogue, type, "radius", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSlotFluidPointTable>().radiusColumn()); });
        registerColumn(catalogue, type, "zCoord", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSlotFluidPointTable>().zCoordColumn()); });
    }
    {
        using model::AcousticSlotSurfacePointTable;
        const std::uint16_t type = model::typeIndexOf<AcousticSlotSurfacePointTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSlotSurfacePointTable>().idColumn()); });
        registerColumn(catalogue, type, "radius", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSlotSurfacePointTable>().radiusColumn()); });
        registerColumn(catalogue, type, "zCoord", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSlotSurfacePointTable>().zCoordColumn()); });
        registerColumn(
            catalogue, type, "slotWidth", [](const auto& model)
            { return exportColumn(model->template table<AcousticSlotSurfacePointTable>().slotWidthColumn()); });
        registerColumn(
            catalogue, type, "fluidPointId", [](const auto& model)
            { return exportColumn(model->template table<AcousticSlotSurfacePointTable>().fluidPointIdColumn()); });
    }
    {
        using model::AdaptiveRefineCriterionTable;
        const std::uint16_t type = model::typeIndexOf<AdaptiveRefineCriterionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AdaptiveRefineCriterionTable>().idColumn()); });
    }
    {
        using model::AdaptiveRefineControlTable;
        const std::uint16_t type = model::typeIndexOf<AdaptiveRefineControlTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AdaptiveRefineControlTable>().idColumn()); });
    }
    {
        using model::ModuleContactPairTable;
        const std::uint16_t type = model::typeIndexOf<ModuleContactPairTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleContactPairTable>().idColumn()); });
    }
    {
        using model::ModuleContactTableTable;
        const std::uint16_t type = model::typeIndexOf<ModuleContactTableTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleContactTableTable>().idColumn()); });
    }
    {
        using model::ModuleBrakeSystemTable;
        const std::uint16_t type = model::typeIndexOf<ModuleBrakeSystemTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleBrakeSystemTable>().idColumn()); });
    }
    {
        using model::ModuleBoundaryNodesTable;
        const std::uint16_t type = model::typeIndexOf<ModuleBoundaryNodesTable>();
        registerColumn(catalogue, type, "moduleIdA", [](const auto& model)
                       { return exportColumn(model->template table<ModuleBoundaryNodesTable>().moduleIdAColumn()); });
    }
    {
        using model::ModuleBoltTable;
        const std::uint16_t type = model::typeIndexOf<ModuleBoltTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleBoltTable>().idColumn()); });
    }
    {
        using model::ModuleBulkOptionsTable;
        const std::uint16_t type = model::typeIndexOf<ModuleBulkOptionsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleBulkOptionsTable>().idColumn()); });
    }
    {
        using model::ModuleBushingTable;
        const std::uint16_t type = model::typeIndexOf<ModuleBushingTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleBushingTable>().idColumn()); });
    }
    {
        using model::ModuleConnectionTable;
        const std::uint16_t type = model::typeIndexOf<ModuleConnectionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleConnectionTable>().idColumn()); });
    }
    {
        using model::ModuleDirectMatrixTable;
        const std::uint16_t type = model::typeIndexOf<ModuleDirectMatrixTable>();
        registerColumn(
            catalogue, type, "entryName", [](const auto& model)
            { return exportTextColumn(model->template table<ModuleDirectMatrixTable>().entryNameColumn()); });
    }
    {
        using model::ModuleExcludedNodesTable;
        const std::uint16_t type = model::typeIndexOf<ModuleExcludedNodesTable>();
        registerColumn(catalogue, type, "moduleIdA", [](const auto& model)
                       { return exportColumn(model->template table<ModuleExcludedNodesTable>().moduleIdAColumn()); });
    }
    {
        using model::ModuleFastenerTable;
        const std::uint16_t type = model::typeIndexOf<ModuleFastenerTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleFastenerTable>().idColumn()); });
    }
    {
        using model::ModuleFluidSetTable;
        const std::uint16_t type = model::typeIndexOf<ModuleFluidSetTable>();
        registerColumn(catalogue, type, "moduleId", [](const auto& model)
                       { return exportColumn(model->template table<ModuleFluidSetTable>().moduleIdColumn()); });
    }
    {
        using model::ModuleLabelTable;
        const std::uint16_t type = model::typeIndexOf<ModuleLabelTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleLabelTable>().idColumn()); });
        registerColumn(catalogue, type, "entryLabel", [](const auto& model)
                       { return exportTextColumn(model->template table<ModuleLabelTable>().entryLabelColumn()); });
    }
    {
        using model::ModuleLocationTable;
        const std::uint16_t type = model::typeIndexOf<ModuleLocationTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleLocationTable>().idColumn()); });
    }
    {
        using model::ModuleMirror1Table;
        const std::uint16_t type = model::typeIndexOf<ModuleMirror1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleMirror1Table>().idColumn()); });
    }
    {
        using model::ModuleMirror2Table;
        const std::uint16_t type = model::typeIndexOf<ModuleMirror2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleMirror2Table>().idColumn()); });
    }
    {
        using model::ModuleMoveSequenceTable;
        const std::uint16_t type = model::typeIndexOf<ModuleMoveSequenceTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleMoveSequenceTable>().idColumn()); });
    }
    {
        using model::ModuleMultiPointConstrTable;
        const std::uint16_t type = model::typeIndexOf<ModuleMultiPointConstrTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleMultiPointConstrTable>().idColumn()); });
    }
    {
        using model::ModuleMirrorPlaneTable;
        const std::uint16_t type = model::typeIndexOf<ModuleMirrorPlaneTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleMirrorPlaneTable>().idColumn()); });
    }
    {
        using model::ModuleRigidBodyStarTable;
        const std::uint16_t type = model::typeIndexOf<ModuleRigidBodyStarTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleRigidBodyStarTable>().idColumn()); });
    }
    {
        using model::ModuleRigidInterpolationTable;
        const std::uint16_t type = model::typeIndexOf<ModuleRigidInterpolationTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleRigidInterpolationTable>().idColumn()); });
    }
    {
        using model::ModuleRigidJointTable;
        const std::uint16_t type = model::typeIndexOf<ModuleRigidJointTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleRigidJointTable>().idColumn()); });
    }
    {
        using model::ModuleRotation1Table;
        const std::uint16_t type = model::typeIndexOf<ModuleRotation1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleRotation1Table>().idColumn()); });
    }
    {
        using model::ModuleRotation2Table;
        const std::uint16_t type = model::typeIndexOf<ModuleRotation2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleRotation2Table>().idColumn()); });
    }
    {
        using model::ModuleRigidRodTable;
        const std::uint16_t type = model::typeIndexOf<ModuleRigidRodTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleRigidRodTable>().idColumn()); });
    }
    {
        using model::ModuleSeamTable;
        const std::uint16_t type = model::typeIndexOf<ModuleSeamTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleSeamTable>().idColumn()); });
    }
    {
        using model::ModuleStructureSetTable;
        const std::uint16_t type = model::typeIndexOf<ModuleStructureSetTable>();
        registerColumn(catalogue, type, "moduleId", [](const auto& model)
                       { return exportColumn(model->template table<ModuleStructureSetTable>().moduleIdColumn()); });
    }
    {
        using model::ModuleTranslationTable;
        const std::uint16_t type = model::typeIndexOf<ModuleTranslationTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleTranslationTable>().idColumn()); });
    }
    {
        using model::ModuleWeldTable;
        const std::uint16_t type = model::typeIndexOf<ModuleWeldTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModuleWeldTable>().idColumn()); });
    }
    {
        using model::GeomPointTable;
        const std::uint16_t type = model::typeIndexOf<GeomPointTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<GeomPointTable>().idColumn()); });
        registerColumn(catalogue, type, "positionFrameId", [](const auto& model)
                       { return exportColumn(model->template table<GeomPointTable>().positionFrameIdColumn()); });
        registerColumn(catalogue, type, "xyz", [](const auto& model)
                       { return exportColumn(model->template table<GeomPointTable>().xyzColumn()); });
    }
    {
        using model::AxisymConePointTable;
        const std::uint16_t type = model::typeIndexOf<AxisymConePointTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymConePointTable>().idColumn()); });
        registerColumn(catalogue, type, "ringId", [](const auto& model)
                       { return exportColumn(model->template table<AxisymConePointTable>().ringIdColumn()); });
        registerColumn(catalogue, type, "phiAngle", [](const auto& model)
                       { return exportColumn(model->template table<AxisymConePointTable>().phiAngleColumn()); });
    }
    {
        using model::AxisymFluidPressurePointTable;
        const std::uint16_t type = model::typeIndexOf<AxisymFluidPressurePointTable>();
        registerColumn(
            catalogue, type, "fluidPointId", [](const auto& model)
            { return exportColumn(model->template table<AxisymFluidPressurePointTable>().fluidPointIdColumn()); });
        registerColumn(
            catalogue, type, "pointId", [](const auto& model)
            { return exportColumn(model->template table<AxisymFluidPressurePointTable>().pointIdColumn()); });
        registerColumn(
            catalogue, type, "phiAngle", [](const auto& model)
            { return exportColumn(model->template table<AxisymFluidPressurePointTable>().phiAngleColumn()); });
    }
    {
        using model::AxisymConeRingTable;
        const std::uint16_t type = model::typeIndexOf<AxisymConeRingTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymConeRingTable>().idColumn()); });
        registerColumn(catalogue, type, "radius", [](const auto& model)
                       { return exportColumn(model->template table<AxisymConeRingTable>().radiusColumn()); });
        registerColumn(catalogue, type, "zCoord", [](const auto& model)
                       { return exportColumn(model->template table<AxisymConeRingTable>().zCoordColumn()); });
        registerColumn(
            catalogue, type, "permanentConstraints", [](const auto& model)
            { return exportColumn(model->template table<AxisymConeRingTable>().permanentConstraintsColumn()); });
    }
    {
        using model::AxisymFluidRingTable;
        const std::uint16_t type = model::typeIndexOf<AxisymFluidRingTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluidRingTable>().idColumn()); });
        registerColumn(catalogue, type, "position1", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluidRingTable>().position1Column()); });
        registerColumn(catalogue, type, "position2", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluidRingTable>().position2Column()); });
        registerColumn(catalogue, type, "position3", [](const auto& model)
                       { return exportColumn(model->template table<AxisymFluidRingTable>().position3Column()); });
    }
    {
        using model::NodeSequenceTable;
        const std::uint16_t type = model::typeIndexOf<NodeSequenceTable>();
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportVariableColumn(model->template table<NodeSequenceTable>().nodeColumn()); });
        registerColumn(catalogue, type, "sequence", [](const auto& model)
                       { return exportVariableColumn(model->template table<NodeSequenceTable>().sequenceColumn()); });
    }
    {
        using model::ShellNormalTable;
        const std::uint16_t type = model::typeIndexOf<ShellNormalTable>();
        registerColumn(catalogue, type, "nodeId", [](const auto& model)
                       { return exportColumn(model->template table<ShellNormalTable>().nodeIdColumn()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<ShellNormalTable>().frameIdColumn()); });
        registerColumn(catalogue, type, "vector", [](const auto& model)
                       { return exportColumn(model->template table<ShellNormalTable>().normalColumn()); });
    }
    {
        using model::ScalarPointTable;
        const std::uint16_t type = model::typeIndexOf<ScalarPointTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ScalarPointTable>().idColumn()); });
    }
}

} // namespace oor::query::column_export
