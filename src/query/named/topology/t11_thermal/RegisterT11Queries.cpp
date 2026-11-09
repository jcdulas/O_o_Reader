#include "oor/query/named/topology/t11_thermal/ThermalQueries.hpp"

namespace oor::query
{

void addT11Queries(NamedQueryRegistry& registry)
{
    addThermalMaterialsTableQuery(registry);
    addThermalBoundaryElementsTableQuery(registry);
    addConvectionTableQuery(registry);
    addRadiationTableQuery(registry);
    addHeatLoadsTableQuery(registry);
    addThermalBoundaryConditionsTableQuery(registry);
}

} // namespace oor::query
