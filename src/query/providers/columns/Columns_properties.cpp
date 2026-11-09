// The stored columns of the cards of the family "properties", for the query engine (ColumnExport.hpp).
// One block a card type: its type index, then a getter a column.

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/properties/axisymHarmonicProp/AxisymHarmonicPropTable.hpp"
#include "oor/model/cards/properties/axisymShellProp/AxisymShellPropTable.hpp"
#include "oor/model/cards/properties/barProp/BarPropTable.hpp"
#include "oor/model/cards/properties/barPropNonlinear/BarPropNonlinearTable.hpp"
#include "oor/model/cards/properties/barShapeProp/BarShapePropTable.hpp"
#include "oor/model/cards/properties/beamLumpedAreaProp/BeamLumpedAreaPropTable.hpp"
#include "oor/model/cards/properties/beamProp/BeamPropTable.hpp"
#include "oor/model/cards/properties/beamProp3/BeamProp3Table.hpp"
#include "oor/model/cards/properties/beamShapeProp/BeamShapePropTable.hpp"
#include "oor/model/cards/properties/bushProp/BushPropTable.hpp"
#include "oor/model/cards/properties/bushProp1D/BushProp1DTable.hpp"
#include "oor/model/cards/properties/bushProp2D/BushProp2DTable.hpp"
#include "oor/model/cards/properties/bushProp2DFreq/BushProp2DFreqTable.hpp"
#include "oor/model/cards/properties/bushPropFreq/BushPropFreqTable.hpp"
#include "oor/model/cards/properties/compProp/CompPropTable.hpp"
#include "oor/model/cards/properties/compPropExplicit/CompPropExplicitTable.hpp"
#include "oor/model/cards/properties/compPropFreqDamping/CompPropFreqDampingTable.hpp"
#include "oor/model/cards/properties/compPropGlobal/CompPropGlobalTable.hpp"
#include "oor/model/cards/properties/compPropIntegration/CompPropIntegrationTable.hpp"
#include "oor/model/cards/properties/compSolidProp/CompSolidPropTable.hpp"
#include "oor/model/cards/properties/compSolidPropNonlinear/CompSolidPropNonlinearTable.hpp"
#include "oor/model/cards/properties/curvedBeamProp/CurvedBeamPropTable.hpp"
#include "oor/model/cards/properties/damperProp/DamperPropTable.hpp"
#include "oor/model/cards/properties/damperPropFreq/DamperPropFreqTable.hpp"
#include "oor/model/cards/properties/fastenerProp/FastenerPropTable.hpp"
#include "oor/model/cards/properties/fastenerPropFreq/FastenerPropFreqTable.hpp"
#include "oor/model/cards/properties/gapProp/GapPropTable.hpp"
#include "oor/model/cards/properties/heatCapacityProp5/HeatCapacityProp5Table.hpp"
#include "oor/model/cards/properties/massProp/MassPropTable.hpp"
#include "oor/model/cards/properties/nonStructMass/NonStructMassTable.hpp"
#include "oor/model/cards/properties/nonStructMass1/NonStructMass1Table.hpp"
#include "oor/model/cards/properties/nonStructMassLumped/NonStructMassLumpedTable.hpp"
#include "oor/model/cards/properties/nonStructMassLumped1/NonStructMassLumped1Table.hpp"
#include "oor/model/cards/properties/nonStructMassUnion/NonStructMassUnionTable.hpp"
#include "oor/model/cards/properties/perforatedShellProp/PerforatedShellPropTable.hpp"
#include "oor/model/cards/properties/planeCompProp/PlaneCompPropTable.hpp"
#include "oor/model/cards/properties/planeProp/PlanePropTable.hpp"
#include "oor/model/cards/properties/planePropNonlinear/PlanePropNonlinearTable.hpp"
#include "oor/model/cards/properties/planePropNonlinearExt/PlanePropNonlinearExtTable.hpp"
#include "oor/model/cards/properties/propDistribution/PropDistributionTable.hpp"
#include "oor/model/cards/properties/propMatDistribution/PropMatDistributionTable.hpp"
#include "oor/model/cards/properties/rodProp/RodPropTable.hpp"
#include "oor/model/cards/properties/rodPropNonlinear/RodPropNonlinearTable.hpp"
#include "oor/model/cards/properties/shearPanelProp/ShearPanelPropTable.hpp"
#include "oor/model/cards/properties/shearPanelPropNonlinear/ShearPanelPropNonlinearTable.hpp"
#include "oor/model/cards/properties/shellProp/ShellPropTable.hpp"
#include "oor/model/cards/properties/shellPropNodal/ShellPropNodalTable.hpp"
#include "oor/model/cards/properties/shellPropNonlinear/ShellPropNonlinearTable.hpp"
#include "oor/model/cards/properties/solidProp/SolidPropTable.hpp"
#include "oor/model/cards/properties/solidPropHyperelastic/SolidPropHyperelasticTable.hpp"
#include "oor/model/cards/properties/springOffsetProp/SpringOffsetPropTable.hpp"
#include "oor/model/cards/properties/springProp/SpringPropTable.hpp"
#include "oor/model/cards/properties/springPropFreq/SpringPropFreqTable.hpp"
#include "oor/model/cards/properties/springPropTable/SpringPropTableTable.hpp"
#include "oor/model/cards/properties/tiltingPadBearingProp/TiltingPadBearingPropTable.hpp"
#include "oor/model/cards/properties/tubeProp/TubePropTable.hpp"
#include "oor/model/cards/properties/userElemService/UserElemServiceTable.hpp"
#include "oor/model/cards/properties/viscousDamperProp/ViscousDamperPropTable.hpp"
#include "oor/model/cards/properties/viscousDamperPropNonlinear/ViscousDamperPropNonlinearTable.hpp"
#include "oor/model/cards/properties/weldProp/WeldPropTable.hpp"
#include "oor/query/providers/ColumnExport.hpp"

namespace oor::query::column_export
{

namespace
{

// The property or element type a non-structural mass card names, as its neutral card name.
template <typename TableT, typename ModelPointer>
QueryColumn nonStructMassTypeColumn(const ModelPointer& model)
{
    return exportCardValues(model->template table<TableT>(),
                            [](const typename TableT::CardType& card) { return card.type; });
}

} // namespace

void registerPropertiesColumns(StoredColumnCatalogue& catalogue)
{
    {
        using model::UserElemServiceTable;
        const std::uint16_t type = model::typeIndexOf<UserElemServiceTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserElemServiceTable>().idColumn()); });
    }
    {
        using model::NonStructMassTable;
        const std::uint16_t type = model::typeIndexOf<NonStructMassTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NonStructMassTable>().idColumn()); });
        registerColumn(catalogue, type, "type",
                       [](const auto& model) { return nonStructMassTypeColumn<NonStructMassTable>(model); });
        registerColumn(catalogue, type, "targetIds", [](const auto& model)
                       { return exportVariableColumn(model->template table<NonStructMassTable>().targetIdsColumn()); });
        registerColumn(
            catalogue, type, "targetValues", [](const auto& model)
            { return exportVariableColumn(model->template table<NonStructMassTable>().targetValuesColumn()); });
    }
    {
        using model::NonStructMass1Table;
        const std::uint16_t type = model::typeIndexOf<NonStructMass1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NonStructMass1Table>().idColumn()); });
        registerColumn(catalogue, type, "type",
                       [](const auto& model) { return nonStructMassTypeColumn<NonStructMass1Table>(model); });
        registerColumn(catalogue, type, "nonStructMass", [](const auto& model)
                       { return exportColumn(model->template table<NonStructMass1Table>().nonStructMassColumn()); });
        registerColumn(catalogue, type, "all", [](const auto& model)
                       { return exportColumn(model->template table<NonStructMass1Table>().allColumn()); });
        registerColumn(catalogue, type, "runs", [](const auto& model)
                       { return exportVariableColumn(model->template table<NonStructMass1Table>().runsColumn()); });
    }
    {
        using model::NonStructMassUnionTable;
        const std::uint16_t type = model::typeIndexOf<NonStructMassUnionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NonStructMassUnionTable>().idColumn()); });
        registerColumn(catalogue, type, "sets", [](const auto& model)
                       { return exportVariableColumn(model->template table<NonStructMassUnionTable>().setColumn()); });
    }
    {
        using model::NonStructMassLumpedTable;
        const std::uint16_t type = model::typeIndexOf<NonStructMassLumpedTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NonStructMassLumpedTable>().idColumn()); });
        registerColumn(catalogue, type, "type",
                       [](const auto& model) { return nonStructMassTypeColumn<NonStructMassLumpedTable>(model); });
        registerColumn(
            catalogue, type, "targetIds", [](const auto& model)
            { return exportVariableColumn(model->template table<NonStructMassLumpedTable>().targetIdsColumn()); });
        registerColumn(
            catalogue, type, "targetValues", [](const auto& model)
            { return exportVariableColumn(model->template table<NonStructMassLumpedTable>().targetValuesColumn()); });
    }
    {
        using model::NonStructMassLumped1Table;
        const std::uint16_t type = model::typeIndexOf<NonStructMassLumped1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NonStructMassLumped1Table>().idColumn()); });
        registerColumn(catalogue, type, "type",
                       [](const auto& model) { return nonStructMassTypeColumn<NonStructMassLumped1Table>(model); });
        registerColumn(
            catalogue, type, "nonStructMass", [](const auto& model)
            { return exportColumn(model->template table<NonStructMassLumped1Table>().nonStructMassColumn()); });
        registerColumn(catalogue, type, "all", [](const auto& model)
                       { return exportColumn(model->template table<NonStructMassLumped1Table>().allColumn()); });
        registerColumn(
            catalogue, type, "runs", [](const auto& model)
            { return exportVariableColumn(model->template table<NonStructMassLumped1Table>().runsColumn()); });
    }
    {
        using model::AxisymShellPropTable;
        const std::uint16_t type = model::typeIndexOf<AxisymShellPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymShellPropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<AxisymShellPropTable>().matIdColumn()); });
        registerColumn(catalogue, type, "thickness1", [](const auto& model)
                       { return exportColumn(model->template table<AxisymShellPropTable>().thickness1Column()); });
        registerColumn(catalogue, type, "thickness2", [](const auto& model)
                       { return exportColumn(model->template table<AxisymShellPropTable>().thickness2Column()); });
        registerColumn(catalogue, type, "analysisType", [](const auto& model)
                       { return exportColumn(model->template table<AxisymShellPropTable>().analysisTypeColumn()); });
    }
    {
        using model::AxisymHarmonicPropTable;
        const std::uint16_t type = model::typeIndexOf<AxisymHarmonicPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymHarmonicPropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<AxisymHarmonicPropTable>().matIdColumn()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<AxisymHarmonicPropTable>().frameIdColumn()); });
        registerColumn(
            catalogue, type, "harmonicIndex", [](const auto& model)
            { return exportColumn(model->template table<AxisymHarmonicPropTable>().harmonicIndexColumn()); });
        registerColumn(catalogue, type, "integration", [](const auto& model)
                       { return exportColumn(model->template table<AxisymHarmonicPropTable>().integrationColumn()); });
    }
    {
        using model::BarPropTable;
        const std::uint16_t type = model::typeIndexOf<BarPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BarPropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<BarPropTable>().matIdColumn()); });
        registerColumn(catalogue, type, "area", [](const auto& model)
                       { return exportColumn(model->template table<BarPropTable>().areaColumn()); });
        registerColumn(catalogue, type, "bendingInertia1", [](const auto& model)
                       { return exportColumn(model->template table<BarPropTable>().bendingInertia1Column()); });
        registerColumn(catalogue, type, "bendingInertia2", [](const auto& model)
                       { return exportColumn(model->template table<BarPropTable>().bendingInertia2Column()); });
        registerColumn(catalogue, type, "torsionConstant", [](const auto& model)
                       { return exportColumn(model->template table<BarPropTable>().torsionConstantColumn()); });
        registerColumn(catalogue, type, "nonStructMass", [](const auto& model)
                       { return exportColumn(model->template table<BarPropTable>().nonStructMassColumn()); });
        registerColumn(catalogue, type, "stressPointC", [](const auto& model)
                       { return exportColumn(model->template table<BarPropTable>().stressPointCColumn()); });
        registerColumn(catalogue, type, "stressPointD", [](const auto& model)
                       { return exportColumn(model->template table<BarPropTable>().stressPointDColumn()); });
        registerColumn(catalogue, type, "stressPointE", [](const auto& model)
                       { return exportColumn(model->template table<BarPropTable>().stressPointEColumn()); });
        registerColumn(catalogue, type, "stressPointF", [](const auto& model)
                       { return exportColumn(model->template table<BarPropTable>().stressPointFColumn()); });
        registerColumn(catalogue, type, "shearFactor", [](const auto& model)
                       { return exportColumn(model->template table<BarPropTable>().shearFactorColumn()); });
        registerColumn(catalogue, type, "productInertia12", [](const auto& model)
                       { return exportColumn(model->template table<BarPropTable>().productInertia12Column()); });
    }
    {
        using model::BarShapePropTable;
        const std::uint16_t type = model::typeIndexOf<BarShapePropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BarShapePropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<BarShapePropTable>().matIdColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportTextColumn(model->template table<BarShapePropTable>().kindColumn()); });
        registerColumn(catalogue, type, "dims", [](const auto& model)
                       { return exportVariableColumn(model->template table<BarShapePropTable>().dimsColumn()); });
        registerColumn(catalogue, type, "nonStructMass", [](const auto& model)
                       { return exportColumn(model->template table<BarShapePropTable>().nonStructMassColumn()); });
    }
    {
        using model::BarPropNonlinearTable;
        const std::uint16_t type = model::typeIndexOf<BarPropNonlinearTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BarPropNonlinearTable>().idColumn()); });
    }
    {
        using model::BeamLumpedAreaPropTable;
        const std::uint16_t type = model::typeIndexOf<BeamLumpedAreaPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BeamLumpedAreaPropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<BeamLumpedAreaPropTable>().matIdColumn()); });
        registerColumn(catalogue, type, "section", [](const auto& model)
                       { return exportColumn(model->template table<BeamLumpedAreaPropTable>().sectionColumn()); });
        registerColumn(catalogue, type, "shearFactor", [](const auto& model)
                       { return exportColumn(model->template table<BeamLumpedAreaPropTable>().shearFactorColumn()); });
        registerColumn(catalogue, type, "nsmCenter", [](const auto& model)
                       { return exportColumn(model->template table<BeamLumpedAreaPropTable>().nsmCenterColumn()); });
        registerColumn(catalogue, type, "neutralAxis", [](const auto& model)
                       { return exportColumn(model->template table<BeamLumpedAreaPropTable>().neutralAxisColumn()); });
        registerColumn(
            catalogue, type, "symmetricOption", [](const auto& model)
            { return exportColumn(model->template table<BeamLumpedAreaPropTable>().symmetricOptionColumn()); });
        registerColumn(
            catalogue, type, "areaY", [](const auto& model)
            { return exportVariableColumn(model->template table<BeamLumpedAreaPropTable>().areaYColumn()); });
        registerColumn(
            catalogue, type, "areaZ", [](const auto& model)
            { return exportVariableColumn(model->template table<BeamLumpedAreaPropTable>().areaZColumn()); });
        registerColumn(
            catalogue, type, "areaFraction", [](const auto& model)
            { return exportVariableColumn(model->template table<BeamLumpedAreaPropTable>().areaFractionColumn()); });
        registerColumn(
            catalogue, type, "areaMatId", [](const auto& model)
            { return exportVariableColumn(model->template table<BeamLumpedAreaPropTable>().areaMatIdColumn()); });
    }
    {
        using model::BeamPropTable;
        const std::uint16_t type = model::typeIndexOf<BeamPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BeamPropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<BeamPropTable>().matIdColumn()); });
        registerColumn(catalogue, type, "stationOutput", [](const auto& model)
                       { return exportVariableColumn(model->template table<BeamPropTable>().stationOutputColumn()); });
        registerColumn(catalogue, type, "stationRatio", [](const auto& model)
                       { return exportVariableColumn(model->template table<BeamPropTable>().stationRatioColumn()); });
        registerColumn(catalogue, type, "stationSection", [](const auto& model)
                       { return exportVariableColumn(model->template table<BeamPropTable>().stationSectionColumn()); });
        registerColumn(catalogue, type, "stationStress", [](const auto& model)
                       { return exportVariableColumn(model->template table<BeamPropTable>().stationStressColumn()); });
        registerColumn(catalogue, type, "shearFactor", [](const auto& model)
                       { return exportColumn(model->template table<BeamPropTable>().shearFactorColumn()); });
        registerColumn(catalogue, type, "shearRelief", [](const auto& model)
                       { return exportColumn(model->template table<BeamPropTable>().shearReliefColumn()); });
        registerColumn(catalogue, type, "nsmInertia", [](const auto& model)
                       { return exportColumn(model->template table<BeamPropTable>().nsmInertiaColumn()); });
        registerColumn(catalogue, type, "warpingCoefficient", [](const auto& model)
                       { return exportColumn(model->template table<BeamPropTable>().warpingCoefficientColumn()); });
        registerColumn(catalogue, type, "nsmCenter", [](const auto& model)
                       { return exportColumn(model->template table<BeamPropTable>().nsmCenterColumn()); });
        registerColumn(catalogue, type, "neutralAxis", [](const auto& model)
                       { return exportColumn(model->template table<BeamPropTable>().neutralAxisColumn()); });
    }
    {
        using model::BeamProp3Table;
        const std::uint16_t type = model::typeIndexOf<BeamProp3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BeamProp3Table>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<BeamProp3Table>().matIdColumn()); });
        registerColumn(catalogue, type, "section", [](const auto& model)
                       { return exportColumn(model->template table<BeamProp3Table>().sectionColumn()); });
        registerColumn(catalogue, type, "stressPoints", [](const auto& model)
                       { return exportColumn(model->template table<BeamProp3Table>().stressPointsColumn()); });
        registerColumn(catalogue, type, "stressOutput", [](const auto& model)
                       { return exportColumn(model->template table<BeamProp3Table>().stressOutputColumn()); });
        registerColumn(catalogue, type, "shearFactor", [](const auto& model)
                       { return exportColumn(model->template table<BeamProp3Table>().shearFactorColumn()); });
        registerColumn(catalogue, type, "neutralAxis", [](const auto& model)
                       { return exportColumn(model->template table<BeamProp3Table>().neutralAxisColumn()); });
        registerColumn(catalogue, type, "nsmCenter", [](const auto& model)
                       { return exportColumn(model->template table<BeamProp3Table>().nsmCenterColumn()); });
        registerColumn(catalogue, type, "nsmInertia", [](const auto& model)
                       { return exportColumn(model->template table<BeamProp3Table>().nsmInertiaColumn()); });
        registerColumn(catalogue, type, "warpingCoefficient", [](const auto& model)
                       { return exportColumn(model->template table<BeamProp3Table>().warpingCoefficientColumn()); });
        registerColumn(catalogue, type, "gaussStress", [](const auto& model)
                       { return exportColumn(model->template table<BeamProp3Table>().gaussStressColumn()); });
        registerColumn(catalogue, type, "warpingGiven", [](const auto& model)
                       { return exportColumn(model->template table<BeamProp3Table>().warpingGivenColumn()); });
        registerColumn(catalogue, type, "warping", [](const auto& model)
                       { return exportColumn(model->template table<BeamProp3Table>().warpingColumn()); });
    }
    {
        using model::BeamShapePropTable;
        const std::uint16_t type = model::typeIndexOf<BeamShapePropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BeamShapePropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<BeamShapePropTable>().matIdColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportTextColumn(model->template table<BeamShapePropTable>().kindColumn()); });
        registerColumn(
            catalogue, type, "stationRatio", [](const auto& model)
            { return exportVariableColumn(model->template table<BeamShapePropTable>().stationRatioColumn()); });
        registerColumn(
            catalogue, type, "stationOutput", [](const auto& model)
            { return exportVariableColumn(model->template table<BeamShapePropTable>().stationOutputColumn()); });
        registerColumn(
            catalogue, type, "stationNonStructMass", [](const auto& model)
            { return exportVariableColumn(model->template table<BeamShapePropTable>().stationNonStructMassColumn()); });
        registerColumn(
            catalogue, type, "stationDims", [](const auto& model)
            { return exportVariableColumn(model->template table<BeamShapePropTable>().stationDimsColumn()); });
    }
    {
        using model::CurvedBeamPropTable;
        const std::uint16_t type = model::typeIndexOf<CurvedBeamPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CurvedBeamPropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<CurvedBeamPropTable>().matIdColumn()); });
        registerColumn(catalogue, type, "form", [](const auto& model)
                       { return exportColumn(model->template table<CurvedBeamPropTable>().formColumn()); });
        registerColumn(catalogue, type, "section", [](const auto& model)
                       { return exportColumn(model->template table<CurvedBeamPropTable>().sectionColumn()); });
        registerColumn(catalogue, type, "stressPoints", [](const auto& model)
                       { return exportColumn(model->template table<CurvedBeamPropTable>().stressPointsColumn()); });
        registerColumn(catalogue, type, "shearFactor", [](const auto& model)
                       { return exportColumn(model->template table<CurvedBeamPropTable>().shearFactorColumn()); });
        registerColumn(
            catalogue, type, "neutralAxisOffset", [](const auto& model)
            { return exportColumn(model->template table<CurvedBeamPropTable>().neutralAxisOffsetColumn()); });
        registerColumn(
            catalogue, type, "flexibilityOption", [](const auto& model)
            { return exportColumn(model->template table<CurvedBeamPropTable>().flexibilityOptionColumn()); });
        registerColumn(catalogue, type, "meanRadius", [](const auto& model)
                       { return exportColumn(model->template table<CurvedBeamPropTable>().meanRadiusColumn()); });
        registerColumn(catalogue, type, "thickness", [](const auto& model)
                       { return exportColumn(model->template table<CurvedBeamPropTable>().thicknessColumn()); });
        registerColumn(catalogue, type, "internalPressure", [](const auto& model)
                       { return exportColumn(model->template table<CurvedBeamPropTable>().internalPressureColumn()); });
        registerColumn(catalogue, type, "bendRadius", [](const auto& model)
                       { return exportColumn(model->template table<CurvedBeamPropTable>().bendRadiusColumn()); });
        registerColumn(catalogue, type, "arcAngle", [](const auto& model)
                       { return exportColumn(model->template table<CurvedBeamPropTable>().arcAngleColumn()); });
        registerColumn(catalogue, type, "nonStructMass", [](const auto& model)
                       { return exportColumn(model->template table<CurvedBeamPropTable>().nonStructMassColumn()); });
        registerColumn(
            catalogue, type, "centroidRadialOffset", [](const auto& model)
            { return exportColumn(model->template table<CurvedBeamPropTable>().centroidRadialOffsetColumn()); });
        registerColumn(catalogue, type, "centerZ", [](const auto& model)
                       { return exportColumn(model->template table<CurvedBeamPropTable>().centerZColumn()); });
    }
    {
        using model::BushProp2DFreqTable;
        const std::uint16_t type = model::typeIndexOf<BushProp2DFreqTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BushProp2DFreqTable>().idColumn()); });
        registerColumn(
            catalogue, type, "stiffnessTableIds", [](const auto& model)
            { return exportColumn(model->template table<BushProp2DFreqTable>().stiffnessTableIdsColumn()); });
        registerColumn(catalogue, type, "dampingTableIds", [](const auto& model)
                       { return exportColumn(model->template table<BushProp2DFreqTable>().dampingTableIdsColumn()); });
        registerColumn(catalogue, type, "massTableIds", [](const auto& model)
                       { return exportColumn(model->template table<BushProp2DFreqTable>().massTableIdsColumn()); });
    }
    {
        using model::BushPropTable;
        const std::uint16_t type = model::typeIndexOf<BushPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BushPropTable>().idColumn()); });
        registerColumn(catalogue, type, "stiffness", [](const auto& model)
                       { return exportColumn(model->template table<BushPropTable>().stiffnessColumn()); });
        registerColumn(catalogue, type, "damping", [](const auto& model)
                       { return exportColumn(model->template table<BushPropTable>().dampingColumn()); });
        registerColumn(catalogue, type, "structuralDamping", [](const auto& model)
                       { return exportColumn(model->template table<BushPropTable>().structuralDampingColumn()); });
        registerColumn(catalogue, type, "recovery", [](const auto& model)
                       { return exportColumn(model->template table<BushPropTable>().recoveryColumn()); });
        registerColumn(catalogue, type, "mass", [](const auto& model)
                       { return exportColumn(model->template table<BushPropTable>().massColumn()); });
        registerColumn(catalogue, type, "thermal", [](const auto& model)
                       { return exportColumn(model->template table<BushPropTable>().thermalColumn()); });
    }
    {
        using model::BushProp1DTable;
        const std::uint16_t type = model::typeIndexOf<BushProp1DTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BushProp1DTable>().idColumn()); });
        registerColumn(catalogue, type, "stiffness", [](const auto& model)
                       { return exportColumn(model->template table<BushProp1DTable>().stiffnessColumn()); });
        registerColumn(catalogue, type, "damping", [](const auto& model)
                       { return exportColumn(model->template table<BushProp1DTable>().dampingColumn()); });
        registerColumn(catalogue, type, "mass", [](const auto& model)
                       { return exportColumn(model->template table<BushProp1DTable>().massColumn()); });
        registerColumn(catalogue, type, "stressFactor", [](const auto& model)
                       { return exportColumn(model->template table<BushProp1DTable>().stressFactorColumn()); });
        registerColumn(catalogue, type, "strainFactor", [](const auto& model)
                       { return exportColumn(model->template table<BushProp1DTable>().strainFactorColumn()); });
        registerColumn(catalogue, type, "groups", [](const auto& model)
                       { return exportColumn(model->template table<BushProp1DTable>().groupsColumn()); });
        registerColumn(catalogue, type, "shockType", [](const auto& model)
                       { return exportColumn(model->template table<BushProp1DTable>().shockTypeColumn()); });
        registerColumn(catalogue, type, "shock", [](const auto& model)
                       { return exportColumn(model->template table<BushProp1DTable>().shockColumn()); });
        registerColumn(catalogue, type, "shockIds", [](const auto& model)
                       { return exportColumn(model->template table<BushProp1DTable>().shockIdsColumn()); });
        registerColumn(catalogue, type, "springType", [](const auto& model)
                       { return exportColumn(model->template table<BushProp1DTable>().springTypeColumn()); });
        registerColumn(catalogue, type, "springIds", [](const auto& model)
                       { return exportColumn(model->template table<BushProp1DTable>().springIdsColumn()); });
        registerColumn(catalogue, type, "damperType", [](const auto& model)
                       { return exportColumn(model->template table<BushProp1DTable>().damperTypeColumn()); });
        registerColumn(catalogue, type, "damperIds", [](const auto& model)
                       { return exportColumn(model->template table<BushProp1DTable>().damperIdsColumn()); });
        registerColumn(catalogue, type, "generalIds", [](const auto& model)
                       { return exportColumn(model->template table<BushProp1DTable>().generalIdsColumn()); });
    }
    {
        using model::BushProp2DTable;
        const std::uint16_t type = model::typeIndexOf<BushProp2DTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BushProp2DTable>().idColumn()); });
    }
    {
        using model::BushPropFreqTable;
        const std::uint16_t type = model::typeIndexOf<BushPropFreqTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BushPropFreqTable>().idColumn()); });
        registerColumn(catalogue, type, "stiffnessTableIds", [](const auto& model)
                       { return exportColumn(model->template table<BushPropFreqTable>().stiffnessTableIdsColumn()); });
        registerColumn(catalogue, type, "dampingTableIds", [](const auto& model)
                       { return exportColumn(model->template table<BushPropFreqTable>().dampingTableIdsColumn()); });
        registerColumn(
            catalogue, type, "structuralDampingTableIds", [](const auto& model)
            { return exportColumn(model->template table<BushPropFreqTable>().structuralDampingTableIdsColumn()); });
        registerColumn(
            catalogue, type, "nonlinearForceTableIds", [](const auto& model)
            { return exportColumn(model->template table<BushPropFreqTable>().nonlinearForceTableIdsColumn()); });
        registerColumn(
            catalogue, type, "forceDeflectionRule", [](const auto& model)
            { return exportColumn(model->template table<BushPropFreqTable>().forceDeflectionRuleColumn()); });
        registerColumn(catalogue, type, "fuseMode", [](const auto& model)
                       { return exportColumn(model->template table<BushPropFreqTable>().fuseModeColumn()); });
        registerColumn(catalogue, type, "direction", [](const auto& model)
                       { return exportColumn(model->template table<BushPropFreqTable>().directionColumn()); });
        registerColumn(catalogue, type, "fuseOption", [](const auto& model)
                       { return exportColumn(model->template table<BushPropFreqTable>().fuseOptionColumn()); });
        registerColumn(catalogue, type, "bounds", [](const auto& model)
                       { return exportColumn(model->template table<BushPropFreqTable>().boundsColumn()); });
        registerColumn(
            catalogue, type, "fuseStiffnessRetention", [](const auto& model)
            { return exportColumn(model->template table<BushPropFreqTable>().fuseStiffnessRetentionColumn()); });
        registerColumn(catalogue, type, "largeRotationEndA", [](const auto& model)
                       { return exportColumn(model->template table<BushPropFreqTable>().largeRotationEndAColumn()); });
    }
    {
        using model::CompPropTable;
        const std::uint16_t type = model::typeIndexOf<CompPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CompPropTable>().idColumn()); });
        registerColumn(catalogue, type, "bottomOffset", [](const auto& model)
                       { return exportColumn(model->template table<CompPropTable>().bottomOffsetColumn()); });
        registerColumn(catalogue, type, "bottomOffsetGiven", [](const auto& model)
                       { return exportColumn(model->template table<CompPropTable>().bottomOffsetGivenColumn()); });
        registerColumn(catalogue, type, "nonStructMass", [](const auto& model)
                       { return exportColumn(model->template table<CompPropTable>().nonStructMassColumn()); });
        registerColumn(catalogue, type, "bondAllowable", [](const auto& model)
                       { return exportColumn(model->template table<CompPropTable>().bondShearAllowableColumn()); });
        registerColumn(catalogue, type, "failureTheory", [](const auto& model)
                       { return exportColumn(model->template table<CompPropTable>().failureTheoryColumn()); });
        registerColumn(catalogue, type, "referenceTemperature", [](const auto& model)
                       { return exportColumn(model->template table<CompPropTable>().referenceTemperatureColumn()); });
        registerColumn(catalogue, type, "structuralDamping", [](const auto& model)
                       { return exportColumn(model->template table<CompPropTable>().structuralDampingColumn()); });
        registerColumn(catalogue, type, "laminateOption", [](const auto& model)
                       { return exportColumn(model->template table<CompPropTable>().optionColumn()); });
        registerColumn(catalogue, type, "plyMid", [](const auto& model)
                       { return exportVariableColumn(model->template table<CompPropTable>().plyMatIdColumn()); });
        registerColumn(catalogue, type, "plyThickness", [](const auto& model)
                       { return exportVariableColumn(model->template table<CompPropTable>().plyThicknessColumn()); });
        registerColumn(catalogue, type, "plyTheta", [](const auto& model)
                       { return exportVariableColumn(model->template table<CompPropTable>().plyAngleColumn()); });
        registerColumn(
            catalogue, type, "plySout", [](const auto& model)
            { return exportVariableColumn(model->template table<CompPropTable>().plyStressOutputColumn()); });
    }
    {
        using model::CompPropExplicitTable;
        const std::uint16_t type = model::typeIndexOf<CompPropExplicitTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CompPropExplicitTable>().idColumn()); });
    }
    {
        using model::CompPropIntegrationTable;
        const std::uint16_t type = model::typeIndexOf<CompPropIntegrationTable>();
        registerColumn(catalogue, type, "integration", [](const auto& model)
                       { return exportColumn(model->template table<CompPropIntegrationTable>().integrationColumn()); });
    }
    {
        using model::CompPropFreqDampingTable;
        const std::uint16_t type = model::typeIndexOf<CompPropFreqDampingTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CompPropFreqDampingTable>().idColumn()); });
        registerColumn(
            catalogue, type, "structuralDamping", [](const auto& model)
            { return exportColumn(model->template table<CompPropFreqDampingTable>().structuralDampingColumn()); });
    }
    {
        using model::CompPropGlobalTable;
        const std::uint16_t type = model::typeIndexOf<CompPropGlobalTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CompPropGlobalTable>().idColumn()); });
        registerColumn(catalogue, type, "bottomOffset", [](const auto& model)
                       { return exportColumn(model->template table<CompPropGlobalTable>().bottomOffsetColumn()); });
        registerColumn(
            catalogue, type, "bottomOffsetGiven", [](const auto& model)
            { return exportColumn(model->template table<CompPropGlobalTable>().bottomOffsetGivenColumn()); });
        registerColumn(catalogue, type, "nonStructMass", [](const auto& model)
                       { return exportColumn(model->template table<CompPropGlobalTable>().nonStructMassColumn()); });
        registerColumn(
            catalogue, type, "bondAllowable", [](const auto& model)
            { return exportColumn(model->template table<CompPropGlobalTable>().bondShearAllowableColumn()); });
        registerColumn(catalogue, type, "failureTheory", [](const auto& model)
                       { return exportColumn(model->template table<CompPropGlobalTable>().failureTheoryColumn()); });
        registerColumn(
            catalogue, type, "referenceTemperature", [](const auto& model)
            { return exportColumn(model->template table<CompPropGlobalTable>().referenceTemperatureColumn()); });
        registerColumn(
            catalogue, type, "structuralDamping", [](const auto& model)
            { return exportColumn(model->template table<CompPropGlobalTable>().structuralDampingColumn()); });
        registerColumn(catalogue, type, "laminateOption", [](const auto& model)
                       { return exportColumn(model->template table<CompPropGlobalTable>().optionColumn()); });
        registerColumn(
            catalogue, type, "structuralDampingFlag", [](const auto& model)
            { return exportColumn(model->template table<CompPropGlobalTable>().structuralDampingFlagColumn()); });
        registerColumn(
            catalogue, type, "plyGlobalId", [](const auto& model)
            { return exportVariableColumn(model->template table<CompPropGlobalTable>().plyGlobalIdColumn()); });
        registerColumn(catalogue, type, "plyMid", [](const auto& model)
                       { return exportVariableColumn(model->template table<CompPropGlobalTable>().plyMatIdColumn()); });
        registerColumn(
            catalogue, type, "plyThickness", [](const auto& model)
            { return exportVariableColumn(model->template table<CompPropGlobalTable>().plyThicknessColumn()); });
        registerColumn(catalogue, type, "plyTheta", [](const auto& model)
                       { return exportVariableColumn(model->template table<CompPropGlobalTable>().plyAngleColumn()); });
        registerColumn(
            catalogue, type, "plySout", [](const auto& model)
            { return exportVariableColumn(model->template table<CompPropGlobalTable>().plyStressOutputColumn()); });
    }
    {
        using model::CompSolidPropNonlinearTable;
        const std::uint16_t type = model::typeIndexOf<CompSolidPropNonlinearTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CompSolidPropNonlinearTable>().idColumn()); });
    }
    {
        using model::CompSolidPropTable;
        const std::uint16_t type = model::typeIndexOf<CompSolidPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CompSolidPropTable>().idColumn()); });
    }
    {
        using model::DamperPropTable;
        const std::uint16_t type = model::typeIndexOf<DamperPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DamperPropTable>().idColumn()); });
        registerColumn(catalogue, type, "damping", [](const auto& model)
                       { return exportColumn(model->template table<DamperPropTable>().dampingColumn()); });
    }
    {
        using model::HeatCapacityProp5Table;
        const std::uint16_t type = model::typeIndexOf<HeatCapacityProp5Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HeatCapacityProp5Table>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<HeatCapacityProp5Table>().matIdColumn()); });
        registerColumn(catalogue, type, "heatCapacity", [](const auto& model)
                       { return exportColumn(model->template table<HeatCapacityProp5Table>().heatCapacityColumn()); });
    }
    {
        using model::DamperPropFreqTable;
        const std::uint16_t type = model::typeIndexOf<DamperPropFreqTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DamperPropFreqTable>().idColumn()); });
        registerColumn(catalogue, type, "dampingTableId", [](const auto& model)
                       { return exportColumn(model->template table<DamperPropFreqTable>().dampingTableIdColumn()); });
    }
    {
        using model::PropDistributionTable;
        const std::uint16_t type = model::typeIndexOf<PropDistributionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PropDistributionTable>().idColumn()); });
    }
    {
        using model::PropMatDistributionTable;
        const std::uint16_t type = model::typeIndexOf<PropMatDistributionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PropMatDistributionTable>().idColumn()); });
    }
    {
        using model::SpringPropTable;
        const std::uint16_t type = model::typeIndexOf<SpringPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SpringPropTable>().idColumn()); });
        registerColumn(catalogue, type, "stiffness", [](const auto& model)
                       { return exportColumn(model->template table<SpringPropTable>().stiffnessColumn()); });
        registerColumn(catalogue, type, "structuralDamping", [](const auto& model)
                       { return exportColumn(model->template table<SpringPropTable>().structuralDampingColumn()); });
        registerColumn(catalogue, type, "stressCoefficient", [](const auto& model)
                       { return exportColumn(model->template table<SpringPropTable>().stressCoefficientColumn()); });
    }
    {
        using model::SpringPropTableTable;
        const std::uint16_t type = model::typeIndexOf<SpringPropTableTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SpringPropTableTable>().idColumn()); });
        registerColumn(catalogue, type, "tableId", [](const auto& model)
                       { return exportColumn(model->template table<SpringPropTableTable>().tableIdColumn()); });
    }
    {
        using model::SpringPropFreqTable;
        const std::uint16_t type = model::typeIndexOf<SpringPropFreqTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SpringPropFreqTable>().idColumn()); });
        registerColumn(catalogue, type, "stiffnessTableId", [](const auto& model)
                       { return exportColumn(model->template table<SpringPropFreqTable>().stiffnessTableIdColumn()); });
        registerColumn(
            catalogue, type, "structuralDampingTableId", [](const auto& model)
            { return exportColumn(model->template table<SpringPropFreqTable>().structuralDampingTableIdColumn()); });
        registerColumn(
            catalogue, type, "nonlinearForceTableId", [](const auto& model)
            { return exportColumn(model->template table<SpringPropFreqTable>().nonlinearForceTableIdColumn()); });
    }
    {
        using model::FastenerPropTable;
        const std::uint16_t type = model::typeIndexOf<FastenerPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FastenerPropTable>().idColumn()); });
        registerColumn(catalogue, type, "diameter", [](const auto& model)
                       { return exportColumn(model->template table<FastenerPropTable>().diameterColumn()); });
        registerColumn(catalogue, type, "matFrameId", [](const auto& model)
                       { return exportColumn(model->template table<FastenerPropTable>().matFrameIdColumn()); });
        registerColumn(catalogue, type, "absoluteFrame", [](const auto& model)
                       { return exportColumn(model->template table<FastenerPropTable>().absoluteFrameColumn()); });
        registerColumn(
            catalogue, type, "translationalStiffness", [](const auto& model)
            { return exportColumn(model->template table<FastenerPropTable>().translationalStiffnessColumn()); });
        registerColumn(
            catalogue, type, "rotationalStiffness", [](const auto& model)
            { return exportColumn(model->template table<FastenerPropTable>().rotationalStiffnessColumn()); });
        registerColumn(catalogue, type, "mass", [](const auto& model)
                       { return exportColumn(model->template table<FastenerPropTable>().massColumn()); });
        registerColumn(catalogue, type, "structuralDamping", [](const auto& model)
                       { return exportColumn(model->template table<FastenerPropTable>().structuralDampingColumn()); });
        registerColumn(catalogue, type, "alphaCoefficient", [](const auto& model)
                       { return exportColumn(model->template table<FastenerPropTable>().alphaCoefficientColumn()); });
        registerColumn(
            catalogue, type, "referenceTemperature", [](const auto& model)
            { return exportColumn(model->template table<FastenerPropTable>().referenceTemperatureColumn()); });
        registerColumn(catalogue, type, "coincidentLength", [](const auto& model)
                       { return exportColumn(model->template table<FastenerPropTable>().coincidentLengthColumn()); });
    }
    {
        using model::FastenerPropFreqTable;
        const std::uint16_t type = model::typeIndexOf<FastenerPropFreqTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FastenerPropFreqTable>().idColumn()); });
        registerColumn(
            catalogue, type, "stiffnessTableIds", [](const auto& model)
            { return exportColumn(model->template table<FastenerPropFreqTable>().stiffnessTableIdsColumn()); });
        registerColumn(
            catalogue, type, "structuralDampingTableId", [](const auto& model)
            { return exportColumn(model->template table<FastenerPropFreqTable>().structuralDampingTableIdColumn()); });
        registerColumn(
            catalogue, type, "nonlinearForceTableIds", [](const auto& model)
            { return exportColumn(model->template table<FastenerPropFreqTable>().nonlinearForceTableIdsColumn()); });
        registerColumn(
            catalogue, type, "forceDeflectionRule", [](const auto& model)
            { return exportColumn(model->template table<FastenerPropFreqTable>().forceDeflectionRuleColumn()); });
        registerColumn(catalogue, type, "fuseMode", [](const auto& model)
                       { return exportColumn(model->template table<FastenerPropFreqTable>().fuseModeColumn()); });
        registerColumn(catalogue, type, "direction", [](const auto& model)
                       { return exportColumn(model->template table<FastenerPropFreqTable>().directionColumn()); });
        registerColumn(catalogue, type, "fuseOption", [](const auto& model)
                       { return exportColumn(model->template table<FastenerPropFreqTable>().fuseOptionColumn()); });
        registerColumn(catalogue, type, "bounds", [](const auto& model)
                       { return exportColumn(model->template table<FastenerPropFreqTable>().boundsColumn()); });
        registerColumn(
            catalogue, type, "fuseStiffnessRetention", [](const auto& model)
            { return exportColumn(model->template table<FastenerPropFreqTable>().fuseStiffnessRetentionColumn()); });
        registerColumn(
            catalogue, type, "largeRotationEndA", [](const auto& model)
            { return exportColumn(model->template table<FastenerPropFreqTable>().largeRotationEndAColumn()); });
    }
    {
        using model::GapPropTable;
        const std::uint16_t type = model::typeIndexOf<GapPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<GapPropTable>().idColumn()); });
        registerColumn(catalogue, type, "initialOpening", [](const auto& model)
                       { return exportColumn(model->template table<GapPropTable>().initialOpeningColumn()); });
        registerColumn(catalogue, type, "preload", [](const auto& model)
                       { return exportColumn(model->template table<GapPropTable>().preloadColumn()); });
        registerColumn(catalogue, type, "closedStiffness", [](const auto& model)
                       { return exportColumn(model->template table<GapPropTable>().closedStiffnessColumn()); });
        registerColumn(catalogue, type, "openStiffness", [](const auto& model)
                       { return exportColumn(model->template table<GapPropTable>().openStiffnessColumn()); });
        registerColumn(catalogue, type, "transverseStiffness", [](const auto& model)
                       { return exportColumn(model->template table<GapPropTable>().transverseStiffnessColumn()); });
        registerColumn(catalogue, type, "frictionCoeff1", [](const auto& model)
                       { return exportColumn(model->template table<GapPropTable>().frictionCoeff1Column()); });
        registerColumn(catalogue, type, "frictionCoeff2", [](const auto& model)
                       { return exportColumn(model->template table<GapPropTable>().frictionCoeff2Column()); });
        registerColumn(catalogue, type, "maxPenetration", [](const auto& model)
                       { return exportColumn(model->template table<GapPropTable>().maxPenetrationColumn()); });
        registerColumn(catalogue, type, "maxAdjustmentRatio", [](const auto& model)
                       { return exportColumn(model->template table<GapPropTable>().maxAdjustmentRatioColumn()); });
        registerColumn(catalogue, type, "minPenetrationRatio", [](const auto& model)
                       { return exportColumn(model->template table<GapPropTable>().minPenetrationRatioColumn()); });
    }
    {
        using model::PlaneCompPropTable;
        const std::uint16_t type = model::typeIndexOf<PlaneCompPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PlaneCompPropTable>().idColumn()); });
    }
    {
        using model::PlanePropNonlinearTable;
        const std::uint16_t type = model::typeIndexOf<PlanePropNonlinearTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PlanePropNonlinearTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<PlanePropNonlinearTable>().matIdColumn()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<PlanePropNonlinearTable>().frameIdColumn()); });
        registerColumn(catalogue, type, "gaussOutput", [](const auto& model)
                       { return exportColumn(model->template table<PlanePropNonlinearTable>().gaussOutputColumn()); });
    }
    {
        using model::SolidPropHyperelasticTable;
        const std::uint16_t type = model::typeIndexOf<SolidPropHyperelasticTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SolidPropHyperelasticTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<SolidPropHyperelasticTable>().matIdColumn()); });
        registerColumn(
            catalogue, type, "gaussOutput", [](const auto& model)
            { return exportColumn(model->template table<SolidPropHyperelasticTable>().gaussOutputColumn()); });
    }
    {
        using model::MassPropTable;
        const std::uint16_t type = model::typeIndexOf<MassPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<MassPropTable>().idColumn()); });
        registerColumn(catalogue, type, "mass", [](const auto& model)
                       { return exportColumn(model->template table<MassPropTable>().massColumn()); });
    }
    {
        using model::PlanePropTable;
        const std::uint16_t type = model::typeIndexOf<PlanePropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PlanePropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<PlanePropTable>().matIdColumn()); });
        registerColumn(catalogue, type, "thickness", [](const auto& model)
                       { return exportColumn(model->template table<PlanePropTable>().tColumn()); });
        registerColumn(catalogue, type, "nonStructMass", [](const auto& model)
                       { return exportColumn(model->template table<PlanePropTable>().nonStructMassColumn()); });
        registerColumn(catalogue, type, "formulationOption", [](const auto& model)
                       { return exportColumn(model->template table<PlanePropTable>().formulationOptionColumn()); });
    }
    {
        using model::RodPropTable;
        const std::uint16_t type = model::typeIndexOf<RodPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RodPropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<RodPropTable>().matIdColumn()); });
        registerColumn(catalogue, type, "area", [](const auto& model)
                       { return exportColumn(model->template table<RodPropTable>().areaColumn()); });
        registerColumn(catalogue, type, "torsionConstant", [](const auto& model)
                       { return exportColumn(model->template table<RodPropTable>().torsionColumn()); });
        registerColumn(catalogue, type, "torsionStressCoefficient", [](const auto& model)
                       { return exportColumn(model->template table<RodPropTable>().stressCoefficientColumn()); });
        registerColumn(catalogue, type, "nonStructMass", [](const auto& model)
                       { return exportColumn(model->template table<RodPropTable>().nonStructMassColumn()); });
    }
    {
        using model::RodPropNonlinearTable;
        const std::uint16_t type = model::typeIndexOf<RodPropNonlinearTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RodPropNonlinearTable>().idColumn()); });
    }
    {
        using model::ShearPanelPropTable;
        const std::uint16_t type = model::typeIndexOf<ShearPanelPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ShearPanelPropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<ShearPanelPropTable>().matIdColumn()); });
        registerColumn(catalogue, type, "thickness", [](const auto& model)
                       { return exportColumn(model->template table<ShearPanelPropTable>().thicknessColumn()); });
        registerColumn(catalogue, type, "nonStructMass", [](const auto& model)
                       { return exportColumn(model->template table<ShearPanelPropTable>().nonStructMassColumn()); });
        registerColumn(
            catalogue, type, "extensionalFactor1", [](const auto& model)
            { return exportColumn(model->template table<ShearPanelPropTable>().extensionalFactor1Column()); });
        registerColumn(
            catalogue, type, "extensionalFactor2", [](const auto& model)
            { return exportColumn(model->template table<ShearPanelPropTable>().extensionalFactor2Column()); });
    }
    {
        using model::ShearPanelPropNonlinearTable;
        const std::uint16_t type = model::typeIndexOf<ShearPanelPropNonlinearTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ShearPanelPropNonlinearTable>().idColumn()); });
    }
    {
        using model::ShellPropTable;
        const std::uint16_t type = model::typeIndexOf<ShellPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ShellPropTable>().idColumn()); });
        registerColumn(catalogue, type, "membraneMatId", [](const auto& model)
                       { return exportColumn(model->template table<ShellPropTable>().membraneMatIdColumn()); });
        registerColumn(catalogue, type, "thickness", [](const auto& model)
                       { return exportColumn(model->template table<ShellPropTable>().thicknessColumn()); });
        registerColumn(catalogue, type, "bendingMatId", [](const auto& model)
                       { return exportColumn(model->template table<ShellPropTable>().bendingMatIdColumn()); });
        registerColumn(catalogue, type, "bendingStiffnessRatio", [](const auto& model)
                       { return exportColumn(model->template table<ShellPropTable>().bendingRatioColumn()); });
        registerColumn(catalogue, type, "shearMatId", [](const auto& model)
                       { return exportColumn(model->template table<ShellPropTable>().shearMatIdColumn()); });
        registerColumn(catalogue, type, "shearThicknessRatio", [](const auto& model)
                       { return exportColumn(model->template table<ShellPropTable>().shearRatioColumn()); });
        registerColumn(catalogue, type, "nonStructMass", [](const auto& model)
                       { return exportColumn(model->template table<ShellPropTable>().nonStructMassColumn()); });
        registerColumn(catalogue, type, "fiberDistance1", [](const auto& model)
                       { return exportColumn(model->template table<ShellPropTable>().fiberDistance1Column()); });
        registerColumn(catalogue, type, "fiberDistance2", [](const auto& model)
                       { return exportColumn(model->template table<ShellPropTable>().fiberDistance2Column()); });
        registerColumn(catalogue, type, "couplingMatId", [](const auto& model)
                       { return exportColumn(model->template table<ShellPropTable>().couplingMatIdColumn()); });
    }
    {
        using model::ShellPropNodalTable;
        const std::uint16_t type = model::typeIndexOf<ShellPropNodalTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ShellPropNodalTable>().idColumn()); });
    }
    {
        using model::ShellPropNonlinearTable;
        const std::uint16_t type = model::typeIndexOf<ShellPropNonlinearTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ShellPropNonlinearTable>().idColumn()); });
    }
    {
        using model::PlanePropNonlinearExtTable;
        const std::uint16_t type = model::typeIndexOf<PlanePropNonlinearExtTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PlanePropNonlinearExtTable>().idColumn()); });
    }
    {
        using model::PerforatedShellPropTable;
        const std::uint16_t type = model::typeIndexOf<PerforatedShellPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PerforatedShellPropTable>().idColumn()); });
    }
    {
        using model::SolidPropTable;
        const std::uint16_t type = model::typeIndexOf<SolidPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SolidPropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<SolidPropTable>().matIdColumn()); });
        registerColumn(catalogue, type, "matFrameId", [](const auto& model)
                       { return exportColumn(model->template table<SolidPropTable>().matFrameIdColumn()); });
        registerColumn(catalogue, type, "integration", [](const auto& model)
                       { return exportTextColumn(model->template table<SolidPropTable>().integrationColumn()); });
        registerColumn(catalogue, type, "stressLocation", [](const auto& model)
                       { return exportTextColumn(model->template table<SolidPropTable>().stressLocationColumn()); });
        registerColumn(catalogue, type, "integrationOrder", [](const auto& model)
                       { return exportTextColumn(model->template table<SolidPropTable>().integrationOrderColumn()); });
        registerColumn(catalogue, type, "elementFunction", [](const auto& model)
                       { return exportTextColumn(model->template table<SolidPropTable>().elementFunctionColumn()); });
    }
    {
        using model::SpringOffsetPropTable;
        const std::uint16_t type = model::typeIndexOf<SpringOffsetPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SpringOffsetPropTable>().idColumn()); });
        registerColumn(catalogue, type, "stiffness", [](const auto& model)
                       { return exportColumn(model->template table<SpringOffsetPropTable>().stiffnessColumn()); });
        registerColumn(
            catalogue, type, "tensileFailureForce", [](const auto& model)
            { return exportColumn(model->template table<SpringOffsetPropTable>().tensileFailureForceColumn()); });
        registerColumn(
            catalogue, type, "compressiveFailureForce", [](const auto& model)
            { return exportColumn(model->template table<SpringOffsetPropTable>().compressiveFailureForceColumn()); });
        registerColumn(catalogue, type, "loadingTableId", [](const auto& model)
                       { return exportColumn(model->template table<SpringOffsetPropTable>().loadingTableIdColumn()); });
        registerColumn(
            catalogue, type, "unloadingTableId", [](const auto& model)
            { return exportColumn(model->template table<SpringOffsetPropTable>().unloadingTableIdColumn()); });
    }
    {
        using model::TubePropTable;
        const std::uint16_t type = model::typeIndexOf<TubePropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TubePropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<TubePropTable>().matIdColumn()); });
        registerColumn(catalogue, type, "outerDiameter", [](const auto& model)
                       { return exportColumn(model->template table<TubePropTable>().outerDiameterColumn()); });
        registerColumn(catalogue, type, "thickness", [](const auto& model)
                       { return exportColumn(model->template table<TubePropTable>().thicknessColumn()); });
        registerColumn(catalogue, type, "nonStructMass", [](const auto& model)
                       { return exportColumn(model->template table<TubePropTable>().nonStructMassColumn()); });
        registerColumn(catalogue, type, "outerDiameter2", [](const auto& model)
                       { return exportColumn(model->template table<TubePropTable>().outerDiameter2Column()); });
        registerColumn(catalogue, type, "thicknessGiven", [](const auto& model)
                       { return exportColumn(model->template table<TubePropTable>().thicknessGivenColumn()); });
        registerColumn(catalogue, type, "outerDiameter2Given", [](const auto& model)
                       { return exportColumn(model->template table<TubePropTable>().outerDiameter2GivenColumn()); });
    }
    {
        using model::ViscousDamperPropTable;
        const std::uint16_t type = model::typeIndexOf<ViscousDamperPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ViscousDamperPropTable>().idColumn()); });
        registerColumn(
            catalogue, type, "extensionalDamping", [](const auto& model)
            { return exportColumn(model->template table<ViscousDamperPropTable>().extensionalDampingColumn()); });
        registerColumn(
            catalogue, type, "rotationalDamping", [](const auto& model)
            { return exportColumn(model->template table<ViscousDamperPropTable>().rotationalDampingColumn()); });
    }
    {
        using model::ViscousDamperPropNonlinearTable;
        const std::uint16_t type = model::typeIndexOf<ViscousDamperPropNonlinearTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ViscousDamperPropNonlinearTable>().idColumn()); });
        registerColumn(
            catalogue, type, "tableId", [](const auto& model)
            { return exportColumn(model->template table<ViscousDamperPropNonlinearTable>().tableIdColumn()); });
    }
    {
        using model::WeldPropTable;
        const std::uint16_t type = model::typeIndexOf<WeldPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<WeldPropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<WeldPropTable>().matIdColumn()); });
        registerColumn(catalogue, type, "diameter", [](const auto& model)
                       { return exportColumn(model->template table<WeldPropTable>().diameterColumn()); });
        registerColumn(catalogue, type, "eliminateConstraints", [](const auto& model)
                       { return exportColumn(model->template table<WeldPropTable>().eliminateConstraintsColumn()); });
        registerColumn(catalogue, type, "spot", [](const auto& model)
                       { return exportColumn(model->template table<WeldPropTable>().spotColumn()); });
        registerColumn(catalogue, type, "minLengthRatio", [](const auto& model)
                       { return exportColumn(model->template table<WeldPropTable>().minLengthRatioColumn()); });
        registerColumn(catalogue, type, "maxLengthRatio", [](const auto& model)
                       { return exportColumn(model->template table<WeldPropTable>().maxLengthRatioColumn()); });
    }
    {
        using model::TiltingPadBearingPropTable;
        const std::uint16_t type = model::typeIndexOf<TiltingPadBearingPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TiltingPadBearingPropTable>().idColumn()); });
    }
}

} // namespace oor::query::column_export
