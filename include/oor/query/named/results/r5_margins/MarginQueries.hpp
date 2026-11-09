#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family R5, margins: the reserve factor and margin of safety of the elements by a criterion, and their summary by
// property, material, component or panel.
void addMarginsQuery(NamedQueryRegistry& registry);
void addMarginSummaryQuery(NamedQueryRegistry& registry);

void addR5Queries(NamedQueryRegistry& registry);

} // namespace oor::query
