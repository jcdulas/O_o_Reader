#include "oor/query/named/topology/t1_inventory/InventoryQueries.hpp"

namespace oor::query
{

void addT1Queries(NamedQueryRegistry& registry)
{
    addNodesTableQuery(registry);
    addElementsTableQuery(registry);
    addPropertiesTableQuery(registry);
    addMaterialsTableQuery(registry);
    addCoordsTableQuery(registry);
    addModelSummaryQuery(registry);
    addSectionsTableQuery(registry);
    addElementAllowablesQuery(registry);
    addDirectMatricesTableQuery(registry);
}

} // namespace oor::query
