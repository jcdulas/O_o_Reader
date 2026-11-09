#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family T1, inventory: the nodes, elements, properties, materials and coordinate systems of
// a model, its summary, the section properties of its bar and beam properties and its direct input matrices.
void addNodesTableQuery(NamedQueryRegistry& registry);
void addElementsTableQuery(NamedQueryRegistry& registry);
void addPropertiesTableQuery(NamedQueryRegistry& registry);
void addMaterialsTableQuery(NamedQueryRegistry& registry);
void addCoordsTableQuery(NamedQueryRegistry& registry);
void addModelSummaryQuery(NamedQueryRegistry& registry);
void addSectionsTableQuery(NamedQueryRegistry& registry);
void addElementAllowablesQuery(NamedQueryRegistry& registry);
void addDirectMatricesTableQuery(NamedQueryRegistry& registry);

void addT1Queries(NamedQueryRegistry& registry);

} // namespace oor::query
