// The stored columns of the cards of the family "aero", for the query engine (ColumnExport.hpp).
// One block a card type: its type index, then a getter a column.

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/aero/aeroBeamSpline2/AeroBeamSpline2Table.hpp"
#include "oor/model/cards/aero/aeroBeamSpline5/AeroBeamSpline5Table.hpp"
#include "oor/model/cards/aero/aeroBody2/AeroBody2Table.hpp"
#include "oor/model/cards/aero/aeroBodyProp2/AeroBodyProp2Table.hpp"
#include "oor/model/cards/aero/aeroBoxList/AeroBoxListTable.hpp"
#include "oor/model/cards/aero/aeroComponent/AeroComponentTable.hpp"
#include "oor/model/cards/aero/aeroComponentUnion/AeroComponentUnionTable.hpp"
#include "oor/model/cards/aero/aeroControlSchedule/AeroControlScheduleTable.hpp"
#include "oor/model/cards/aero/aeroControlSurface/AeroControlSurfaceTable.hpp"
#include "oor/model/cards/aero/aeroControlSurfaceNodes/AeroControlSurfaceNodesTable.hpp"
#include "oor/model/cards/aero/aeroControlVector/AeroControlVectorTable.hpp"
#include "oor/model/cards/aero/aeroController/AeroControllerTable.hpp"
#include "oor/model/cards/aero/aeroDivergence/AeroDivergenceTable.hpp"
#include "oor/model/cards/aero/aeroDownwash/AeroDownwashTable.hpp"
#include "oor/model/cards/aero/aeroFactorList/AeroFactorListTable.hpp"
#include "oor/model/cards/aero/aeroFiniteBeamSpline7/AeroFiniteBeamSpline7Table.hpp"
#include "oor/model/cards/aero/aeroFiniteSurfaceSpline6/AeroFiniteSurfaceSpline6Table.hpp"
#include "oor/model/cards/aero/aeroFlutter/AeroFlutterTable.hpp"
#include "oor/model/cards/aero/aeroFlutterFactors/AeroFlutterFactorsTable.hpp"
#include "oor/model/cards/aero/aeroForceVector/AeroForceVectorTable.hpp"
#include "oor/model/cards/aero/aeroGust/AeroGustTable.hpp"
#include "oor/model/cards/aero/aeroLabelList/AeroLabelListTable.hpp"
#include "oor/model/cards/aero/aeroLiftingPanel1/AeroLiftingPanel1Table.hpp"
#include "oor/model/cards/aero/aeroLiftingPanelProp1/AeroLiftingPanelProp1Table.hpp"
#include "oor/model/cards/aero/aeroMachBoxPanel3/AeroMachBoxPanel3Table.hpp"
#include "oor/model/cards/aero/aeroMachBoxProp3/AeroMachBoxProp3Table.hpp"
#include "oor/model/cards/aero/aeroMachFreqPairs2/AeroMachFreqPairs2Table.hpp"
#include "oor/model/cards/aero/aeroMachFreqTable1/AeroMachFreqTable1Table.hpp"
#include "oor/model/cards/aero/aeroNode/AeroNodeTable.hpp"
#include "oor/model/cards/aero/aeroNodeScale/AeroNodeScaleTable.hpp"
#include "oor/model/cards/aero/aeroPistonPanel5/AeroPistonPanel5Table.hpp"
#include "oor/model/cards/aero/aeroPistonProp5/AeroPistonProp5Table.hpp"
#include "oor/model/cards/aero/aeroPressureVector/AeroPressureVectorTable.hpp"
#include "oor/model/cards/aero/aeroQuad4/AeroQuad4Table.hpp"
#include "oor/model/cards/aero/aeroRigidSpline/AeroRigidSplineTable.hpp"
#include "oor/model/cards/aero/aeroRigidTrimVariable/AeroRigidTrimVariableTable.hpp"
#include "oor/model/cards/aero/aeroSplineCurveBlend/AeroSplineCurveBlendTable.hpp"
#include "oor/model/cards/aero/aeroSplineEquation3/AeroSplineEquation3Table.hpp"
#include "oor/model/cards/aero/aeroSplineRelaxation/AeroSplineRelaxationTable.hpp"
#include "oor/model/cards/aero/aeroSplineStripBlend/AeroSplineStripBlendTable.hpp"
#include "oor/model/cards/aero/aeroStaticParams/AeroStaticParamsTable.hpp"
#include "oor/model/cards/aero/aeroStripPanel4/AeroStripPanel4Table.hpp"
#include "oor/model/cards/aero/aeroStripProp4/AeroStripProp4Table.hpp"
#include "oor/model/cards/aero/aeroSurfaceSpline1/AeroSurfaceSpline1Table.hpp"
#include "oor/model/cards/aero/aeroSurfaceSpline4/AeroSurfaceSpline4Table.hpp"
#include "oor/model/cards/aero/aeroTria3/AeroTria3Table.hpp"
#include "oor/model/cards/aero/aeroTrim/AeroTrimTable.hpp"
#include "oor/model/cards/aero/aeroTrim2/AeroTrim2Table.hpp"
#include "oor/model/cards/aero/aeroTrimLink/AeroTrimLinkTable.hpp"
#include "oor/model/cards/aero/aeroUnsteadyParams/AeroUnsteadyParamsTable.hpp"
#include "oor/model/cards/aero/aeroUserSpline/AeroUserSplineTable.hpp"
#include "oor/model/cards/aero/trimmedMaterialMatrices/TrimmedMaterialMatricesTable.hpp"
#include "oor/query/providers/ColumnExport.hpp"

namespace oor::query::column_export
{

void registerAeroColumns(StoredColumnCatalogue& catalogue)
{
    {
        using model::TrimmedMaterialMatricesTable;
        const std::uint16_t type = model::typeIndexOf<TrimmedMaterialMatricesTable>();
        registerColumn(
            catalogue, type, "name", [](const auto& model)
            { return exportTextColumn(model->template table<TrimmedMaterialMatricesTable>().nameColumn()); });
    }
    {
        using model::AeroComponentCard;
        using model::AeroComponentTable;
        const std::uint16_t type = model::typeIndexOf<AeroComponentTable>();
        registerColumn(catalogue, type, "name", [](const auto& model)
                       { return exportTextColumn(model->template table<AeroComponentTable>().nameColumn()); });
        registerColumn(catalogue, type, "listKind",
                       [](const auto& model)
                       {
                           // The neutral card name of the list kind (AeroBoxList or IdListSet1).
                           return exportCardValues(
                               model->template table<AeroComponentTable>(),
                               [](const model::AeroComponentCard& card)
                               {
                                   return std::string(
                                       model::AERO_COMPONENT_LIST_TYPE_WORDS[static_cast<std::size_t>(card.listKind)]);
                               });
                       });
        registerColumn(catalogue, type, "ids",
                       [](const auto& model)
                       {
                           return exportCardLists(model->template table<AeroComponentTable>(),
                                                  [](const model::AeroComponentCard& card) { return card.ids; });
                       });
    }
    {
        using model::AeroComponentUnionTable;
        const std::uint16_t type = model::typeIndexOf<AeroComponentUnionTable>();
        registerColumn(catalogue, type, "name", [](const auto& model)
                       { return exportTextColumn(model->template table<AeroComponentUnionTable>().nameColumn()); });
        registerColumn(catalogue, type, "labels",
                       [](const auto& model)
                       {
                           auto& table = model->template table<AeroComponentUnionTable>();
                           std::vector<std::vector<std::string>> labels;
                           for (std::uint32_t row = 0; row < table.rows().rowCount(); ++row)
                               labels.push_back(table.card(row).labels);
                           return exportTextListColumn(labels);
                       });
    }
    {
        using model::AeroDownwashTable;
        const std::uint16_t type = model::typeIndexOf<AeroDownwashTable>();
        registerColumn(catalogue, type, "controlVectorId", [](const auto& model)
                       { return exportColumn(model->template table<AeroDownwashTable>().controlVectorIdColumn()); });
    }
    {
        using model::AeroFactorListTable;
        const std::uint16_t type = model::typeIndexOf<AeroFactorListTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroFactorListTable>().idColumn()); });
        registerColumn(catalogue, type, "numbers", [](const auto& model)
                       { return exportVariableColumn(model->template table<AeroFactorListTable>().valuesColumn()); });
    }
    {
        using model::AeroForceVectorTable;
        const std::uint16_t type = model::typeIndexOf<AeroForceVectorTable>();
        registerColumn(catalogue, type, "controlVectorId", [](const auto& model)
                       { return exportColumn(model->template table<AeroForceVectorTable>().controlVectorIdColumn()); });
    }
    {
        using model::AeroNodeTable;
        const std::uint16_t type = model::typeIndexOf<AeroNodeTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroNodeTable>().idColumn()); });
    }
    {
        using model::AeroTrimLinkCard;
        using model::AeroTrimLinkTable;
        const std::uint16_t type = model::typeIndexOf<AeroTrimLinkTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroTrimLinkTable>().idColumn()); });
        registerColumn(catalogue, type, "always",
                       [](const auto& model)
                       {
                           return exportCardValues(model->template table<AeroTrimLinkTable>(),
                                                   [](const AeroTrimLinkCard& card)
                                                   { return static_cast<std::uint8_t>(card.always ? 1 : 0); });
                       });
        registerColumn(catalogue, type, "dependentLabel",
                       [](const auto& model)
                       {
                           return exportCardValues(model->template table<AeroTrimLinkTable>(),
                                                   [](const AeroTrimLinkCard& card) { return card.dependentLabel; });
                       });
        registerColumn(catalogue, type, "termLabels",
                       [](const auto& model)
                       {
                           auto& table = model->template table<AeroTrimLinkTable>();
                           std::vector<std::vector<std::string>> labels;
                           for (std::uint32_t row = 0; row < table.rows().rowCount(); ++row)
                           {
                               std::vector<std::string>& own = labels.emplace_back();
                               for (const model::LabeledValue& term : table.card(row).terms)
                                   own.push_back(term.label);
                           }
                           return exportTextListColumn(labels);
                       });
        registerColumn(catalogue, type, "termCoefficients",
                       [](const auto& model)
                       {
                           return exportCardLists(model->template table<AeroTrimLinkTable>(),
                                                  [](const AeroTrimLinkCard& card)
                                                  {
                                                      std::vector<double> values;
                                                      for (const model::LabeledValue& term : card.terms)
                                                          values.push_back(term.value);
                                                      return values;
                                                  });
                       });
    }
    {
        using model::AeroBoxListTable;
        const std::uint16_t type = model::typeIndexOf<AeroBoxListTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroBoxListTable>().idColumn()); });
        registerColumn(catalogue, type, "runs", [](const auto& model)
                       { return exportVariableColumn(model->template table<AeroBoxListTable>().runsColumn()); });
    }
    {
        using model::AeroLabelListTable;
        const std::uint16_t type = model::typeIndexOf<AeroLabelListTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroLabelListTable>().idColumn()); });
    }
    {
        using model::AeroControllerTable;
        const std::uint16_t type = model::typeIndexOf<AeroControllerTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroControllerTable>().idColumn()); });
        registerColumn(catalogue, type, "label", [](const auto& model)
                       { return exportTextColumn(model->template table<AeroControllerTable>().labelColumn()); });
        registerColumn(catalogue, type, "units", [](const auto& model)
                       { return exportTextColumn(model->template table<AeroControllerTable>().unitsColumn()); });
    }
    {
        using model::AeroPressureVectorTable;
        const std::uint16_t type = model::typeIndexOf<AeroPressureVectorTable>();
        registerColumn(
            catalogue, type, "controlVectorId", [](const auto& model)
            { return exportColumn(model->template table<AeroPressureVectorTable>().controlVectorIdColumn()); });
    }
    {
        using model::AeroQuad4Table;
        const std::uint16_t type = model::typeIndexOf<AeroQuad4Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroQuad4Table>().idColumn()); });
    }
    {
        using model::AeroUnsteadyParamsTable;
        const std::uint16_t type = model::typeIndexOf<AeroUnsteadyParamsTable>();
        registerColumn(catalogue, type, "aeroFrameId", [](const auto& model)
                       { return exportColumn(model->template table<AeroUnsteadyParamsTable>().aeroFrameIdColumn()); });
        registerColumn(catalogue, type, "velocity", [](const auto& model)
                       { return exportColumn(model->template table<AeroUnsteadyParamsTable>().velocityColumn()); });
        registerColumn(
            catalogue, type, "referenceChord", [](const auto& model)
            { return exportColumn(model->template table<AeroUnsteadyParamsTable>().referenceChordColumn()); });
        registerColumn(
            catalogue, type, "referenceDensity", [](const auto& model)
            { return exportColumn(model->template table<AeroUnsteadyParamsTable>().referenceDensityColumn()); });
        registerColumn(catalogue, type, "symmetricXz", [](const auto& model)
                       { return exportColumn(model->template table<AeroUnsteadyParamsTable>().symmetricXzColumn()); });
        registerColumn(catalogue, type, "symmetricXy", [](const auto& model)
                       { return exportColumn(model->template table<AeroUnsteadyParamsTable>().symmetricXyColumn()); });
    }
    {
        using model::AeroStaticParamsTable;
        const std::uint16_t type = model::typeIndexOf<AeroStaticParamsTable>();
        registerColumn(catalogue, type, "aeroFrameId", [](const auto& model)
                       { return exportColumn(model->template table<AeroStaticParamsTable>().aeroFrameIdColumn()); });
        registerColumn(
            catalogue, type, "referenceFrameId", [](const auto& model)
            { return exportColumn(model->template table<AeroStaticParamsTable>().referenceFrameIdColumn()); });
        registerColumn(catalogue, type, "referenceChord", [](const auto& model)
                       { return exportColumn(model->template table<AeroStaticParamsTable>().referenceChordColumn()); });
        registerColumn(catalogue, type, "referenceSpan", [](const auto& model)
                       { return exportColumn(model->template table<AeroStaticParamsTable>().referenceSpanColumn()); });
        registerColumn(catalogue, type, "referenceArea", [](const auto& model)
                       { return exportColumn(model->template table<AeroStaticParamsTable>().referenceAreaColumn()); });
        registerColumn(catalogue, type, "symmetricXz", [](const auto& model)
                       { return exportColumn(model->template table<AeroStaticParamsTable>().symmetricXzColumn()); });
        registerColumn(catalogue, type, "symmetricXy", [](const auto& model)
                       { return exportColumn(model->template table<AeroStaticParamsTable>().symmetricXyColumn()); });
    }
    {
        using model::AeroNodeScaleTable;
        const std::uint16_t type = model::typeIndexOf<AeroNodeScaleTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroNodeScaleTable>().idColumn()); });
    }
    {
        using model::AeroRigidTrimVariableTable;
        const std::uint16_t type = model::typeIndexOf<AeroRigidTrimVariableTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroRigidTrimVariableTable>().idColumn()); });
        registerColumn(catalogue, type, "label", [](const auto& model)
                       { return exportTextColumn(model->template table<AeroRigidTrimVariableTable>().labelColumn()); });
    }
    {
        using model::AeroControlSurfaceTable;
        const std::uint16_t type = model::typeIndexOf<AeroControlSurfaceTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroControlSurfaceTable>().idColumn()); });
        registerColumn(catalogue, type, "label", [](const auto& model)
                       { return exportTextColumn(model->template table<AeroControlSurfaceTable>().labelColumn()); });
        registerColumn(catalogue, type, "frameId1", [](const auto& model)
                       { return exportColumn(model->template table<AeroControlSurfaceTable>().frameId1Column()); });
        registerColumn(catalogue, type, "boxListId1", [](const auto& model)
                       { return exportColumn(model->template table<AeroControlSurfaceTable>().boxListId1Column()); });
        registerColumn(catalogue, type, "frameId2", [](const auto& model)
                       { return exportColumn(model->template table<AeroControlSurfaceTable>().frameId2Column()); });
        registerColumn(catalogue, type, "boxListId2", [](const auto& model)
                       { return exportColumn(model->template table<AeroControlSurfaceTable>().boxListId2Column()); });
        registerColumn(
            catalogue, type, "effectiveness", [](const auto& model)
            { return exportColumn(model->template table<AeroControlSurfaceTable>().effectivenessColumn()); });
        registerColumn(
            catalogue, type, "linearDownwash", [](const auto& model)
            { return exportColumn(model->template table<AeroControlSurfaceTable>().linearDownwashColumn()); });
        registerColumn(
            catalogue, type, "referenceChord", [](const auto& model)
            { return exportColumn(model->template table<AeroControlSurfaceTable>().referenceChordColumn()); });
        registerColumn(
            catalogue, type, "referenceArea", [](const auto& model)
            { return exportColumn(model->template table<AeroControlSurfaceTable>().referenceAreaColumn()); });
        registerColumn(
            catalogue, type, "deflectionLowerLimit", [](const auto& model)
            { return exportColumn(model->template table<AeroControlSurfaceTable>().deflectionLowerLimitColumn()); });
        registerColumn(
            catalogue, type, "deflectionUpperLimit", [](const auto& model)
            { return exportColumn(model->template table<AeroControlSurfaceTable>().deflectionUpperLimitColumn()); });
        registerColumn(
            catalogue, type, "hingeMomentLowerLimit", [](const auto& model)
            { return exportColumn(model->template table<AeroControlSurfaceTable>().hingeMomentLowerLimitColumn()); });
        registerColumn(
            catalogue, type, "hingeMomentUpperLimit", [](const auto& model)
            { return exportColumn(model->template table<AeroControlSurfaceTable>().hingeMomentUpperLimitColumn()); });
        registerColumn(
            catalogue, type, "lowerLimitTableId", [](const auto& model)
            { return exportColumn(model->template table<AeroControlSurfaceTable>().lowerLimitTableIdColumn()); });
        registerColumn(
            catalogue, type, "upperLimitTableId", [](const auto& model)
            { return exportColumn(model->template table<AeroControlSurfaceTable>().upperLimitTableIdColumn()); });
    }
    {
        using model::AeroControlSurfaceNodesTable;
        const std::uint16_t type = model::typeIndexOf<AeroControlSurfaceNodesTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroControlSurfaceNodesTable>().idColumn()); });
    }
    {
        using model::AeroTria3Table;
        const std::uint16_t type = model::typeIndexOf<AeroTria3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroTria3Table>().idColumn()); });
    }
    {
        using model::AeroLiftingPanel1Table;
        const std::uint16_t type = model::typeIndexOf<AeroLiftingPanel1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroLiftingPanel1Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<AeroLiftingPanel1Table>().propIdColumn()); });
        registerColumn(
            catalogue, type, "positionFrameId", [](const auto& model)
            { return exportColumn(model->template table<AeroLiftingPanel1Table>().positionFrameIdColumn()); });
        registerColumn(catalogue, type, "spanBoxCount", [](const auto& model)
                       { return exportColumn(model->template table<AeroLiftingPanel1Table>().spanBoxCountColumn()); });
        registerColumn(catalogue, type, "chordBoxCount", [](const auto& model)
                       { return exportColumn(model->template table<AeroLiftingPanel1Table>().chordBoxCountColumn()); });
        registerColumn(
            catalogue, type, "spanDivisionListId", [](const auto& model)
            { return exportColumn(model->template table<AeroLiftingPanel1Table>().spanDivisionListIdColumn()); });
        registerColumn(
            catalogue, type, "chordDivisionListId", [](const auto& model)
            { return exportColumn(model->template table<AeroLiftingPanel1Table>().chordDivisionListIdColumn()); });
        registerColumn(
            catalogue, type, "interferenceGroupId", [](const auto& model)
            { return exportColumn(model->template table<AeroLiftingPanel1Table>().interferenceGroupIdColumn()); });
        registerColumn(catalogue, type, "point1", [](const auto& model)
                       { return exportColumn(model->template table<AeroLiftingPanel1Table>().point1Column()); });
        registerColumn(catalogue, type, "rootChord", [](const auto& model)
                       { return exportColumn(model->template table<AeroLiftingPanel1Table>().rootChordColumn()); });
        registerColumn(catalogue, type, "point4", [](const auto& model)
                       { return exportColumn(model->template table<AeroLiftingPanel1Table>().point4Column()); });
        registerColumn(catalogue, type, "tipChord", [](const auto& model)
                       { return exportColumn(model->template table<AeroLiftingPanel1Table>().tipChordColumn()); });
    }
    {
        using model::AeroBody2Table;
        const std::uint16_t type = model::typeIndexOf<AeroBody2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroBody2Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<AeroBody2Table>().propIdColumn()); });
        registerColumn(catalogue, type, "positionFrameId", [](const auto& model)
                       { return exportColumn(model->template table<AeroBody2Table>().positionFrameIdColumn()); });
        registerColumn(catalogue, type, "slenderElemCount", [](const auto& model)
                       { return exportColumn(model->template table<AeroBody2Table>().slenderElemCountColumn()); });
        registerColumn(catalogue, type, "interferenceElemCount", [](const auto& model)
                       { return exportColumn(model->template table<AeroBody2Table>().interferenceElemCountColumn()); });
        registerColumn(catalogue, type, "slenderListId", [](const auto& model)
                       { return exportColumn(model->template table<AeroBody2Table>().slenderListIdColumn()); });
        registerColumn(catalogue, type, "interferenceListId", [](const auto& model)
                       { return exportColumn(model->template table<AeroBody2Table>().interferenceListIdColumn()); });
        registerColumn(catalogue, type, "interferenceGroupId", [](const auto& model)
                       { return exportColumn(model->template table<AeroBody2Table>().interferenceGroupIdColumn()); });
        registerColumn(catalogue, type, "point1", [](const auto& model)
                       { return exportColumn(model->template table<AeroBody2Table>().point1Column()); });
        registerColumn(catalogue, type, "length", [](const auto& model)
                       { return exportColumn(model->template table<AeroBody2Table>().lengthColumn()); });
    }
    {
        using model::AeroMachBoxPanel3Table;
        const std::uint16_t type = model::typeIndexOf<AeroMachBoxPanel3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroMachBoxPanel3Table>().idColumn()); });
    }
    {
        using model::AeroStripPanel4Table;
        const std::uint16_t type = model::typeIndexOf<AeroStripPanel4Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroStripPanel4Table>().idColumn()); });
    }
    {
        using model::AeroPistonPanel5Table;
        const std::uint16_t type = model::typeIndexOf<AeroPistonPanel5Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroPistonPanel5Table>().idColumn()); });
    }
    {
        using model::AeroControlScheduleTable;
        const std::uint16_t type = model::typeIndexOf<AeroControlScheduleTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroControlScheduleTable>().idColumn()); });
    }
    {
        using model::AeroDivergenceTable;
        const std::uint16_t type = model::typeIndexOf<AeroDivergenceTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroDivergenceTable>().idColumn()); });
    }
    {
        using model::AeroFlutterFactorsTable;
        const std::uint16_t type = model::typeIndexOf<AeroFlutterFactorsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroFlutterFactorsTable>().idColumn()); });
        registerColumn(
            catalogue, type, "factors", [](const auto& model)
            { return exportVariableColumn(model->template table<AeroFlutterFactorsTable>().factorsColumn()); });
        registerColumn(catalogue, type, "isRange", [](const auto& model)
                       { return exportColumn(model->template table<AeroFlutterFactorsTable>().isRangeColumn()); });
        registerColumn(catalogue, type, "factor1", [](const auto& model)
                       { return exportColumn(model->template table<AeroFlutterFactorsTable>().factor1Column()); });
        registerColumn(catalogue, type, "factorLast", [](const auto& model)
                       { return exportColumn(model->template table<AeroFlutterFactorsTable>().factorLastColumn()); });
        registerColumn(catalogue, type, "factorCount", [](const auto& model)
                       { return exportColumn(model->template table<AeroFlutterFactorsTable>().factorCountColumn()); });
        registerColumn(catalogue, type, "factorMid", [](const auto& model)
                       { return exportColumn(model->template table<AeroFlutterFactorsTable>().factorMidColumn()); });
    }
    {
        using model::AeroFlutterTable;
        const std::uint16_t type = model::typeIndexOf<AeroFlutterTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroFlutterTable>().idColumn()); });
        registerColumn(catalogue, type, "method", [](const auto& model)
                       { return exportColumn(model->template table<AeroFlutterTable>().methodColumn()); });
        registerColumn(catalogue, type, "densityFactorsId", [](const auto& model)
                       { return exportColumn(model->template table<AeroFlutterTable>().densityFactorsIdColumn()); });
        registerColumn(catalogue, type, "machFactorsId", [](const auto& model)
                       { return exportColumn(model->template table<AeroFlutterTable>().machFactorsIdColumn()); });
        registerColumn(
            catalogue, type, "reducedFreqFactorsId", [](const auto& model)
            { return exportColumn(model->template table<AeroFlutterTable>().reducedFreqFactorsIdColumn()); });
        registerColumn(catalogue, type, "interpolationMethod", [](const auto& model)
                       { return exportColumn(model->template table<AeroFlutterTable>().interpolationMethodColumn()); });
        registerColumn(catalogue, type, "eigenvalueCount", [](const auto& model)
                       { return exportColumn(model->template table<AeroFlutterTable>().eigenvalueCountColumn()); });
        registerColumn(catalogue, type, "maxFrequency", [](const auto& model)
                       { return exportColumn(model->template table<AeroFlutterTable>().maxFrequencyColumn()); });
        registerColumn(catalogue, type, "epsilon", [](const auto& model)
                       { return exportColumn(model->template table<AeroFlutterTable>().epsilonColumn()); });
    }
    {
        using model::AeroGustTable;
        const std::uint16_t type = model::typeIndexOf<AeroGustTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroGustTable>().idColumn()); });
        registerColumn(catalogue, type, "dynamicLoadId", [](const auto& model)
                       { return exportColumn(model->template table<AeroGustTable>().dynamicLoadIdColumn()); });
        registerColumn(catalogue, type, "gustScale", [](const auto& model)
                       { return exportColumn(model->template table<AeroGustTable>().gustScaleColumn()); });
        registerColumn(catalogue, type, "gustOriginX", [](const auto& model)
                       { return exportColumn(model->template table<AeroGustTable>().gustOriginXColumn()); });
        registerColumn(catalogue, type, "velocity", [](const auto& model)
                       { return exportColumn(model->template table<AeroGustTable>().velocityColumn()); });
    }
    {
        using model::AeroMachFreqTable1Table;
        const std::uint16_t type = model::typeIndexOf<AeroMachFreqTable1Table>();
        registerColumn(
            catalogue, type, "machs", [](const auto& model)
            { return exportVariableColumn(model->template table<AeroMachFreqTable1Table>().machsColumn()); });
        registerColumn(
            catalogue, type, "frequencies", [](const auto& model)
            { return exportVariableColumn(model->template table<AeroMachFreqTable1Table>().frequenciesColumn()); });
    }
    {
        using model::AeroMachFreqPairs2Table;
        const std::uint16_t type = model::typeIndexOf<AeroMachFreqPairs2Table>();
        registerColumn(
            catalogue, type, "pairs", [](const auto& model)
            { return exportVariableColumn(model->template table<AeroMachFreqPairs2Table>().pairsColumn()); });
    }
    {
        using model::AeroLiftingPanelProp1Table;
        const std::uint16_t type = model::typeIndexOf<AeroLiftingPanelProp1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroLiftingPanelProp1Table>().idColumn()); });
        registerColumn(catalogue, type, "bodies", [](const auto& model)
                       { return exportColumn(model->template table<AeroLiftingPanelProp1Table>().bodiesColumn()); });
    }
    {
        using model::AeroBodyProp2Table;
        const std::uint16_t type = model::typeIndexOf<AeroBodyProp2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroBodyProp2Table>().idColumn()); });
        registerColumn(catalogue, type, "orientation", [](const auto& model)
                       { return exportColumn(model->template table<AeroBodyProp2Table>().orientationColumn()); });
        registerColumn(catalogue, type, "width", [](const auto& model)
                       { return exportColumn(model->template table<AeroBodyProp2Table>().widthColumn()); });
        registerColumn(catalogue, type, "aspectRatio", [](const auto& model)
                       { return exportColumn(model->template table<AeroBodyProp2Table>().aspectRatioColumn()); });
        registerColumn(
            catalogue, type, "slenderHalfWidthListId", [](const auto& model)
            { return exportColumn(model->template table<AeroBodyProp2Table>().slenderHalfWidthListIdColumn()); });
        registerColumn(
            catalogue, type, "interferenceHalfWidthListId", [](const auto& model)
            { return exportColumn(model->template table<AeroBodyProp2Table>().interferenceHalfWidthListIdColumn()); });
        registerColumn(catalogue, type, "angleListId1", [](const auto& model)
                       { return exportColumn(model->template table<AeroBodyProp2Table>().angleListId1Column()); });
        registerColumn(catalogue, type, "angleListId2", [](const auto& model)
                       { return exportColumn(model->template table<AeroBodyProp2Table>().angleListId2Column()); });
        registerColumn(
            catalogue, type, "angleList1FirstElem", [](const auto& model)
            { return exportColumn(model->template table<AeroBodyProp2Table>().angleList1FirstElemColumn()); });
        registerColumn(
            catalogue, type, "angleList1LastElem", [](const auto& model)
            { return exportColumn(model->template table<AeroBodyProp2Table>().angleList1LastElemColumn()); });
        registerColumn(
            catalogue, type, "angleList2FirstElem", [](const auto& model)
            { return exportColumn(model->template table<AeroBodyProp2Table>().angleList2FirstElemColumn()); });
        registerColumn(
            catalogue, type, "angleList2LastElem", [](const auto& model)
            { return exportColumn(model->template table<AeroBodyProp2Table>().angleList2LastElemColumn()); });
    }
    {
        using model::AeroMachBoxProp3Table;
        const std::uint16_t type = model::typeIndexOf<AeroMachBoxProp3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroMachBoxProp3Table>().idColumn()); });
    }
    {
        using model::AeroStripProp4Table;
        const std::uint16_t type = model::typeIndexOf<AeroStripProp4Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroStripProp4Table>().idColumn()); });
    }
    {
        using model::AeroPistonProp5Table;
        const std::uint16_t type = model::typeIndexOf<AeroPistonProp5Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroPistonProp5Table>().idColumn()); });
    }
    {
        using model::AeroSplineStripBlendTable;
        const std::uint16_t type = model::typeIndexOf<AeroSplineStripBlendTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroSplineStripBlendTable>().idColumn()); });
    }
    {
        using model::AeroSplineCurveBlendTable;
        const std::uint16_t type = model::typeIndexOf<AeroSplineCurveBlendTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroSplineCurveBlendTable>().idColumn()); });
    }
    {
        using model::AeroSurfaceSpline1Table;
        const std::uint16_t type = model::typeIndexOf<AeroSurfaceSpline1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroSurfaceSpline1Table>().idColumn()); });
        registerColumn(catalogue, type, "aeroPanelId", [](const auto& model)
                       { return exportColumn(model->template table<AeroSurfaceSpline1Table>().aeroPanelIdColumn()); });
        registerColumn(catalogue, type, "firstBox", [](const auto& model)
                       { return exportColumn(model->template table<AeroSurfaceSpline1Table>().firstBoxColumn()); });
        registerColumn(catalogue, type, "lastBox", [](const auto& model)
                       { return exportColumn(model->template table<AeroSurfaceSpline1Table>().lastBoxColumn()); });
        registerColumn(catalogue, type, "nodeSetId", [](const auto& model)
                       { return exportColumn(model->template table<AeroSurfaceSpline1Table>().nodeSetIdColumn()); });
        registerColumn(
            catalogue, type, "linearFlexibility", [](const auto& model)
            { return exportColumn(model->template table<AeroSurfaceSpline1Table>().linearFlexibilityColumn()); });
        registerColumn(catalogue, type, "method", [](const auto& model)
                       { return exportColumn(model->template table<AeroSurfaceSpline1Table>().methodColumn()); });
        registerColumn(catalogue, type, "usage", [](const auto& model)
                       { return exportColumn(model->template table<AeroSurfaceSpline1Table>().usageColumn()); });
        registerColumn(catalogue, type, "elemCountX", [](const auto& model)
                       { return exportColumn(model->template table<AeroSurfaceSpline1Table>().elemCountXColumn()); });
        registerColumn(catalogue, type, "elemCountY", [](const auto& model)
                       { return exportColumn(model->template table<AeroSurfaceSpline1Table>().elemCountYColumn()); });
    }
    {
        using model::AeroBeamSpline2Table;
        const std::uint16_t type = model::typeIndexOf<AeroBeamSpline2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroBeamSpline2Table>().idColumn()); });
        registerColumn(catalogue, type, "aeroPanelId", [](const auto& model)
                       { return exportColumn(model->template table<AeroBeamSpline2Table>().aeroPanelIdColumn()); });
        registerColumn(catalogue, type, "firstBox", [](const auto& model)
                       { return exportColumn(model->template table<AeroBeamSpline2Table>().firstBoxColumn()); });
        registerColumn(catalogue, type, "lastBox", [](const auto& model)
                       { return exportColumn(model->template table<AeroBeamSpline2Table>().lastBoxColumn()); });
        registerColumn(catalogue, type, "nodeSetId", [](const auto& model)
                       { return exportColumn(model->template table<AeroBeamSpline2Table>().nodeSetIdColumn()); });
        registerColumn(
            catalogue, type, "linearFlexibility", [](const auto& model)
            { return exportColumn(model->template table<AeroBeamSpline2Table>().linearFlexibilityColumn()); });
        registerColumn(
            catalogue, type, "bendingTorsionRatio", [](const auto& model)
            { return exportColumn(model->template table<AeroBeamSpline2Table>().bendingTorsionRatioColumn()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<AeroBeamSpline2Table>().frameIdColumn()); });
        registerColumn(
            catalogue, type, "rotationFlexibilityX", [](const auto& model)
            { return exportColumn(model->template table<AeroBeamSpline2Table>().rotationFlexibilityXColumn()); });
        registerColumn(
            catalogue, type, "rotationFlexibilityY", [](const auto& model)
            { return exportColumn(model->template table<AeroBeamSpline2Table>().rotationFlexibilityYColumn()); });
        registerColumn(catalogue, type, "usage", [](const auto& model)
                       { return exportColumn(model->template table<AeroBeamSpline2Table>().usageColumn()); });
    }
    {
        using model::AeroSplineEquation3Table;
        const std::uint16_t type = model::typeIndexOf<AeroSplineEquation3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroSplineEquation3Table>().idColumn()); });
    }
    {
        using model::AeroSurfaceSpline4Table;
        const std::uint16_t type = model::typeIndexOf<AeroSurfaceSpline4Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroSurfaceSpline4Table>().idColumn()); });
        registerColumn(catalogue, type, "aeroPanelId", [](const auto& model)
                       { return exportColumn(model->template table<AeroSurfaceSpline4Table>().aeroPanelIdColumn()); });
        registerColumn(
            catalogue, type, "aeroBoxListId", [](const auto& model)
            { return exportColumn(model->template table<AeroSurfaceSpline4Table>().aeroBoxListIdColumn()); });
        registerColumn(catalogue, type, "nodeSetId", [](const auto& model)
                       { return exportColumn(model->template table<AeroSurfaceSpline4Table>().nodeSetIdColumn()); });
        registerColumn(
            catalogue, type, "linearFlexibility", [](const auto& model)
            { return exportColumn(model->template table<AeroSurfaceSpline4Table>().linearFlexibilityColumn()); });
        registerColumn(catalogue, type, "method", [](const auto& model)
                       { return exportColumn(model->template table<AeroSurfaceSpline4Table>().methodColumn()); });
        registerColumn(catalogue, type, "usage", [](const auto& model)
                       { return exportColumn(model->template table<AeroSurfaceSpline4Table>().usageColumn()); });
        registerColumn(catalogue, type, "elemCountX", [](const auto& model)
                       { return exportColumn(model->template table<AeroSurfaceSpline4Table>().elemCountXColumn()); });
        registerColumn(catalogue, type, "elemCountY", [](const auto& model)
                       { return exportColumn(model->template table<AeroSurfaceSpline4Table>().elemCountYColumn()); });
        registerColumn(catalogue, type, "functionKind", [](const auto& model)
                       { return exportColumn(model->template table<AeroSurfaceSpline4Table>().functionKindColumn()); });
        registerColumn(catalogue, type, "coreRadius", [](const auto& model)
                       { return exportColumn(model->template table<AeroSurfaceSpline4Table>().coreRadiusColumn()); });
    }
    {
        using model::AeroBeamSpline5Table;
        const std::uint16_t type = model::typeIndexOf<AeroBeamSpline5Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroBeamSpline5Table>().idColumn()); });
        registerColumn(catalogue, type, "aeroPanelId", [](const auto& model)
                       { return exportColumn(model->template table<AeroBeamSpline5Table>().aeroPanelIdColumn()); });
        registerColumn(catalogue, type, "aeroBoxListId", [](const auto& model)
                       { return exportColumn(model->template table<AeroBeamSpline5Table>().aeroBoxListIdColumn()); });
        registerColumn(catalogue, type, "nodeSetId", [](const auto& model)
                       { return exportColumn(model->template table<AeroBeamSpline5Table>().nodeSetIdColumn()); });
        registerColumn(
            catalogue, type, "linearFlexibility", [](const auto& model)
            { return exportColumn(model->template table<AeroBeamSpline5Table>().linearFlexibilityColumn()); });
        registerColumn(
            catalogue, type, "bendingTorsionRatio", [](const auto& model)
            { return exportColumn(model->template table<AeroBeamSpline5Table>().bendingTorsionRatioColumn()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<AeroBeamSpline5Table>().frameIdColumn()); });
        registerColumn(
            catalogue, type, "rotationFlexibilityX", [](const auto& model)
            { return exportColumn(model->template table<AeroBeamSpline5Table>().rotationFlexibilityXColumn()); });
        registerColumn(
            catalogue, type, "rotationFlexibilityY", [](const auto& model)
            { return exportColumn(model->template table<AeroBeamSpline5Table>().rotationFlexibilityYColumn()); });
        registerColumn(catalogue, type, "usage", [](const auto& model)
                       { return exportColumn(model->template table<AeroBeamSpline5Table>().usageColumn()); });
        registerColumn(catalogue, type, "method", [](const auto& model)
                       { return exportColumn(model->template table<AeroBeamSpline5Table>().methodColumn()); });
        registerColumn(catalogue, type, "functionKind", [](const auto& model)
                       { return exportColumn(model->template table<AeroBeamSpline5Table>().functionKindColumn()); });
        registerColumn(catalogue, type, "coreRadius", [](const auto& model)
                       { return exportColumn(model->template table<AeroBeamSpline5Table>().coreRadiusColumn()); });
    }
    {
        using model::AeroFiniteSurfaceSpline6Table;
        const std::uint16_t type = model::typeIndexOf<AeroFiniteSurfaceSpline6Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroFiniteSurfaceSpline6Table>().idColumn()); });
    }
    {
        using model::AeroFiniteBeamSpline7Table;
        const std::uint16_t type = model::typeIndexOf<AeroFiniteBeamSpline7Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroFiniteBeamSpline7Table>().idColumn()); });
    }
    {
        using model::AeroUserSplineTable;
        const std::uint16_t type = model::typeIndexOf<AeroUserSplineTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroUserSplineTable>().idColumn()); });
    }
    {
        using model::AeroRigidSplineTable;
        const std::uint16_t type = model::typeIndexOf<AeroRigidSplineTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroRigidSplineTable>().idColumn()); });
    }
    {
        using model::AeroSplineRelaxationTable;
        const std::uint16_t type = model::typeIndexOf<AeroSplineRelaxationTable>();
        registerColumn(catalogue, type, "splineId1", [](const auto& model)
                       { return exportColumn(model->template table<AeroSplineRelaxationTable>().splineId1Column()); });
    }
    {
        using model::AeroTrimTable;
        const std::uint16_t type = model::typeIndexOf<AeroTrimTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroTrimTable>().idColumn()); });
    }
    {
        using model::AeroTrim2Table;
        const std::uint16_t type = model::typeIndexOf<AeroTrim2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroTrim2Table>().idColumn()); });
    }
    {
        using model::AeroControlVectorTable;
        const std::uint16_t type = model::typeIndexOf<AeroControlVectorTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroControlVectorTable>().idColumn()); });
    }
}

} // namespace oor::query::column_export
