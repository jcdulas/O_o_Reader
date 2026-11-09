// The stored columns of the cards of the family "sets", for the query engine (ColumnExport.hpp).
// One block a card type: its type index, then a getter a column.

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/sets/aeroPrismNodeSet2/AeroPrismNodeSet2Table.hpp"
#include "oor/model/cards/sets/analysisDofSet/AnalysisDofSetTable.hpp"
#include "oor/model/cards/sets/analysisDofSet1/AnalysisDofSet1Table.hpp"
#include "oor/model/cards/sets/axisymOmittedDofSet/AxisymOmittedDofSetTable.hpp"
#include "oor/model/cards/sets/dofSetRenames/DofSetRenamesTable.hpp"
#include "oor/model/cards/sets/fixedBoundaryDofSet/FixedBoundaryDofSetTable.hpp"
#include "oor/model/cards/sets/fixedBoundaryDofSet1/FixedBoundaryDofSet1Table.hpp"
#include "oor/model/cards/sets/freeBoundaryDofSet/FreeBoundaryDofSetTable.hpp"
#include "oor/model/cards/sets/freeBoundaryDofSet1/FreeBoundaryDofSet1Table.hpp"
#include "oor/model/cards/sets/generalizedDofSet/GeneralizedDofSetTable.hpp"
#include "oor/model/cards/sets/generalizedDofSet1/GeneralizedDofSet1Table.hpp"
#include "oor/model/cards/sets/idListSet1/IdListSet1Table.hpp"
#include "oor/model/cards/sets/omittedDofSet/OmittedDofSetTable.hpp"
#include "oor/model/cards/sets/omittedDofSet1/OmittedDofSet1Table.hpp"
#include "oor/model/cards/sets/propertyListSet4/PropertyListSet4Table.hpp"
#include "oor/model/cards/sets/radiatedPowerPanelSet/RadiatedPowerPanelSetTable.hpp"
#include "oor/model/cards/sets/shapeBoundaryNodeSet/ShapeBoundaryNodeSetTable.hpp"
#include "oor/model/cards/sets/typedListSet3/TypedListSet3Table.hpp"
#include "oor/model/cards/sets/userDofSet/UserDofSetTable.hpp"
#include "oor/model/cards/sets/userDofSet1/UserDofSet1Table.hpp"
#include "oor/query/providers/ColumnExport.hpp"

namespace oor::query::column_export
{

void registerSetsColumns(StoredColumnCatalogue& catalogue)
{
    {
        using model::AnalysisDofSetTable;
        const std::uint16_t type = model::typeIndexOf<AnalysisDofSetTable>();
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportVariableColumn(model->template table<AnalysisDofSetTable>().nodeColumn()); });
        registerColumn(
            catalogue, type, "component", [](const auto& model)
            { return exportVariableColumn(model->template table<AnalysisDofSetTable>().componentColumn()); });
    }
    {
        using model::AnalysisDofSet1Table;
        const std::uint16_t type = model::typeIndexOf<AnalysisDofSet1Table>();
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<AnalysisDofSet1Table>().componentsColumn()); });
        registerColumn(catalogue, type, "runs", [](const auto& model)
                       { return exportVariableColumn(model->template table<AnalysisDofSet1Table>().runsColumn()); });
    }
    {
        using model::ShapeBoundaryNodeSetTable;
        const std::uint16_t type = model::typeIndexOf<ShapeBoundaryNodeSetTable>();
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<ShapeBoundaryNodeSetTable>().componentsColumn()); });
        registerColumn(
            catalogue, type, "runs", [](const auto& model)
            { return exportVariableColumn(model->template table<ShapeBoundaryNodeSetTable>().runsColumn()); });
    }
    {
        using model::FixedBoundaryDofSetTable;
        const std::uint16_t type = model::typeIndexOf<FixedBoundaryDofSetTable>();
        registerColumn(
            catalogue, type, "node", [](const auto& model)
            { return exportVariableColumn(model->template table<FixedBoundaryDofSetTable>().nodeColumn()); });
        registerColumn(
            catalogue, type, "component", [](const auto& model)
            { return exportVariableColumn(model->template table<FixedBoundaryDofSetTable>().componentColumn()); });
    }
    {
        using model::FixedBoundaryDofSet1Table;
        const std::uint16_t type = model::typeIndexOf<FixedBoundaryDofSet1Table>();
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<FixedBoundaryDofSet1Table>().componentsColumn()); });
        registerColumn(
            catalogue, type, "runs", [](const auto& model)
            { return exportVariableColumn(model->template table<FixedBoundaryDofSet1Table>().runsColumn()); });
    }
    {
        using model::FreeBoundaryDofSetTable;
        const std::uint16_t type = model::typeIndexOf<FreeBoundaryDofSetTable>();
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportVariableColumn(model->template table<FreeBoundaryDofSetTable>().nodeColumn()); });
        registerColumn(
            catalogue, type, "component", [](const auto& model)
            { return exportVariableColumn(model->template table<FreeBoundaryDofSetTable>().componentColumn()); });
    }
    {
        using model::FreeBoundaryDofSet1Table;
        const std::uint16_t type = model::typeIndexOf<FreeBoundaryDofSet1Table>();
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<FreeBoundaryDofSet1Table>().componentsColumn()); });
        registerColumn(
            catalogue, type, "runs", [](const auto& model)
            { return exportVariableColumn(model->template table<FreeBoundaryDofSet1Table>().runsColumn()); });
    }
    {
        using model::DofSetRenamesTable;
        const std::uint16_t type = model::typeIndexOf<DofSetRenamesTable>();
        registerColumn(catalogue, type, "renameCount", [](const auto& model)
                       { return exportColumn(model->template table<DofSetRenamesTable>().renameCountColumn()); });
    }
    {
        using model::RadiatedPowerPanelSetTable;
        const std::uint16_t type = model::typeIndexOf<RadiatedPowerPanelSetTable>();
        registerColumn(
            catalogue, type, "panelCount", [](const auto& model)
            { return exportColumn(model->template table<RadiatedPowerPanelSetTable>().panelCountColumn()); });
    }
    {
        using model::OmittedDofSetTable;
        const std::uint16_t type = model::typeIndexOf<OmittedDofSetTable>();
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportVariableColumn(model->template table<OmittedDofSetTable>().nodeColumn()); });
        registerColumn(catalogue, type, "component", [](const auto& model)
                       { return exportVariableColumn(model->template table<OmittedDofSetTable>().componentColumn()); });
    }
    {
        using model::OmittedDofSet1Table;
        const std::uint16_t type = model::typeIndexOf<OmittedDofSet1Table>();
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<OmittedDofSet1Table>().componentsColumn()); });
        registerColumn(catalogue, type, "runs", [](const auto& model)
                       { return exportVariableColumn(model->template table<OmittedDofSet1Table>().runsColumn()); });
    }
    {
        using model::AxisymOmittedDofSetTable;
        const std::uint16_t type = model::typeIndexOf<AxisymOmittedDofSetTable>();
        registerColumn(catalogue, type, "count", [](const auto& model)
                       { return exportColumn(model->template table<AxisymOmittedDofSetTable>().countColumn()); });
    }
    {
        using model::GeneralizedDofSetTable;
        const std::uint16_t type = model::typeIndexOf<GeneralizedDofSetTable>();
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportVariableColumn(model->template table<GeneralizedDofSetTable>().nodeColumn()); });
        registerColumn(
            catalogue, type, "component", [](const auto& model)
            { return exportVariableColumn(model->template table<GeneralizedDofSetTable>().componentColumn()); });
    }
    {
        using model::GeneralizedDofSet1Table;
        const std::uint16_t type = model::typeIndexOf<GeneralizedDofSet1Table>();
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<GeneralizedDofSet1Table>().componentsColumn()); });
        registerColumn(catalogue, type, "runs", [](const auto& model)
                       { return exportVariableColumn(model->template table<GeneralizedDofSet1Table>().runsColumn()); });
    }
    {
        using model::IdListSet1Table;
        const std::uint16_t type = model::typeIndexOf<IdListSet1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<IdListSet1Table>().idColumn()); });
        registerColumn(catalogue, type, "skin", [](const auto& model)
                       { return exportColumn(model->template table<IdListSet1Table>().skinColumn()); });
        registerColumn(catalogue, type, "runs", [](const auto& model)
                       { return exportVariableColumn(model->template table<IdListSet1Table>().runsColumn()); });
    }
    {
        using model::AeroPrismNodeSet2Table;
        const std::uint16_t type = model::typeIndexOf<AeroPrismNodeSet2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AeroPrismNodeSet2Table>().idColumn()); });
        registerColumn(catalogue, type, "aeroPanelId", [](const auto& model)
                       { return exportColumn(model->template table<AeroPrismNodeSet2Table>().aeroPanelIdColumn()); });
        registerColumn(catalogue, type, "spanStart", [](const auto& model)
                       { return exportColumn(model->template table<AeroPrismNodeSet2Table>().spanStartColumn()); });
        registerColumn(catalogue, type, "spanEnd", [](const auto& model)
                       { return exportColumn(model->template table<AeroPrismNodeSet2Table>().spanEndColumn()); });
        registerColumn(catalogue, type, "chordStart", [](const auto& model)
                       { return exportColumn(model->template table<AeroPrismNodeSet2Table>().chordStartColumn()); });
        registerColumn(catalogue, type, "chordEnd", [](const auto& model)
                       { return exportColumn(model->template table<AeroPrismNodeSet2Table>().chordEndColumn()); });
        registerColumn(catalogue, type, "zMaximum", [](const auto& model)
                       { return exportColumn(model->template table<AeroPrismNodeSet2Table>().zMaximumColumn()); });
        registerColumn(catalogue, type, "zMinimum", [](const auto& model)
                       { return exportColumn(model->template table<AeroPrismNodeSet2Table>().zMinimumColumn()); });
    }
    {
        using model::TypedListSet3Table;
        const std::uint16_t type = model::typeIndexOf<TypedListSet3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TypedListSet3Table>().idColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportColumn(model->template table<TypedListSet3Table>().kindColumn()); });
        registerColumn(catalogue, type, "runs", [](const auto& model)
                       { return exportVariableColumn(model->template table<TypedListSet3Table>().runsColumn()); });
    }
    {
        using model::PropertyListSet4Table;
        const std::uint16_t type = model::typeIndexOf<PropertyListSet4Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PropertyListSet4Table>().idColumn()); });
    }
    {
        using model::UserDofSetTable;
        const std::uint16_t type = model::typeIndexOf<UserDofSetTable>();
        registerColumn(catalogue, type, "setName", [](const auto& model)
                       { return exportTextColumn(model->template table<UserDofSetTable>().setNameColumn()); });
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportVariableColumn(model->template table<UserDofSetTable>().nodeColumn()); });
        registerColumn(catalogue, type, "component", [](const auto& model)
                       { return exportVariableColumn(model->template table<UserDofSetTable>().componentColumn()); });
    }
    {
        using model::UserDofSet1Table;
        const std::uint16_t type = model::typeIndexOf<UserDofSet1Table>();
        registerColumn(catalogue, type, "setName", [](const auto& model)
                       { return exportTextColumn(model->template table<UserDofSet1Table>().setNameColumn()); });
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<UserDofSet1Table>().componentsColumn()); });
        registerColumn(catalogue, type, "runs", [](const auto& model)
                       { return exportVariableColumn(model->template table<UserDofSet1Table>().runsColumn()); });
    }
}

} // namespace oor::query::column_export
