#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family T6, connections: the rigid elements and the MPC equations, the fasteners and springs, the interfaces they make
// between components, the connection system (fastener piercings, connection sharing, spiders, clusters) and the contact
// between bodies.
void addRigidElementsTableQuery(NamedQueryRegistry& registry);
void addMultiPointConstrTableQuery(NamedQueryRegistry& registry);
void addFastenersTableQuery(NamedQueryRegistry& registry);
void addInterfacesQuery(NamedQueryRegistry& registry);
void addFastenerNeighbourhoodsQuery(NamedQueryRegistry& registry);
void addFastenerPiercingsQuery(NamedQueryRegistry& registry);
void addConnectionSharingQuery(NamedQueryRegistry& registry);
void addSpidersQuery(NamedQueryRegistry& registry);
void addConnectionClustersQuery(NamedQueryRegistry& registry);
void addContactNodePairsQuery(NamedQueryRegistry& registry);
void addContactPairsTableQuery(NamedQueryRegistry& registry);
void addContactTablesTableQuery(NamedQueryRegistry& registry);
void addConnectionNodesQuery(NamedQueryRegistry& registry);

void addT6Queries(NamedQueryRegistry& registry);

} // namespace oor::query
