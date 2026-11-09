// The stored columns of the cards of the family "nonlinear_contact", for the query engine (ColumnExport.hpp).
// One block a card type: its type index, then a getter a column.

#include "oor/model/cards/nonlinear_contact/boltConnection/BoltConnectionTable.hpp"
#include "oor/model/cards/nonlinear_contact/boltPretension/BoltPretensionTable.hpp"
#include "oor/model/cards/nonlinear_contact/brakeMotion/BrakeMotionTable.hpp"
#include "oor/model/cards/nonlinear_contact/brakeProp/BrakePropTable.hpp"
#include "oor/model/cards/nonlinear_contact/brakeSqueal/BrakeSquealTable.hpp"
#include "oor/model/cards/nonlinear_contact/brakeSystem/BrakeSystemTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactAutoParams/ContactAutoParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactBeamCap/ContactBeamCapTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactBeamRadius/ContactBeamRadiusTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactBeamSectionFlags/ContactBeamSectionFlagsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactBezierSurface/ContactBezierSurfaceTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactBody/ContactBodyTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactBody1/ContactBody1Table.hpp"
#include "oor/model/cards/nonlinear_contact/contactBodyChange/ContactBodyChangeTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactBodyMove/ContactBodyMoveTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactBodyParams/ContactBodyParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactBoxRegion/ContactBoxRegionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactControlParams/ContactControlParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactElemSurface/ContactElemSurfaceTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactEllipsoidList/ContactEllipsoidListTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactFaceRegion/ContactFaceRegionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactMaterialRegion/ContactMaterialRegionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactNodeRegion/ContactNodeRegionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactNurbsCurve/ContactNurbsCurveTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactNurbsSurface/ContactNurbsSurfaceTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactPair/ContactPairTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactPairGeomParams/ContactPairGeomParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactPairPhysicalParams/ContactPairPhysicalParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactParams/ContactParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactPatchSurface/ContactPatchSurfaceTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactPropertyRegion/ContactPropertyRegionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactRigidMotion/ContactRigidMotionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactRigidSurfaceList/ContactRigidSurfaceListTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactSegment/ContactSegmentTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactSet/ContactSetTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactSetUnion/ContactSetUnionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactSlideline/ContactSlidelineTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactSurfaceParams/ContactSurfaceParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactTable/ContactTableTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactTable1/ContactTable1Table.hpp"
#include "oor/model/cards/nonlinear_contact/contactTrimCurve/ContactTrimCurveTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactUnglueNodes/ContactUnglueNodesTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactUserService/ContactUserServiceTable.hpp"
#include "oor/model/cards/nonlinear_contact/crackClosureCrack/CrackClosureCrackTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearAdaptiveStepping/NonlinearAdaptiveSteppingTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearBushForceTables/NonlinearBushForceTablesTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearBushLoad3D/NonlinearBushLoad3DTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearHarmonicFreqContinuation/NonlinearHarmonicFreqContinuationTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearHarmonicFreqList/NonlinearHarmonicFreqListTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearHarmonicFreqSteps/NonlinearHarmonicFreqStepsTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearHarmonicParams/NonlinearHarmonicParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearOutputItems/NonlinearOutputItemsTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearRadialGap/NonlinearRadialGapTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearSqueezeFilmDamper/NonlinearSqueezeFilmDamperTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearStepControl/NonlinearStepControlTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearUserOutput/NonlinearUserOutputTable.hpp"
#include "oor/model/cards/nonlinear_contact/slidelineFriction/SlidelineFrictionTable.hpp"
#include "oor/model/cards/nonlinear_contact/slidelineOutput/SlidelineOutputTable.hpp"
#include "oor/model/cards/nonlinear_contact/slidelineSegments/SlidelineSegmentsTable.hpp"
#include "oor/model/cards/nonlinear_contact/slidelineWidth/SlidelineWidthTable.hpp"
#include "oor/model/cards/nonlinear_contact/stepControlCriteria/StepControlCriteriaTable.hpp"
#include "oor/query/providers/ColumnExport.hpp"

namespace oor::query::column_export
{

void registerNonlinearContactColumns(StoredColumnCatalogue& catalogue)
{
    {
        using model::ContactAutoParamsTable;
        const std::uint16_t type = model::typeIndexOf<ContactAutoParamsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactAutoParamsTable>().idColumn()); });
    }
    {
        using model::ContactBodyParamsTable;
        const std::uint16_t type = model::typeIndexOf<ContactBodyParamsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactBodyParamsTable>().idColumn()); });
    }
    {
        using model::ContactBeamRadiusTable;
        const std::uint16_t type = model::typeIndexOf<ContactBeamRadiusTable>();
        registerColumn(catalogue, type, "radius", [](const auto& model)
                       { return exportColumn(model->template table<ContactBeamRadiusTable>().radiusColumn()); });
        registerColumn(catalogue, type, "idKind", [](const auto& model)
                       { return exportTextColumn(model->template table<ContactBeamRadiusTable>().idKindColumn()); });
        registerColumn(catalogue, type, "runs", [](const auto& model)
                       { return exportVariableColumn(model->template table<ContactBeamRadiusTable>().runsColumn()); });
    }
    {
        using model::ContactBodyTable;
        const std::uint16_t type = model::typeIndexOf<ContactBodyTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactBodyTable>().idColumn()); });
    }
    {
        using model::ContactBody1Table;
        const std::uint16_t type = model::typeIndexOf<ContactBody1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactBody1Table>().idColumn()); });
        registerColumn(catalogue, type, "bodyParamsId", [](const auto& model)
                       { return exportColumn(model->template table<ContactBody1Table>().bodyParamsIdColumn()); });
        registerColumn(catalogue, type, "dimension", [](const auto& model)
                       { return exportTextColumn(model->template table<ContactBody1Table>().dimensionColumn()); });
        registerColumn(catalogue, type, "behavior", [](const auto& model)
                       { return exportTextColumn(model->template table<ContactBody1Table>().behaviorColumn()); });
        registerColumn(catalogue, type, "surfaceId", [](const auto& model)
                       { return exportColumn(model->template table<ContactBody1Table>().surfaceIdColumn()); });
        registerColumn(catalogue, type, "rigidMotionId", [](const auto& model)
                       { return exportColumn(model->template table<ContactBody1Table>().rigidMotionIdColumn()); });
        registerColumn(
            catalogue, type, "forceReferencePoint", [](const auto& model)
            { return exportColumn(model->template table<ContactBody1Table>().forceReferencePointColumn()); });
    }
    {
        using model::ContactBoxRegionTable;
        const std::uint16_t type = model::typeIndexOf<ContactBoxRegionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactBoxRegionTable>().idColumn()); });
        registerColumn(catalogue, type, "inclusionRule", [](const auto& model)
                       { return exportColumn(model->template table<ContactBoxRegionTable>().inclusionRuleColumn()); });
        registerColumn(catalogue, type, "byCoordinates", [](const auto& model)
                       { return exportColumn(model->template table<ContactBoxRegionTable>().byCoordinatesColumn()); });
        registerColumn(catalogue, type, "cornerNodes", [](const auto& model)
                       { return exportColumn(model->template table<ContactBoxRegionTable>().cornerNodesColumn()); });
        registerColumn(
            catalogue, type, "cornerCoordinates", [](const auto& model)
            { return exportColumn(model->template table<ContactBoxRegionTable>().cornerCoordinatesColumn()); });
    }
    {
        using model::ContactBezierSurfaceTable;
        const std::uint16_t type = model::typeIndexOf<ContactBezierSurfaceTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactBezierSurfaceTable>().idColumn()); });
        registerColumn(
            catalogue, type, "pointCount1", [](const auto& model)
            { return exportColumn(model->template table<ContactBezierSurfaceTable>().pointCount1Column()); });
        registerColumn(
            catalogue, type, "pointCount2", [](const auto& model)
            { return exportColumn(model->template table<ContactBezierSurfaceTable>().pointCount2Column()); });
        registerColumn(
            catalogue, type, "subdivisions1", [](const auto& model)
            { return exportColumn(model->template table<ContactBezierSurfaceTable>().subdivisions1Column()); });
        registerColumn(
            catalogue, type, "subdivisions2", [](const auto& model)
            { return exportColumn(model->template table<ContactBezierSurfaceTable>().subdivisions2Column()); });
        registerColumn(
            catalogue, type, "nodes", [](const auto& model)
            { return exportVariableColumn(model->template table<ContactBezierSurfaceTable>().nodesColumn()); });
    }
    {
        using model::ContactEllipsoidListTable;
        const std::uint16_t type = model::typeIndexOf<ContactEllipsoidListTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactEllipsoidListTable>().idColumn()); });
    }
    {
        using model::ContactNodeRegionTable;
        const std::uint16_t type = model::typeIndexOf<ContactNodeRegionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactNodeRegionTable>().idColumn()); });
        registerColumn(catalogue, type, "bodyParamsId", [](const auto& model)
                       { return exportColumn(model->template table<ContactNodeRegionTable>().bodyParamsIdColumn()); });
        registerColumn(catalogue, type, "dimension", [](const auto& model)
                       { return exportTextColumn(model->template table<ContactNodeRegionTable>().dimensionColumn()); });
        registerColumn(catalogue, type, "sol700", [](const auto& model)
                       { return exportColumn(model->template table<ContactNodeRegionTable>().sol700Column()); });
        registerColumn(catalogue, type, "runs", [](const auto& model)
                       { return exportVariableColumn(model->template table<ContactNodeRegionTable>().runsColumn()); });
    }
    {
        using model::ContactBodyChangeTable;
        const std::uint16_t type = model::typeIndexOf<ContactBodyChangeTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactBodyChangeTable>().idColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportTextColumn(model->template table<ContactBodyChangeTable>().kindColumn()); });
        registerColumn(
            catalogue, type, "ranges", [](const auto& model)
            { return exportVariableColumn(model->template table<ContactBodyChangeTable>().rangesColumn()); });
    }
    {
        using model::ContactMaterialRegionTable;
        const std::uint16_t type = model::typeIndexOf<ContactMaterialRegionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactMaterialRegionTable>().idColumn()); });
        registerColumn(
            catalogue, type, "materials", [](const auto& model)
            { return exportVariableColumn(model->template table<ContactMaterialRegionTable>().materialsColumn()); });
    }
    {
        using model::ContactBodyMoveTable;
        const std::uint16_t type = model::typeIndexOf<ContactBodyMoveTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactBodyMoveTable>().idColumn()); });
        registerColumn(catalogue, type, "moveKind", [](const auto& model)
                       { return exportTextColumn(model->template table<ContactBodyMoveTable>().moveKindColumn()); });
        registerColumn(catalogue, type, "bodies", [](const auto& model)
                       { return exportVariableColumn(model->template table<ContactBodyMoveTable>().bodiesColumn()); });
    }
    {
        using model::ContactNurbsCurveTable;
        const std::uint16_t type = model::typeIndexOf<ContactNurbsCurveTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactNurbsCurveTable>().idColumn()); });
    }
    {
        using model::ContactNurbsSurfaceTable;
        const std::uint16_t type = model::typeIndexOf<ContactNurbsSurfaceTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactNurbsSurfaceTable>().idColumn()); });
    }
    {
        using model::ContactPairTable;
        const std::uint16_t type = model::typeIndexOf<ContactPairTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactPairTable>().idColumn()); });
        registerColumn(catalogue, type, "geomParamsId", [](const auto& model)
                       { return exportColumn(model->template table<ContactPairTable>().geomParamsIdColumn()); });
        registerColumn(catalogue, type, "physicalParamsId", [](const auto& model)
                       { return exportColumn(model->template table<ContactPairTable>().physicalParamsIdColumn()); });
        registerColumn(catalogue, type, "secondaryBodyId", [](const auto& model)
                       { return exportColumn(model->template table<ContactPairTable>().secondaryBodyIdColumn()); });
        registerColumn(catalogue, type, "primaryBodyId", [](const auto& model)
                       { return exportColumn(model->template table<ContactPairTable>().primaryBodyIdColumn()); });
        registerColumn(
            catalogue, type, "secondaryBodies", [](const auto& model)
            { return exportVariableColumn(model->template table<ContactPairTable>().secondaryBodiesColumn()); });
        registerColumn(
            catalogue, type, "primaryBodies", [](const auto& model)
            { return exportVariableColumn(model->template table<ContactPairTable>().primaryBodiesColumn()); });
    }
    {
        using model::ContactSlidelineTable;
        const std::uint16_t type = model::typeIndexOf<ContactSlidelineTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactSlidelineTable>().idColumn()); });
        registerColumn(
            catalogue, type, "secondaryLineId", [](const auto& model)
            { return exportColumn(model->template table<ContactSlidelineTable>().secondaryLineIdColumn()); });
        registerColumn(catalogue, type, "primaryLineId", [](const auto& model)
                       { return exportColumn(model->template table<ContactSlidelineTable>().primaryLineIdColumn()); });
        registerColumn(catalogue, type, "penaltyScale", [](const auto& model)
                       { return exportColumn(model->template table<ContactSlidelineTable>().penaltyScaleColumn()); });
        registerColumn(catalogue, type, "frictionId", [](const auto& model)
                       { return exportColumn(model->template table<ContactSlidelineTable>().frictionIdColumn()); });
        registerColumn(
            catalogue, type, "penetrationKind", [](const auto& model)
            { return exportColumn(model->template table<ContactSlidelineTable>().penetrationKindColumn()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<ContactSlidelineTable>().frameIdColumn()); });
    }
    {
        using model::ContactPairGeomParamsTable;
        const std::uint16_t type = model::typeIndexOf<ContactPairGeomParamsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactPairGeomParamsTable>().idColumn()); });
    }
    {
        using model::ContactPairPhysicalParamsTable;
        const std::uint16_t type = model::typeIndexOf<ContactPairPhysicalParamsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactPairPhysicalParamsTable>().idColumn()); });
    }
    {
        using model::ContactUserServiceTable;
        const std::uint16_t type = model::typeIndexOf<ContactUserServiceTable>();
        registerColumn(catalogue, type, "bodyId", [](const auto& model)
                       { return exportColumn(model->template table<ContactUserServiceTable>().bodyIdColumn()); });
    }
    {
        using model::ContactParamsTable;
        const std::uint16_t type = model::typeIndexOf<ContactParamsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactParamsTable>().idColumn()); });
    }
    {
        using model::ContactPatchSurfaceTable;
        const std::uint16_t type = model::typeIndexOf<ContactPatchSurfaceTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactPatchSurfaceTable>().idColumn()); });
        registerColumn(
            catalogue, type, "patches", [](const auto& model)
            { return exportVariableColumn(model->template table<ContactPatchSurfaceTable>().patchesColumn()); });
    }
    {
        using model::ContactBeamSectionFlagsTable;
        const std::uint16_t type = model::typeIndexOf<ContactBeamSectionFlagsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactBeamSectionFlagsTable>().idColumn()); });
        registerColumn(
            catalogue, type, "segments", [](const auto& model)
            { return exportVariableColumn(model->template table<ContactBeamSectionFlagsTable>().segmentsColumn()); });
    }
    {
        using model::ContactPropertyRegionTable;
        const std::uint16_t type = model::typeIndexOf<ContactPropertyRegionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactPropertyRegionTable>().idColumn()); });
        registerColumn(
            catalogue, type, "runs", [](const auto& model)
            { return exportVariableColumn(model->template table<ContactPropertyRegionTable>().runsColumn()); });
    }
    {
        using model::ContactRigidSurfaceListTable;
        const std::uint16_t type = model::typeIndexOf<ContactRigidSurfaceListTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactRigidSurfaceListTable>().idColumn()); });
        registerColumn(
            catalogue, type, "surfaces", [](const auto& model)
            { return exportVariableColumn(model->template table<ContactRigidSurfaceListTable>().surfacesColumn()); });
    }
    {
        using model::ContactRigidMotionTable;
        const std::uint16_t type = model::typeIndexOf<ContactRigidMotionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactRigidMotionTable>().idColumn()); });
    }
    {
        using model::ContactBeamCapTable;
        const std::uint16_t type = model::typeIndexOf<ContactBeamCapTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactBeamCapTable>().idColumn()); });
        registerColumn(catalogue, type, "capKind", [](const auto& model)
                       { return exportColumn(model->template table<ContactBeamCapTable>().capKindColumn()); });
        registerColumn(catalogue, type, "segmentCount", [](const auto& model)
                       { return exportColumn(model->template table<ContactBeamCapTable>().segmentCountColumn()); });
    }
    {
        using model::ContactSegmentTable;
        const std::uint16_t type = model::typeIndexOf<ContactSegmentTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactSegmentTable>().idColumn()); });
        registerColumn(catalogue, type, "surfaceId", [](const auto& model)
                       { return exportColumn(model->template table<ContactSegmentTable>().surfaceIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<ContactSegmentTable>().nodesColumn()); });
    }
    {
        using model::ContactFaceRegionTable;
        const std::uint16_t type = model::typeIndexOf<ContactFaceRegionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactFaceRegionTable>().idColumn()); });
    }
    {
        using model::ContactTable1Table;
        const std::uint16_t type = model::typeIndexOf<ContactTable1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactTable1Table>().idColumn()); });
        registerColumn(catalogue, type, "runs", [](const auto& model)
                       { return exportVariableColumn(model->template table<ContactTable1Table>().runsColumn()); });
    }
    {
        using model::ContactTableTable;
        const std::uint16_t type = model::typeIndexOf<ContactTableTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactTableTable>().idColumn()); });
    }
    {
        using model::ContactSetUnionTable;
        const std::uint16_t type = model::typeIndexOf<ContactSetUnionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactSetUnionTable>().idColumn()); });
        registerColumn(catalogue, type, "sets", [](const auto& model)
                       { return exportVariableColumn(model->template table<ContactSetUnionTable>().setsColumn()); });
    }
    {
        using model::ContactSurfaceParamsTable;
        const std::uint16_t type = model::typeIndexOf<ContactSurfaceParamsTable>();
        registerColumn(
            catalogue, type, "contactSetId", [](const auto& model)
            { return exportColumn(model->template table<ContactSurfaceParamsTable>().contactSetIdColumn()); });
    }
    {
        using model::ContactControlParamsTable;
        const std::uint16_t type = model::typeIndexOf<ContactControlParamsTable>();
        registerColumn(
            catalogue, type, "contactSetId", [](const auto& model)
            { return exportColumn(model->template table<ContactControlParamsTable>().contactSetIdColumn()); });
    }
    {
        using model::ContactTrimCurveTable;
        const std::uint16_t type = model::typeIndexOf<ContactTrimCurveTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactTrimCurveTable>().idColumn()); });
    }
    {
        using model::ContactSetTable;
        const std::uint16_t type = model::typeIndexOf<ContactSetTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactSetTable>().idColumn()); });
    }
    {
        using model::SlidelineFrictionTable;
        const std::uint16_t type = model::typeIndexOf<SlidelineFrictionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SlidelineFrictionTable>().idColumn()); });
        registerColumn(
            catalogue, type, "stickStiffness", [](const auto& model)
            { return exportColumn(model->template table<SlidelineFrictionTable>().stickStiffnessColumn()); });
        registerColumn(
            catalogue, type, "staticFriction", [](const auto& model)
            { return exportColumn(model->template table<SlidelineFrictionTable>().staticFrictionColumn()); });
    }
    {
        using model::SlidelineSegmentsTable;
        const std::uint16_t type = model::typeIndexOf<SlidelineSegmentsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SlidelineSegmentsTable>().idColumn()); });
        registerColumn(catalogue, type, "runs", [](const auto& model)
                       { return exportVariableColumn(model->template table<SlidelineSegmentsTable>().runsColumn()); });
    }
    {
        using model::BoltConnectionTable;
        const std::uint16_t type = model::typeIndexOf<BoltConnectionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BoltConnectionTable>().idColumn()); });
        registerColumn(catalogue, type, "controlNode", [](const auto& model)
                       { return exportColumn(model->template table<BoltConnectionTable>().controlNodeColumn()); });
        registerColumn(catalogue, type, "topNodes", [](const auto& model)
                       { return exportVariableColumn(model->template table<BoltConnectionTable>().topNodesColumn()); });
        registerColumn(
            catalogue, type, "bottomNodes", [](const auto& model)
            { return exportVariableColumn(model->template table<BoltConnectionTable>().bottomNodesColumn()); });
    }
    {
        using model::BoltPretensionTable;
        const std::uint16_t type = model::typeIndexOf<BoltPretensionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BoltPretensionTable>().idColumn()); });
    }
    {
        using model::SlidelineOutputTable;
        const std::uint16_t type = model::typeIndexOf<SlidelineOutputTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SlidelineOutputTable>().idColumn()); });
        registerColumn(catalogue, type, "all", [](const auto& model)
                       { return exportColumn(model->template table<SlidelineOutputTable>().allColumn()); });
        registerColumn(catalogue, type, "runs", [](const auto& model)
                       { return exportVariableColumn(model->template table<SlidelineOutputTable>().runsColumn()); });
    }
    {
        using model::BrakePropTable;
        const std::uint16_t type = model::typeIndexOf<BrakePropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BrakePropTable>().idColumn()); });
    }
    {
        using model::BrakeSystemTable;
        const std::uint16_t type = model::typeIndexOf<BrakeSystemTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BrakeSystemTable>().idColumn()); });
    }
    {
        using model::BrakeSquealTable;
        const std::uint16_t type = model::typeIndexOf<BrakeSquealTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BrakeSquealTable>().idColumn()); });
    }
    {
        using model::ContactElemSurfaceTable;
        const std::uint16_t type = model::typeIndexOf<ContactElemSurfaceTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactElemSurfaceTable>().idColumn()); });
        registerColumn(catalogue, type, "runs", [](const auto& model)
                       { return exportVariableColumn(model->template table<ContactElemSurfaceTable>().runsColumn()); });
    }
    {
        using model::SlidelineWidthTable;
        const std::uint16_t type = model::typeIndexOf<SlidelineWidthTable>();
        registerColumn(catalogue, type, "segmentsId", [](const auto& model)
                       { return exportColumn(model->template table<SlidelineWidthTable>().segmentsIdColumn()); });
        registerColumn(catalogue, type, "widths", [](const auto& model)
                       { return exportVariableColumn(model->template table<SlidelineWidthTable>().widthsColumn()); });
    }
    {
        using model::NonlinearBushForceTablesTable;
        const std::uint16_t type = model::typeIndexOf<NonlinearBushForceTablesTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearBushForceTablesTable>().idColumn()); });
    }
    {
        using model::BrakeMotionTable;
        const std::uint16_t type = model::typeIndexOf<BrakeMotionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BrakeMotionTable>().idColumn()); });
    }
    {
        using model::NonlinearAdaptiveSteppingTable;
        const std::uint16_t type = model::typeIndexOf<NonlinearAdaptiveSteppingTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearAdaptiveSteppingTable>().idColumn()); });
    }
    {
        using model::NonlinearBushLoad3DTable;
        const std::uint16_t type = model::typeIndexOf<NonlinearBushLoad3DTable>();
        registerColumn(catalogue, type, "setId", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearBushLoad3DTable>().setIdColumn()); });
        registerColumn(catalogue, type, "nodeA", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearBushLoad3DTable>().nodeAColumn()); });
        registerColumn(catalogue, type, "nodeB", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearBushLoad3DTable>().nodeBColumn()); });
        registerColumn(catalogue, type, "loadId1", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearBushLoad3DTable>().loadId1Column()); });
        registerColumn(catalogue, type, "loadId2", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearBushLoad3DTable>().loadId2Column()); });
    }
    {
        using model::NonlinearHarmonicFreqListTable;
        const std::uint16_t type = model::typeIndexOf<NonlinearHarmonicFreqListTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearHarmonicFreqListTable>().idColumn()); });
        registerColumn(catalogue, type, "frequencies",
                       [](const auto& model)
                       {
                           return exportVariableColumn(
                               model->template table<NonlinearHarmonicFreqListTable>().frequenciesColumn());
                       });
    }
    {
        using model::NonlinearHarmonicFreqStepsTable;
        const std::uint16_t type = model::typeIndexOf<NonlinearHarmonicFreqStepsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearHarmonicFreqStepsTable>().idColumn()); });
        registerColumn(
            catalogue, type, "startFreq", [](const auto& model)
            { return exportColumn(model->template table<NonlinearHarmonicFreqStepsTable>().startFreqColumn()); });
        registerColumn(
            catalogue, type, "freqStep", [](const auto& model)
            { return exportColumn(model->template table<NonlinearHarmonicFreqStepsTable>().freqStepColumn()); });
        registerColumn(
            catalogue, type, "stepCount", [](const auto& model)
            { return exportColumn(model->template table<NonlinearHarmonicFreqStepsTable>().stepCountColumn()); });
    }
    {
        using model::NonlinearHarmonicFreqContinuationTable;
        const std::uint16_t type = model::typeIndexOf<NonlinearHarmonicFreqContinuationTable>();
        registerColumn(
            catalogue, type, "id", [](const auto& model)
            { return exportColumn(model->template table<NonlinearHarmonicFreqContinuationTable>().idColumn()); });
    }
    {
        using model::NonlinearHarmonicParamsTable;
        const std::uint16_t type = model::typeIndexOf<NonlinearHarmonicParamsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearHarmonicParamsTable>().idColumn()); });
        registerColumn(
            catalogue, type, "subharmonicFactor", [](const auto& model)
            { return exportColumn(model->template table<NonlinearHarmonicParamsTable>().subharmonicFactorColumn()); });
        registerColumn(
            catalogue, type, "harmonicCount", [](const auto& model)
            { return exportColumn(model->template table<NonlinearHarmonicParamsTable>().harmonicCountColumn()); });
        registerColumn(
            catalogue, type, "freqListId", [](const auto& model)
            { return exportColumn(model->template table<NonlinearHarmonicParamsTable>().freqListIdColumn()); });
    }
    {
        using model::NonlinearOutputItemsTable;
        const std::uint16_t type = model::typeIndexOf<NonlinearOutputItemsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearOutputItemsTable>().idColumn()); });
    }
    {
        using model::NonlinearUserOutputTable;
        const std::uint16_t type = model::typeIndexOf<NonlinearUserOutputTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearUserOutputTable>().idColumn()); });
    }
    {
        using model::NonlinearRadialGapTable;
        const std::uint16_t type = model::typeIndexOf<NonlinearRadialGapTable>();
        registerColumn(catalogue, type, "setId", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearRadialGapTable>().setIdColumn()); });
        registerColumn(catalogue, type, "nodeA", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearRadialGapTable>().nodeAColumn()); });
        registerColumn(catalogue, type, "nodeB", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearRadialGapTable>().nodeBColumn()); });
        registerColumn(catalogue, type, "plane", [](const auto& model)
                       { return exportTextColumn(model->template table<NonlinearRadialGapTable>().planeColumn()); });
        registerColumn(
            catalogue, type, "stiffnessTableId", [](const auto& model)
            { return exportColumn(model->template table<NonlinearRadialGapTable>().stiffnessTableIdColumn()); });
        registerColumn(
            catalogue, type, "clearanceTableId", [](const auto& model)
            { return exportColumn(model->template table<NonlinearRadialGapTable>().clearanceTableIdColumn()); });
        registerColumn(
            catalogue, type, "frictionTableId", [](const auto& model)
            { return exportColumn(model->template table<NonlinearRadialGapTable>().frictionTableIdColumn()); });
        registerColumn(catalogue, type, "radius", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearRadialGapTable>().radiusColumn()); });
        registerColumn(catalogue, type, "smoothing", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearRadialGapTable>().smoothingColumn()); });
        registerColumn(
            catalogue, type, "smoothingValue", [](const auto& model)
            { return exportColumn(model->template table<NonlinearRadialGapTable>().smoothingValueColumn()); });
    }
    {
        using model::NonlinearSqueezeFilmDamperTable;
        const std::uint16_t type = model::typeIndexOf<NonlinearSqueezeFilmDamperTable>();
        registerColumn(
            catalogue, type, "setId", [](const auto& model)
            { return exportColumn(model->template table<NonlinearSqueezeFilmDamperTable>().setIdColumn()); });
    }
    {
        using model::NonlinearStepControlTable;
        const std::uint16_t type = model::typeIndexOf<NonlinearStepControlTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearStepControlTable>().idColumn()); });
        registerColumn(catalogue, type, "totalTime", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearStepControlTable>().totalTimeColumn()); });
    }
    {
        using model::StepControlCriteriaTable;
        const std::uint16_t type = model::typeIndexOf<StepControlCriteriaTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<StepControlCriteriaTable>().idColumn()); });
    }
    {
        using model::ContactUnglueNodesTable;
        const std::uint16_t type = model::typeIndexOf<ContactUnglueNodesTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ContactUnglueNodesTable>().idColumn()); });
    }
    {
        using model::CrackClosureCrackTable;
        const std::uint16_t type = model::typeIndexOf<CrackClosureCrackTable>();
        registerColumn(catalogue, type, "setId", [](const auto& model)
                       { return exportColumn(model->template table<CrackClosureCrackTable>().setIdColumn()); });
    }
}

} // namespace oor::query::column_export
