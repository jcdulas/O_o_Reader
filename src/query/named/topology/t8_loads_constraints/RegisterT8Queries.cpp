#include "oor/query/named/topology/t8_loads_constraints/LoadConstraintQueries.hpp"

namespace oor::query
{

void addT8Queries(NamedQueryRegistry& registry)
{
    addSubcasesTableQuery(registry);
    addLoadsTableQuery(registry);
    addLoadCombinationsTableQuery(registry);
    addLoadResultantQuery(registry);
    addSinglePointConstrTableQuery(registry);
    addPressureTableQuery(registry);
    addElementPressureQuery(registry);
    addLineLoadsTableQuery(registry);
    addTemperatureTableQuery(registry);
    addInertialLoadsTableQuery(registry);
    addOutputRequestsTableQuery(registry);
    addCaseSetsTableQuery(registry);
    addParamsTableQuery(registry);
    addDofSetsTableQuery(registry);
}

} // namespace oor::query
