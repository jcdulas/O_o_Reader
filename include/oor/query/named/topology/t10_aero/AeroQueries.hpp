#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family T10, the aeroelastic model: the reference values, the panels and their boxes, the
// splines and their structural nodes, the control surfaces, and the trim, flutter and gust cases. They read the
// aeroelastic cards of the model and need no result file.
void addAeroReferenceTableQuery(NamedQueryRegistry& registry);
void addAeroPanelsTableQuery(NamedQueryRegistry& registry);
void addAeroBoxesTableQuery(NamedQueryRegistry& registry);
void addSplinesTableQuery(NamedQueryRegistry& registry);
void addSplineNodesQuery(NamedQueryRegistry& registry);
void addControlSurfacesTableQuery(NamedQueryRegistry& registry);
void addTrimCasesTableQuery(NamedQueryRegistry& registry);
void addFlutterCasesTableQuery(NamedQueryRegistry& registry);
void addGustsTableQuery(NamedQueryRegistry& registry);

void addT10Queries(NamedQueryRegistry& registry);

} // namespace oor::query
