#include "oor/query/named/topology/t10_aero/AeroQueries.hpp"

namespace oor::query
{

void addT10Queries(NamedQueryRegistry& registry)
{
    addAeroReferenceTableQuery(registry);
    addAeroPanelsTableQuery(registry);
    addAeroBoxesTableQuery(registry);
    addSplinesTableQuery(registry);
    addSplineNodesQuery(registry);
    addControlSurfacesTableQuery(registry);
    addTrimCasesTableQuery(registry);
    addFlutterCasesTableQuery(registry);
    addGustsTableQuery(registry);
}

} // namespace oor::query
