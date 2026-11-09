#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"
#include "oor/query/providers/quantities/QuantityProvider.hpp"

namespace oor::query
{

// Family R10, thermal results: the temperatures, their extremes, the heat fluxes of the elements, the heat flows of the
// surface elements and at the constraints, and the heat balance.
void addTemperaturesTableQuery(NamedQueryRegistry& registry);
void addTemperatureExtremesQuery(NamedQueryRegistry& registry);
void addHeatFluxesTableQuery(NamedQueryRegistry& registry);
void addBoundaryHeatFlowsTableQuery(NamedQueryRegistry& registry);
void addConstraintHeatTableQuery(NamedQueryRegistry& registry);
void addHeatBalanceCheckQuery(NamedQueryRegistry& registry);

// The ways of obtaining the applied heat, in the order they are tried.
[[nodiscard]] QuantityInfo appliedHeatQuantity();

void addR10Queries(NamedQueryRegistry& registry);

} // namespace oor::query
