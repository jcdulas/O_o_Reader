#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family T8, loads and constraints: the subcases, the loads with their LOAD combinations expanded, the resultant of the
// loads, the single point constraints, pressures, line loads and temperatures, and the rest of the set-up of an
// analysis (the inertial loads, the output requests and the sets of the Case Control, the parameters, the DOF set
// cards).
void addSubcasesTableQuery(NamedQueryRegistry& registry);
void addLoadsTableQuery(NamedQueryRegistry& registry);
void addLoadCombinationsTableQuery(NamedQueryRegistry& registry);
void addLoadResultantQuery(NamedQueryRegistry& registry);
void addSinglePointConstrTableQuery(NamedQueryRegistry& registry);
void addPressureTableQuery(NamedQueryRegistry& registry);
void addElementPressureQuery(NamedQueryRegistry& registry);
void addLineLoadsTableQuery(NamedQueryRegistry& registry);
void addTemperatureTableQuery(NamedQueryRegistry& registry);
void addInertialLoadsTableQuery(NamedQueryRegistry& registry);
void addOutputRequestsTableQuery(NamedQueryRegistry& registry);
void addCaseSetsTableQuery(NamedQueryRegistry& registry);
void addParamsTableQuery(NamedQueryRegistry& registry);
void addDofSetsTableQuery(NamedQueryRegistry& registry);

void addT8Queries(NamedQueryRegistry& registry);

} // namespace oor::query
