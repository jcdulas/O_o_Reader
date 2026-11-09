#include "oor/query/named/results/r10_thermal_results/ThermalResultQueries.hpp"

namespace oor::query
{

void addR10Queries(NamedQueryRegistry& registry)
{
    addTemperaturesTableQuery(registry);
    addTemperatureExtremesQuery(registry);
    addHeatFluxesTableQuery(registry);
    addBoundaryHeatFlowsTableQuery(registry);
    addConstraintHeatTableQuery(registry);
    addHeatBalanceCheckQuery(registry);
}

} // namespace oor::query
