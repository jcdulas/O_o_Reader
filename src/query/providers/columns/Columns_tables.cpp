// The stored columns of the cards of the family "tables", for the query engine (ColumnExport.hpp).
// One block a card type: its type index, then a getter a column.

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/tables/tableDynamic1/TableDynamic1Table.hpp"
#include "oor/model/cards/tables/tableDynamic2/TableDynamic2Table.hpp"
#include "oor/model/cards/tables/tableDynamic3/TableDynamic3Table.hpp"
#include "oor/model/cards/tables/tableDynamic4/TableDynamic4Table.hpp"
#include "oor/model/cards/tables/tableDynamic5/TableDynamic5Table.hpp"
#include "oor/model/cards/tables/tableFatigueHistory/TableFatigueHistoryTable.hpp"
#include "oor/model/cards/tables/tableGustSpectrum/TableGustSpectrumTable.hpp"
#include "oor/model/cards/tables/tableHeat1/TableHeat1Table.hpp"
#include "oor/model/cards/tables/tableHeatTwoVariables/TableHeatTwoVariablesTable.hpp"
#include "oor/model/cards/tables/tableMaterial1/TableMaterial1Table.hpp"
#include "oor/model/cards/tables/tableMaterial2/TableMaterial2Table.hpp"
#include "oor/model/cards/tables/tableMaterial3/TableMaterial3Table.hpp"
#include "oor/model/cards/tables/tableMaterial4/TableMaterial4Table.hpp"
#include "oor/model/cards/tables/tableModalDamping1/TableModalDamping1Table.hpp"
#include "oor/model/cards/tables/tableMultiVariable0/TableMultiVariable0Table.hpp"
#include "oor/model/cards/tables/tableMultiVariable1/TableMultiVariable1Table.hpp"
#include "oor/model/cards/tables/tableMultiVariable2/TableMultiVariable2Table.hpp"
#include "oor/model/cards/tables/tableRandomSpectrum1/TableRandomSpectrum1Table.hpp"
#include "oor/model/cards/tables/tableScattered3D/TableScattered3DTable.hpp"
#include "oor/model/cards/tables/tableStress1/TableStress1Table.hpp"
#include "oor/model/cards/tables/tableStressTemperature/TableStressTemperatureTable.hpp"
#include "oor/model/cards/tables/tableTimeHistoryFile/TableTimeHistoryFileTable.hpp"
#include "oor/model/cards/tables/tableUserRoutine/TableUserRoutineTable.hpp"
#include "oor/query/providers/ColumnExport.hpp"

namespace oor::query::column_export
{

void registerTablesColumns(StoredColumnCatalogue& catalogue)
{
    {
        using model::TableModalDamping1Table;
        const std::uint16_t type = model::typeIndexOf<TableModalDamping1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableModalDamping1Table>().idColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportColumn(model->template table<TableModalDamping1Table>().typeColumn()); });
        registerColumn(catalogue, type, "x", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableModalDamping1Table>().xColumn()); });
        registerColumn(catalogue, type, "y", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableModalDamping1Table>().yColumn()); });
    }
    {
        using model::TableMultiVariable0Table;
        const std::uint16_t type = model::typeIndexOf<TableMultiVariable0Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableMultiVariable0Table>().idColumn()); });
    }
    {
        using model::TableMultiVariable1Table;
        const std::uint16_t type = model::typeIndexOf<TableMultiVariable1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableMultiVariable1Table>().idColumn()); });
    }
    {
        using model::TableMultiVariable2Table;
        const std::uint16_t type = model::typeIndexOf<TableMultiVariable2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableMultiVariable2Table>().idColumn()); });
    }
    {
        using model::TableScattered3DTable;
        const std::uint16_t type = model::typeIndexOf<TableScattered3DTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableScattered3DTable>().idColumn()); });
    }
    {
        using model::TableDynamic1Table;
        const std::uint16_t type = model::typeIndexOf<TableDynamic1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableDynamic1Table>().idColumn()); });
        registerColumn(catalogue, type, "xAxisKind", [](const auto& model)
                       { return exportColumn(model->template table<TableDynamic1Table>().xAxisKindColumn()); });
        registerColumn(catalogue, type, "yAxisKind", [](const auto& model)
                       { return exportColumn(model->template table<TableDynamic1Table>().yAxisKindColumn()); });
        registerColumn(catalogue, type, "x", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableDynamic1Table>().xColumn()); });
        registerColumn(catalogue, type, "y", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableDynamic1Table>().yColumn()); });
    }
    {
        using model::TableDynamic2Table;
        const std::uint16_t type = model::typeIndexOf<TableDynamic2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableDynamic2Table>().idColumn()); });
        registerColumn(catalogue, type, "abscissaShift", [](const auto& model)
                       { return exportColumn(model->template table<TableDynamic2Table>().abscissaShiftColumn()); });
        registerColumn(catalogue, type, "x", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableDynamic2Table>().xColumn()); });
        registerColumn(catalogue, type, "y", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableDynamic2Table>().yColumn()); });
    }
    {
        using model::TableDynamic3Table;
        const std::uint16_t type = model::typeIndexOf<TableDynamic3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableDynamic3Table>().idColumn()); });
        registerColumn(catalogue, type, "abscissaShift", [](const auto& model)
                       { return exportColumn(model->template table<TableDynamic3Table>().abscissaShiftColumn()); });
        registerColumn(catalogue, type, "abscissaScale", [](const auto& model)
                       { return exportColumn(model->template table<TableDynamic3Table>().abscissaScaleColumn()); });
        registerColumn(catalogue, type, "x", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableDynamic3Table>().xColumn()); });
        registerColumn(catalogue, type, "y", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableDynamic3Table>().yColumn()); });
    }
    {
        using model::TableDynamic4Table;
        const std::uint16_t type = model::typeIndexOf<TableDynamic4Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableDynamic4Table>().idColumn()); });
        registerColumn(catalogue, type, "abscissaShift", [](const auto& model)
                       { return exportColumn(model->template table<TableDynamic4Table>().abscissaShiftColumn()); });
        registerColumn(catalogue, type, "abscissaScale", [](const auto& model)
                       { return exportColumn(model->template table<TableDynamic4Table>().abscissaScaleColumn()); });
        registerColumn(catalogue, type, "abscissaMinimum", [](const auto& model)
                       { return exportColumn(model->template table<TableDynamic4Table>().abscissaMinimumColumn()); });
        registerColumn(catalogue, type, "abscissaMaximum", [](const auto& model)
                       { return exportColumn(model->template table<TableDynamic4Table>().abscissaMaximumColumn()); });
        registerColumn(
            catalogue, type, "coefficients", [](const auto& model)
            { return exportVariableColumn(model->template table<TableDynamic4Table>().coefficientColumn()); });
    }
    {
        using model::TableDynamic5Table;
        const std::uint16_t type = model::typeIndexOf<TableDynamic5Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableDynamic5Table>().idColumn()); });
        registerColumn(catalogue, type, "x", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableDynamic5Table>().xColumn()); });
        registerColumn(catalogue, type, "tables", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableDynamic5Table>().tablesColumn()); });
    }
    {
        using model::TableHeat1Table;
        const std::uint16_t type = model::typeIndexOf<TableHeat1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableHeat1Table>().idColumn()); });
        registerColumn(catalogue, type, "x", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableHeat1Table>().xColumn()); });
        registerColumn(catalogue, type, "y", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableHeat1Table>().yColumn()); });
    }
    {
        using model::TableHeatTwoVariablesTable;
        const std::uint16_t type = model::typeIndexOf<TableHeatTwoVariablesTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableHeatTwoVariablesTable>().idColumn()); });
        registerColumn(catalogue, type, "x", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableHeatTwoVariablesTable>().xColumn()); });
        registerColumn(
            catalogue, type, "tables", [](const auto& model)
            { return exportVariableColumn(model->template table<TableHeatTwoVariablesTable>().tablesColumn()); });
    }
    {
        using model::TableMaterial1Table;
        const std::uint16_t type = model::typeIndexOf<TableMaterial1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableMaterial1Table>().idColumn()); });
        registerColumn(catalogue, type, "xAxisKind", [](const auto& model)
                       { return exportColumn(model->template table<TableMaterial1Table>().xAxisKindColumn()); });
        registerColumn(catalogue, type, "yAxisKind", [](const auto& model)
                       { return exportColumn(model->template table<TableMaterial1Table>().yAxisKindColumn()); });
        registerColumn(catalogue, type, "x", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableMaterial1Table>().xColumn()); });
        registerColumn(catalogue, type, "y", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableMaterial1Table>().yColumn()); });
    }
    {
        using model::TableMaterial2Table;
        const std::uint16_t type = model::typeIndexOf<TableMaterial2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableMaterial2Table>().idColumn()); });
        registerColumn(catalogue, type, "abscissaShift", [](const auto& model)
                       { return exportColumn(model->template table<TableMaterial2Table>().abscissaShiftColumn()); });
        registerColumn(catalogue, type, "x", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableMaterial2Table>().xColumn()); });
        registerColumn(catalogue, type, "y", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableMaterial2Table>().yColumn()); });
    }
    {
        using model::TableMaterial3Table;
        const std::uint16_t type = model::typeIndexOf<TableMaterial3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableMaterial3Table>().idColumn()); });
        registerColumn(catalogue, type, "abscissaShift", [](const auto& model)
                       { return exportColumn(model->template table<TableMaterial3Table>().abscissaShiftColumn()); });
        registerColumn(catalogue, type, "abscissaScale", [](const auto& model)
                       { return exportColumn(model->template table<TableMaterial3Table>().abscissaScaleColumn()); });
        registerColumn(catalogue, type, "x", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableMaterial3Table>().xColumn()); });
        registerColumn(catalogue, type, "y", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableMaterial3Table>().yColumn()); });
    }
    {
        using model::TableMaterial4Table;
        const std::uint16_t type = model::typeIndexOf<TableMaterial4Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableMaterial4Table>().idColumn()); });
        registerColumn(catalogue, type, "abscissaShift", [](const auto& model)
                       { return exportColumn(model->template table<TableMaterial4Table>().abscissaShiftColumn()); });
        registerColumn(catalogue, type, "abscissaScale", [](const auto& model)
                       { return exportColumn(model->template table<TableMaterial4Table>().abscissaScaleColumn()); });
        registerColumn(catalogue, type, "abscissaMinimum", [](const auto& model)
                       { return exportColumn(model->template table<TableMaterial4Table>().abscissaMinimumColumn()); });
        registerColumn(catalogue, type, "abscissaMaximum", [](const auto& model)
                       { return exportColumn(model->template table<TableMaterial4Table>().abscissaMaximumColumn()); });
        registerColumn(
            catalogue, type, "coefficients", [](const auto& model)
            { return exportVariableColumn(model->template table<TableMaterial4Table>().coefficientColumn()); });
    }
    {
        using model::TableStress1Table;
        const std::uint16_t type = model::typeIndexOf<TableStress1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableStress1Table>().idColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportColumn(model->template table<TableStress1Table>().kindColumn()); });
        registerColumn(catalogue, type, "x", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableStress1Table>().xColumn()); });
        registerColumn(catalogue, type, "y", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableStress1Table>().yColumn()); });
    }
    {
        using model::TableStressTemperatureTable;
        const std::uint16_t type = model::typeIndexOf<TableStressTemperatureTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableStressTemperatureTable>().idColumn()); });
        registerColumn(
            catalogue, type, "temperatures",
            [](const auto& model)
            {
                return exportVariableColumn(model->template table<TableStressTemperatureTable>().temperaturesColumn());
            });
        registerColumn(
            catalogue, type, "tables", [](const auto& model)
            { return exportVariableColumn(model->template table<TableStressTemperatureTable>().tablesColumn()); });
    }
    {
        using model::TableFatigueHistoryTable;
        const std::uint16_t type = model::typeIndexOf<TableFatigueHistoryTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableFatigueHistoryTable>().idColumn()); });
        registerColumn(
            catalogue, type, "ordinate", [](const auto& model)
            { return exportVariableColumn(model->template table<TableFatigueHistoryTable>().ordinateColumn()); });
    }
    {
        using model::TableTimeHistoryFileTable;
        const std::uint16_t type = model::typeIndexOf<TableTimeHistoryFileTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableTimeHistoryFileTable>().idColumn()); });
    }
    {
        using model::TableUserRoutineTable;
        const std::uint16_t type = model::typeIndexOf<TableUserRoutineTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableUserRoutineTable>().idColumn()); });
    }
    {
        using model::TableRandomSpectrum1Table;
        const std::uint16_t type = model::typeIndexOf<TableRandomSpectrum1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableRandomSpectrum1Table>().idColumn()); });
        registerColumn(catalogue, type, "xAxisKind", [](const auto& model)
                       { return exportColumn(model->template table<TableRandomSpectrum1Table>().xAxisKindColumn()); });
        registerColumn(catalogue, type, "yAxisKind", [](const auto& model)
                       { return exportColumn(model->template table<TableRandomSpectrum1Table>().yAxisKindColumn()); });
        registerColumn(catalogue, type, "x", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableRandomSpectrum1Table>().xColumn()); });
        registerColumn(catalogue, type, "y", [](const auto& model)
                       { return exportVariableColumn(model->template table<TableRandomSpectrum1Table>().yColumn()); });
    }
    {
        using model::TableGustSpectrumTable;
        const std::uint16_t type = model::typeIndexOf<TableGustSpectrumTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TableGustSpectrumTable>().idColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportColumn(model->template table<TableGustSpectrumTable>().kindColumn()); });
        registerColumn(
            catalogue, type, "turbulenceScaleRatio", [](const auto& model)
            { return exportColumn(model->template table<TableGustSpectrumTable>().turbulenceScaleRatioColumn()); });
        registerColumn(catalogue, type, "gustVelocity", [](const auto& model)
                       { return exportColumn(model->template table<TableGustSpectrumTable>().gustVelocityColumn()); });
    }
}

} // namespace oor::query::column_export
