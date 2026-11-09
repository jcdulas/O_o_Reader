#include "oor/query/named/topology/t6_connections/ConnectionQueries.hpp"

namespace oor::query
{

void addT6Queries(NamedQueryRegistry& registry)
{
    addRigidElementsTableQuery(registry);
    addMultiPointConstrTableQuery(registry);
    addFastenersTableQuery(registry);
    addInterfacesQuery(registry);
    addFastenerNeighbourhoodsQuery(registry);
    addFastenerPiercingsQuery(registry);
    addConnectionSharingQuery(registry);
    addSpidersQuery(registry);
    addConnectionClustersQuery(registry);
    addContactNodePairsQuery(registry);
    addContactPairsTableQuery(registry);
    addContactTablesTableQuery(registry);
    addConnectionNodesQuery(registry);
}

} // namespace oor::query
