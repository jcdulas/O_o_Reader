// The stored columns of the cards of the family "matrices", for the query engine (ColumnExport.hpp).
// One block a card type: its type index, then a getter a column.

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/matrices/MatrixCards.hpp"
#include "oor/model/cards/matrices/directMatrix/DirectMatrixTable.hpp"
#include "oor/model/cards/matrices/directMatrixAeroJ/DirectMatrixAeroJTable.hpp"
#include "oor/model/cards/matrices/directMatrixAeroJInterference/DirectMatrixAeroJInterferenceTable.hpp"
#include "oor/model/cards/matrices/directMatrixAeroK/DirectMatrixAeroKTable.hpp"
#include "oor/model/cards/matrices/directMatrixAxisym/DirectMatrixAxisymTable.hpp"
#include "oor/model/cards/matrices/directMatrixNodal/DirectMatrixNodalTable.hpp"
#include "oor/model/cards/matrices/directTable/DirectTableTable.hpp"
#include "oor/model/cards/matrices/monitorChordNormalCoeffs/MonitorChordNormalCoeffsTable.hpp"
#include "oor/model/cards/matrices/monitorDisplacement/MonitorDisplacementTable.hpp"
#include "oor/model/cards/matrices/monitorElemLoad2/MonitorElemLoad2Table.hpp"
#include "oor/model/cards/matrices/monitorGroup/MonitorGroupTable.hpp"
#include "oor/model/cards/matrices/monitorIntegratedLoad1/MonitorIntegratedLoad1Table.hpp"
#include "oor/model/cards/matrices/monitorNodeForceSum3/MonitorNodeForceSum3Table.hpp"
#include "oor/model/cards/matrices/monitorSum/MonitorSumTable.hpp"
#include "oor/model/cards/matrices/monitorSumLocated/MonitorSumLocatedTable.hpp"
#include "oor/model/cards/matrices/monitorSumTotal/MonitorSumTotalTable.hpp"
#include "oor/model/cards/matrices/monteCarloParams/MonteCarloParamsTable.hpp"
#include "oor/query/providers/ColumnExport.hpp"
#include "oor/results/core/MatrixResults.hpp"

namespace oor::query::column_export
{

void registerMatricesColumns(StoredColumnCatalogue& catalogue)
{
    {
        using model::DirectMatrixTable;
        const std::uint16_t type = model::typeIndexOf<DirectMatrixTable>();
        registerColumn(catalogue, type, "entryName", [](const auto& model)
                       { return exportTextColumn(model->template table<DirectMatrixTable>().entryNameColumn()); });
        registerColumn(catalogue, type, "columnIndex", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixTable>().columnIndexColumn()); });
        registerColumn(catalogue, type, "matrixForm", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixTable>().matrixFormColumn()); });
        registerColumn(catalogue, type, "inputType", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixTable>().inputTypeColumn()); });
        registerColumn(catalogue, type, "outputType", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixTable>().outputTypeColumn()); });
        registerColumn(catalogue, type, "rowCount", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixTable>().rowsColumn()); });
        registerColumn(catalogue, type, "columnCount", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixTable>().columnsColumn()); });
        registerColumn(catalogue, type, "firstRow", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixTable>().firstRowColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportVariableColumn(model->template table<DirectMatrixTable>().kindColumn()); });
        registerColumn(catalogue, type, "value", [](const auto& model)
                       { return exportVariableColumn(model->template table<DirectMatrixTable>().valueColumn()); });
        registerColumn(catalogue, type, "row", [](const auto& model)
                       { return exportVariableColumn(model->template table<DirectMatrixTable>().rowColumn()); });
    }
    {
        using model::DirectMatrixAxisymTable;
        const std::uint16_t type = model::typeIndexOf<DirectMatrixAxisymTable>();
        registerColumn(
            catalogue, type, "entryName", [](const auto& model)
            { return exportTextColumn(model->template table<DirectMatrixAxisymTable>().entryNameColumn()); });
        registerColumn(catalogue, type, "nodeJ", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAxisymTable>().nodeJColumn()); });
        registerColumn(catalogue, type, "componentsJ", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAxisymTable>().componentsJColumn()); });
        registerColumn(catalogue, type, "harmonicJ", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAxisymTable>().harmonicJColumn()); });
        registerColumn(catalogue, type, "matrixShape", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAxisymTable>().matrixShapeColumn()); });
        registerColumn(catalogue, type, "inputType", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAxisymTable>().inputTypeColumn()); });
        registerColumn(catalogue, type, "outputType", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAxisymTable>().outputTypeColumn()); });
        registerColumn(catalogue, type, "imaginary", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAxisymTable>().imaginaryColumn()); });
        registerColumn(
            catalogue, type, "rows", [](const auto& model)
            { return exportVariableColumn(model->template table<DirectMatrixAxisymTable>().pointColumn()); });
        registerColumn(
            catalogue, type, "rowComponents", [](const auto& model)
            { return exportVariableColumn(model->template table<DirectMatrixAxisymTable>().componentColumn()); });
        registerColumn(
            catalogue, type, "rowHarmonics", [](const auto& model)
            { return exportVariableColumn(model->template table<DirectMatrixAxisymTable>().harmonicColumn()); });
        registerColumn(catalogue, type, "real", [](const auto& model)
                       { return exportVariableColumn(model->template table<DirectMatrixAxisymTable>().realColumn()); });
        registerColumn(
            catalogue, type, "imaginary", [](const auto& model)
            { return exportVariableColumn(model->template table<DirectMatrixAxisymTable>().imaginaryPartColumn()); });
    }
    {
        using model::DirectMatrixNodalTable;
        const std::uint16_t type = model::typeIndexOf<DirectMatrixNodalTable>();
        registerColumn(catalogue, type, "entryName", [](const auto& model)
                       { return exportTextColumn(model->template table<DirectMatrixNodalTable>().entryNameColumn()); });
        registerColumn(catalogue, type, "nodeJ", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixNodalTable>().nodeJColumn()); });
        registerColumn(catalogue, type, "componentsJ", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixNodalTable>().componentsJColumn()); });
        registerColumn(catalogue, type, "matrixShape", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixNodalTable>().matrixShapeColumn()); });
        registerColumn(catalogue, type, "inputType", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixNodalTable>().inputTypeColumn()); });
        registerColumn(catalogue, type, "outputType", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixNodalTable>().outputTypeColumn()); });
        registerColumn(catalogue, type, "polarFlag", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixNodalTable>().polarFlagColumn()); });
        registerColumn(catalogue, type, "columnCount", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixNodalTable>().columnCountColumn()); });
        registerColumn(catalogue, type, "imaginary", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixNodalTable>().imaginaryColumn()); });
        registerColumn(catalogue, type, "rows", [](const auto& model)
                       { return exportVariableColumn(model->template table<DirectMatrixNodalTable>().pointColumn()); });
        registerColumn(
            catalogue, type, "rowComponents", [](const auto& model)
            { return exportVariableColumn(model->template table<DirectMatrixNodalTable>().componentColumn()); });
        registerColumn(catalogue, type, "real", [](const auto& model)
                       { return exportVariableColumn(model->template table<DirectMatrixNodalTable>().realColumn()); });
        registerColumn(
            catalogue, type, "imaginary", [](const auto& model)
            { return exportVariableColumn(model->template table<DirectMatrixNodalTable>().imaginaryPartColumn()); });
    }
    {
        using model::DirectMatrixAeroJTable;
        const std::uint16_t type = model::typeIndexOf<DirectMatrixAeroJTable>();
        registerColumn(catalogue, type, "entryName", [](const auto& model)
                       { return exportTextColumn(model->template table<DirectMatrixAeroJTable>().entryNameColumn()); });
        registerColumn(catalogue, type, "nodeJ", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAeroJTable>().nodeJColumn()); });
        registerColumn(catalogue, type, "componentsJ", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAeroJTable>().componentsJColumn()); });
        registerColumn(catalogue, type, "matrixShape", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAeroJTable>().matrixShapeColumn()); });
        registerColumn(catalogue, type, "inputType", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAeroJTable>().inputTypeColumn()); });
        registerColumn(catalogue, type, "outputType", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAeroJTable>().outputTypeColumn()); });
        registerColumn(catalogue, type, "polarFlag", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAeroJTable>().polarFlagColumn()); });
        registerColumn(catalogue, type, "columnCount", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAeroJTable>().columnCountColumn()); });
        registerColumn(catalogue, type, "imaginary", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAeroJTable>().imaginaryColumn()); });
        registerColumn(catalogue, type, "rows", [](const auto& model)
                       { return exportVariableColumn(model->template table<DirectMatrixAeroJTable>().pointColumn()); });
        registerColumn(
            catalogue, type, "rowComponents", [](const auto& model)
            { return exportVariableColumn(model->template table<DirectMatrixAeroJTable>().componentColumn()); });
        registerColumn(catalogue, type, "real", [](const auto& model)
                       { return exportVariableColumn(model->template table<DirectMatrixAeroJTable>().realColumn()); });
        registerColumn(
            catalogue, type, "imaginary", [](const auto& model)
            { return exportVariableColumn(model->template table<DirectMatrixAeroJTable>().imaginaryPartColumn()); });
    }
    {
        using model::DirectMatrixAeroJInterferenceTable;
        const std::uint16_t type = model::typeIndexOf<DirectMatrixAeroJInterferenceTable>();
        registerColumn(
            catalogue, type, "entryName",
            [](const auto& model)
            {
                return exportTextColumn(model->template table<DirectMatrixAeroJInterferenceTable>().entryNameColumn());
            });
        registerColumn(
            catalogue, type, "nodeJ", [](const auto& model)
            { return exportColumn(model->template table<DirectMatrixAeroJInterferenceTable>().nodeJColumn()); });
        registerColumn(
            catalogue, type, "componentsJ", [](const auto& model)
            { return exportColumn(model->template table<DirectMatrixAeroJInterferenceTable>().componentsJColumn()); });
        registerColumn(
            catalogue, type, "matrixShape", [](const auto& model)
            { return exportColumn(model->template table<DirectMatrixAeroJInterferenceTable>().matrixShapeColumn()); });
        registerColumn(
            catalogue, type, "inputType", [](const auto& model)
            { return exportColumn(model->template table<DirectMatrixAeroJInterferenceTable>().inputTypeColumn()); });
        registerColumn(
            catalogue, type, "outputType", [](const auto& model)
            { return exportColumn(model->template table<DirectMatrixAeroJInterferenceTable>().outputTypeColumn()); });
        registerColumn(
            catalogue, type, "polarFlag", [](const auto& model)
            { return exportColumn(model->template table<DirectMatrixAeroJInterferenceTable>().polarFlagColumn()); });
        registerColumn(
            catalogue, type, "columnCount", [](const auto& model)
            { return exportColumn(model->template table<DirectMatrixAeroJInterferenceTable>().columnCountColumn()); });
        registerColumn(
            catalogue, type, "imaginary", [](const auto& model)
            { return exportColumn(model->template table<DirectMatrixAeroJInterferenceTable>().imaginaryColumn()); });
        registerColumn(
            catalogue, type, "rows",
            [](const auto& model)
            {
                return exportVariableColumn(model->template table<DirectMatrixAeroJInterferenceTable>().pointColumn());
            });
        registerColumn(catalogue, type, "rowComponents",
                       [](const auto& model)
                       {
                           return exportVariableColumn(
                               model->template table<DirectMatrixAeroJInterferenceTable>().componentColumn());
                       });
        registerColumn(
            catalogue, type, "real", [](const auto& model)
            { return exportVariableColumn(model->template table<DirectMatrixAeroJInterferenceTable>().realColumn()); });
        registerColumn(catalogue, type, "imaginary",
                       [](const auto& model)
                       {
                           return exportVariableColumn(
                               model->template table<DirectMatrixAeroJInterferenceTable>().imaginaryPartColumn());
                       });
    }
    {
        using model::DirectMatrixAeroKTable;
        const std::uint16_t type = model::typeIndexOf<DirectMatrixAeroKTable>();
        registerColumn(catalogue, type, "entryName", [](const auto& model)
                       { return exportTextColumn(model->template table<DirectMatrixAeroKTable>().entryNameColumn()); });
        registerColumn(catalogue, type, "nodeJ", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAeroKTable>().nodeJColumn()); });
        registerColumn(catalogue, type, "componentsJ", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAeroKTable>().componentsJColumn()); });
        registerColumn(catalogue, type, "matrixShape", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAeroKTable>().matrixShapeColumn()); });
        registerColumn(catalogue, type, "inputType", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAeroKTable>().inputTypeColumn()); });
        registerColumn(catalogue, type, "outputType", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAeroKTable>().outputTypeColumn()); });
        registerColumn(catalogue, type, "polarFlag", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAeroKTable>().polarFlagColumn()); });
        registerColumn(catalogue, type, "columnCount", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAeroKTable>().columnCountColumn()); });
        registerColumn(catalogue, type, "imaginary", [](const auto& model)
                       { return exportColumn(model->template table<DirectMatrixAeroKTable>().imaginaryColumn()); });
        registerColumn(catalogue, type, "rows", [](const auto& model)
                       { return exportVariableColumn(model->template table<DirectMatrixAeroKTable>().pointColumn()); });
        registerColumn(
            catalogue, type, "rowComponents", [](const auto& model)
            { return exportVariableColumn(model->template table<DirectMatrixAeroKTable>().componentColumn()); });
        registerColumn(catalogue, type, "real", [](const auto& model)
                       { return exportVariableColumn(model->template table<DirectMatrixAeroKTable>().realColumn()); });
        registerColumn(
            catalogue, type, "imaginary", [](const auto& model)
            { return exportVariableColumn(model->template table<DirectMatrixAeroKTable>().imaginaryPartColumn()); });
    }
    {
        using model::DirectTableTable;
        const std::uint16_t type = model::typeIndexOf<DirectTableTable>();
        registerColumn(catalogue, type, "entryName", [](const auto& model)
                       { return exportTextColumn(model->template table<DirectTableTable>().entryNameColumn()); });
        registerColumn(catalogue, type, "recordNumber", [](const auto& model)
                       { return exportColumn(model->template table<DirectTableTable>().recordNumberColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportVariableColumn(model->template table<DirectTableTable>().kindColumn()); });
        registerColumn(catalogue, type, "integer", [](const auto& model)
                       { return exportVariableColumn(model->template table<DirectTableTable>().integerColumn()); });
        registerColumn(catalogue, type, "real", [](const auto& model)
                       { return exportVariableColumn(model->template table<DirectTableTable>().realColumn()); });
    }
    {
        using model::MonteCarloParamsTable;
        const std::uint16_t type = model::typeIndexOf<MonteCarloParamsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<MonteCarloParamsTable>().idColumn()); });
    }
    {
        using model::MonitorChordNormalCoeffsTable;
        const std::uint16_t type = model::typeIndexOf<MonitorChordNormalCoeffsTable>();
        registerColumn(
            catalogue, type, "name", [](const auto& model)
            { return exportTextColumn(model->template table<MonitorChordNormalCoeffsTable>().nameColumn()); });
    }
    {
        using model::MonitorDisplacementTable;
        const std::uint16_t type = model::typeIndexOf<MonitorDisplacementTable>();
        registerColumn(catalogue, type, "name", [](const auto& model)
                       { return exportTextColumn(model->template table<MonitorDisplacementTable>().nameColumn()); });
        registerColumn(catalogue, type, "label", [](const auto& model)
                       { return exportTextColumn(model->template table<MonitorDisplacementTable>().labelColumn()); });
        registerColumn(catalogue, type, "outputAxes", [](const auto& model)
                       { return exportColumn(model->template table<MonitorDisplacementTable>().outputAxesColumn()); });
        registerColumn(
            catalogue, type, "component", [](const auto& model)
            { return exportTextColumn(model->template table<MonitorDisplacementTable>().componentColumn()); });
        registerColumn(
            catalogue, type, "positionFrameId", [](const auto& model)
            { return exportColumn(model->template table<MonitorDisplacementTable>().positionFrameIdColumn()); });
        registerColumn(catalogue, type, "xCoord", [](const auto& model)
                       { return exportColumn(model->template table<MonitorDisplacementTable>().xCoordColumn()); });
        registerColumn(catalogue, type, "yCoord", [](const auto& model)
                       { return exportColumn(model->template table<MonitorDisplacementTable>().yCoordColumn()); });
        registerColumn(catalogue, type, "zCoord", [](const auto& model)
                       { return exportColumn(model->template table<MonitorDisplacementTable>().zCoordColumn()); });
        registerColumn(
            catalogue, type, "displacementFrameId", [](const auto& model)
            { return exportColumn(model->template table<MonitorDisplacementTable>().displacementFrameIdColumn()); });
        registerColumn(
            catalogue, type, "sampledComponents", [](const auto& model)
            { return exportColumn(model->template table<MonitorDisplacementTable>().sampledComponentsColumn()); });
    }
    {
        using model::MonitorGroupTable;
        const std::uint16_t type = model::typeIndexOf<MonitorGroupTable>();
        registerColumn(catalogue, type, "name", [](const auto& model)
                       { return exportTextColumn(model->template table<MonitorGroupTable>().groupNameColumn()); });
    }
    {
        using model::MonitorIntegratedLoad1Table;
        const std::uint16_t type = model::typeIndexOf<MonitorIntegratedLoad1Table>();
        registerColumn(catalogue, type, "name", [](const auto& model)
                       { return exportTextColumn(model->template table<MonitorIntegratedLoad1Table>().nameColumn()); });
        registerColumn(
            catalogue, type, "label", [](const auto& model)
            { return exportTextColumn(model->template table<MonitorIntegratedLoad1Table>().labelColumn()); });
        registerColumn(
            catalogue, type, "outputAxes", [](const auto& model)
            { return exportColumn(model->template table<MonitorIntegratedLoad1Table>().outputAxesColumn()); });
        registerColumn(
            catalogue, type, "component", [](const auto& model)
            { return exportTextColumn(model->template table<MonitorIntegratedLoad1Table>().componentColumn()); });
        registerColumn(
            catalogue, type, "positionFrameId", [](const auto& model)
            { return exportColumn(model->template table<MonitorIntegratedLoad1Table>().positionFrameIdColumn()); });
        registerColumn(catalogue, type, "xCoord", [](const auto& model)
                       { return exportColumn(model->template table<MonitorIntegratedLoad1Table>().xCoordColumn()); });
        registerColumn(catalogue, type, "yCoord", [](const auto& model)
                       { return exportColumn(model->template table<MonitorIntegratedLoad1Table>().yCoordColumn()); });
        registerColumn(catalogue, type, "zCoord", [](const auto& model)
                       { return exportColumn(model->template table<MonitorIntegratedLoad1Table>().zCoordColumn()); });
        registerColumn(
            catalogue, type, "displacementFrameId", [](const auto& model)
            { return exportColumn(model->template table<MonitorIntegratedLoad1Table>().displacementFrameIdColumn()); });
    }
    {
        using model::MonitorElemLoad2Table;
        const std::uint16_t type = model::typeIndexOf<MonitorElemLoad2Table>();
        registerColumn(catalogue, type, "name", [](const auto& model)
                       { return exportTextColumn(model->template table<MonitorElemLoad2Table>().nameColumn()); });
        registerColumn(catalogue, type, "label", [](const auto& model)
                       { return exportTextColumn(model->template table<MonitorElemLoad2Table>().labelColumn()); });
        registerColumn(
            catalogue, type, "resultTable", [](const auto& model)
            { return exportTextColumn(model->template table<MonitorElemLoad2Table>().resultTableColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportTextColumn(model->template table<MonitorElemLoad2Table>().kindColumn()); });
        registerColumn(catalogue, type, "resultItem", [](const auto& model)
                       { return exportTextColumn(model->template table<MonitorElemLoad2Table>().resultItemColumn()); });
        registerColumn(catalogue, type, "elemId", [](const auto& model)
                       { return exportColumn(model->template table<MonitorElemLoad2Table>().elemIdColumn()); });
    }
    {
        using model::MonitorNodeForceSum3Table;
        const std::uint16_t type = model::typeIndexOf<MonitorNodeForceSum3Table>();
        registerColumn(catalogue, type, "name", [](const auto& model)
                       { return exportTextColumn(model->template table<MonitorNodeForceSum3Table>().nameColumn()); });
        registerColumn(catalogue, type, "label", [](const auto& model)
                       { return exportTextColumn(model->template table<MonitorNodeForceSum3Table>().labelColumn()); });
        registerColumn(catalogue, type, "outputAxes", [](const auto& model)
                       { return exportColumn(model->template table<MonitorNodeForceSum3Table>().outputAxesColumn()); });
        registerColumn(catalogue, type, "nodeSetId", [](const auto& model)
                       { return exportColumn(model->template table<MonitorNodeForceSum3Table>().nodeSetIdColumn()); });
        registerColumn(catalogue, type, "elementSetId", [](const auto& model)
                       { return exportColumn(model->template table<MonitorNodeForceSum3Table>().elementSetColumn()); });
        registerColumn(
            catalogue, type, "positionFrameId", [](const auto& model)
            { return exportColumn(model->template table<MonitorNodeForceSum3Table>().positionFrameIdColumn()); });
        registerColumn(catalogue, type, "xCoord", [](const auto& model)
                       { return exportColumn(model->template table<MonitorNodeForceSum3Table>().xCoordColumn()); });
        registerColumn(catalogue, type, "yCoord", [](const auto& model)
                       { return exportColumn(model->template table<MonitorNodeForceSum3Table>().yCoordColumn()); });
        registerColumn(catalogue, type, "zCoord", [](const auto& model)
                       { return exportColumn(model->template table<MonitorNodeForceSum3Table>().zCoordColumn()); });
        registerColumn(
            catalogue, type, "excludedForces", [](const auto& model)
            { return exportTextColumn(model->template table<MonitorNodeForceSum3Table>().excludedColumn()); });
        registerColumn(
            catalogue, type, "displacementFrameId", [](const auto& model)
            { return exportColumn(model->template table<MonitorNodeForceSum3Table>().displacementFrameIdColumn()); });
    }
    {
        using model::MonitorSumTable;
        const std::uint16_t type = model::typeIndexOf<MonitorSumTable>();
        registerColumn(catalogue, type, "name", [](const auto& model)
                       { return exportTextColumn(model->template table<MonitorSumTable>().nameColumn()); });
    }
    {
        using model::MonitorSumLocatedTable;
        const std::uint16_t type = model::typeIndexOf<MonitorSumLocatedTable>();
        registerColumn(catalogue, type, "name", [](const auto& model)
                       { return exportTextColumn(model->template table<MonitorSumLocatedTable>().nameColumn()); });
    }
    {
        using model::MonitorSumTotalTable;
        const std::uint16_t type = model::typeIndexOf<MonitorSumTotalTable>();
        registerColumn(catalogue, type, "entryName", [](const auto& model)
                       { return exportTextColumn(model->template table<MonitorSumTotalTable>().entryNameColumn()); });
    }
}

} // namespace oor::query::column_export
