#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family T2, connectivity: which nodes belong to which elements, which elements touch, the edges and the faces.
void addNodeElementsQuery(NamedQueryRegistry& registry);
void addElementNeighborsQuery(NamedQueryRegistry& registry);
void addEdgesTableQuery(NamedQueryRegistry& registry);
void addFacesTableQuery(NamedQueryRegistry& registry);

void addT2Queries(NamedQueryRegistry& registry);

} // namespace oor::query
