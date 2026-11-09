#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Adds every named query to the registry, in the order of the catalogue. One line per family: each family function
// (src/query/named/topology/t*/RegisterT*Queries.cpp) adds the queries of its family one by one.
void registerNamedQueries(NamedQueryRegistry& registry);

} // namespace oor::query
