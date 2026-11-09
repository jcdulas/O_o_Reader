#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family R7, analysis checks: the equilibrium of the loads and the reactions, and the mass of the model against the
// grid point weight of the result file.
void addEquilibriumCheckQuery(NamedQueryRegistry& registry);
void addMassCheckQuery(NamedQueryRegistry& registry);

void addR7Queries(NamedQueryRegistry& registry);

} // namespace oor::query
