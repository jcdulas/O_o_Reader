#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family T12, laminates: the plies of each laminate, a summary per laminate, and the continuity of the plies between
// neighbouring property zones.
void addLaminatePliesTableQuery(NamedQueryRegistry& registry);
void addLaminateSummaryQuery(NamedQueryRegistry& registry);
void addSandwichLayupQuery(NamedQueryRegistry& registry);
void addPlyContinuityQuery(NamedQueryRegistry& registry);

void addT12Queries(NamedQueryRegistry& registry);

} // namespace oor::query
