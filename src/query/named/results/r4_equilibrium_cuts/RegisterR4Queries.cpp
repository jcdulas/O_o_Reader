#include "oor/query/named/results/r4_equilibrium_cuts/EquilibriumQueries.hpp"

namespace oor::query
{

void addR4Queries(NamedQueryRegistry& registry)
{
    addNodePointForcesTableQuery(registry);
    addFreebodyQuery(registry);
    addSectionCutQuery(registry);
    addSectionCutsQuery(registry);
    addMonitorPointLoadsQuery(registry);
}

} // namespace oor::query
