#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family T5, mesh checks: the quality of the elements against thresholds, nodes that coincide, elements on the same
// nodes, cards nothing refers to and references to cards that do not exist.
void addQualityTableQuery(NamedQueryRegistry& registry);
void addCoincidentNodesQuery(NamedQueryRegistry& registry);
void addDuplicateElementsQuery(NamedQueryRegistry& registry);
void addUnusedCardsQuery(NamedQueryRegistry& registry);
void addDanglingReferencesQuery(NamedQueryRegistry& registry);

void addT5Queries(NamedQueryRegistry& registry);

} // namespace oor::query
