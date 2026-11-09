#include "oor/query/named/topology/t12_laminates/LaminateQueries.hpp"

namespace oor::query
{

void addT12Queries(NamedQueryRegistry& registry)
{
    addLaminatePliesTableQuery(registry);
    addLaminateSummaryQuery(registry);
    addSandwichLayupQuery(registry);
    addPlyContinuityQuery(registry);
}

} // namespace oor::query
