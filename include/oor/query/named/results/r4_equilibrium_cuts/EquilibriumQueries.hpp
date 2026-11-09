#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family R4, equilibrium and cuts: the grid point force balance, the resultant of the loads that a free body carries,
// the resultant of the forces across a section or a series of sections, and the integrated loads of the monitor
// points.
void addNodePointForcesTableQuery(NamedQueryRegistry& registry);
void addFreebodyQuery(NamedQueryRegistry& registry);
void addSectionCutQuery(NamedQueryRegistry& registry);
void addSectionCutsQuery(NamedQueryRegistry& registry);
void addMonitorPointLoadsQuery(NamedQueryRegistry& registry);

void addR4Queries(NamedQueryRegistry& registry);

} // namespace oor::query
