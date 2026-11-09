#include "oor/query/named/topology/t14_superelements/SuperelementQueries.hpp"

namespace oor::query
{

void addT14Queries(NamedQueryRegistry& registry)
{
    addPartsTableQuery(registry);
    addPartBoundariesQuery(registry);
    addSuperelementInteriorsQuery(registry);
}

} // namespace oor::query
