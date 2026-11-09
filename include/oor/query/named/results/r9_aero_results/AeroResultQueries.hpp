#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family R9, aeroelastic results: the trim variables, the stability derivatives, the pressures and forces on the
// aerodynamic model, the control surfaces and the flutter summary with the speeds where a mode goes unstable.
void addTrimResultsQuery(NamedQueryRegistry& registry);
void addStabilityDerivativesTableQuery(NamedQueryRegistry& registry);
void addAeroPressuresTableQuery(NamedQueryRegistry& registry);
void addAeroForcesTableQuery(NamedQueryRegistry& registry);
void addControlSurfaceResultsQuery(NamedQueryRegistry& registry);
void addFlutterSummaryQuery(NamedQueryRegistry& registry);
void addFlutterCrossingsQuery(NamedQueryRegistry& registry);

void addR9Queries(NamedQueryRegistry& registry);

} // namespace oor::query
