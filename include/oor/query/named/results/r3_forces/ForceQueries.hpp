#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family R3, forces: the forces of the one-dimensional elements, the force resultants of the shells, the forces of the
// fasteners and the loads of the elements on the ends of the bars.
void addElementForcesTableQuery(NamedQueryRegistry& registry);
void addShellFluxesQuery(NamedQueryRegistry& registry);
void addFastenerForcesQuery(NamedQueryRegistry& registry);
void addBarEndLoadsQuery(NamedQueryRegistry& registry);

void addR3Queries(NamedQueryRegistry& registry);

} // namespace oor::query
