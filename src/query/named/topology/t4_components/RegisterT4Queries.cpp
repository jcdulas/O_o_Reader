#include "oor/query/named/topology/t4_components/ComponentQueries.hpp"

namespace oor::query
{

void addT4Queries(NamedQueryRegistry& registry)
{
    addConnectedComponentsQuery(registry);
    addLoadCaseGroupsQuery(registry);
    addComponentBoundariesQuery(registry);
    addPropertyZonesQuery(registry);
    addElementChainsQuery(registry);
    addGroupRevisionsQuery(registry);
    addConstraintStampsQuery(registry);
    addZoneBoundariesQuery(registry);
    addEnclosedElementsQuery(registry);
}

} // namespace oor::query
