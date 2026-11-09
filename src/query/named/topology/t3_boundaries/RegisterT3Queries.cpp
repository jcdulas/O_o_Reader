#include "oor/query/named/topology/t3_boundaries/BoundaryQueries.hpp"

namespace oor::query
{

void addT3Queries(NamedQueryRegistry& registry)
{
    addFreeEdgesQuery(registry);
    addFreeFacesQuery(registry);
    addNonManifoldEdgesQuery(registry);
    addFeatureEdgesQuery(registry);
    addNormalFlipsQuery(registry);
    addPropertyBoundariesQuery(registry);
}

} // namespace oor::query
