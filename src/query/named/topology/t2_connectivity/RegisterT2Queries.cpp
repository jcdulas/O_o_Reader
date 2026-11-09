#include "oor/query/named/topology/t2_connectivity/ConnectivityQueries.hpp"

namespace oor::query
{

void addT2Queries(NamedQueryRegistry& registry)
{
    addNodeElementsQuery(registry);
    addElementNeighborsQuery(registry);
    addEdgesTableQuery(registry);
    addFacesTableQuery(registry);
}

} // namespace oor::query
