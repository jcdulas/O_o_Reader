#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family T13, comparison: the differences between two models, card by card and field by field.
void addModelDiffQuery(NamedQueryRegistry& registry);

void addT13Queries(NamedQueryRegistry& registry);

} // namespace oor::query
