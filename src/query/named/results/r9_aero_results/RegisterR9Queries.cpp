#include "oor/query/named/results/r9_aero_results/AeroResultQueries.hpp"

namespace oor::query
{

void addR9Queries(NamedQueryRegistry& registry)
{
    addTrimResultsQuery(registry);
    addStabilityDerivativesTableQuery(registry);
    addAeroPressuresTableQuery(registry);
    addAeroForcesTableQuery(registry);
    addControlSurfaceResultsQuery(registry);
    addFlutterSummaryQuery(registry);
    addFlutterCrossingsQuery(registry);
}

} // namespace oor::query
