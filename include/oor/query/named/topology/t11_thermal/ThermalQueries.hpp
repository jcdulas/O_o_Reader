#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family T11, the thermal model: the heat transfer materials, the surface elements, the convection and radiation
// boundary conditions, the heat loads with their power, and the temperature boundary conditions.
void addThermalMaterialsTableQuery(NamedQueryRegistry& registry);
void addThermalBoundaryElementsTableQuery(NamedQueryRegistry& registry);
void addConvectionTableQuery(NamedQueryRegistry& registry);
void addRadiationTableQuery(NamedQueryRegistry& registry);
void addHeatLoadsTableQuery(NamedQueryRegistry& registry);
void addThermalBoundaryConditionsTableQuery(NamedQueryRegistry& registry);

void addT11Queries(NamedQueryRegistry& registry);

} // namespace oor::query
