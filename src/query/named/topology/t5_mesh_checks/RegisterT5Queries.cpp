#include "oor/query/named/topology/t5_mesh_checks/MeshCheckQueries.hpp"

namespace oor::query
{

void addT5Queries(NamedQueryRegistry& registry)
{
    addQualityTableQuery(registry);
    addCoincidentNodesQuery(registry);
    addDuplicateElementsQuery(registry);
    addUnusedCardsQuery(registry);
    addDanglingReferencesQuery(registry);
}

} // namespace oor::query
