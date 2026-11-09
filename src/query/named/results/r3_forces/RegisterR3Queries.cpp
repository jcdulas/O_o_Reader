#include "oor/query/named/results/r3_forces/ForceQueries.hpp"

namespace oor::query
{

void addR3Queries(NamedQueryRegistry& registry)
{
    addElementForcesTableQuery(registry);
    addShellFluxesQuery(registry);
    addFastenerForcesQuery(registry);
    addBarEndLoadsQuery(registry);
}

} // namespace oor::query
