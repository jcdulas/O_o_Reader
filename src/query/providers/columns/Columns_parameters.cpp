// The stored columns of the cards of the family "parameters", for the query engine (ColumnExport.hpp).
// One block a card type: its type index, then a getter a column.

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/parameters/parameter/ParameterTable.hpp"
#include "oor/model/parameters/ParameterCatalogue.hpp"
#include "oor/model/parameters/ParameterValues.hpp"
#include "oor/query/providers/ColumnExport.hpp"

namespace oor::query::column_export
{

void registerParametersColumns(StoredColumnCatalogue& catalogue)
{
    {
        using model::ParameterTable;
        const std::uint16_t type = model::typeIndexOf<ParameterTable>();
        registerColumn(catalogue, type, "name", [](const auto& model)
                       { return exportTextColumn(model->template table<ParameterTable>().nameColumn()); });
    }
}

} // namespace oor::query::column_export
