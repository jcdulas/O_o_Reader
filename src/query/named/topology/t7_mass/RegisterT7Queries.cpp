#include "oor/query/named/topology/t7_mass/MassQueries.hpp"

namespace oor::query
{

void addT7Queries(NamedQueryRegistry& registry)
{
    addMassByPropertyQuery(registry);
    addMassByMaterialQuery(registry);
    addMassByTypeQuery(registry);
    addMassByComponentQuery(registry);
    addMassBySetQuery(registry);
    addMassPropertiesQuery(registry);
    addNonStructMassTableQuery(registry);
}

} // namespace oor::query
