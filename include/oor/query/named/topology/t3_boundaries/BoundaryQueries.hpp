#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family T3, boundaries: the free edges of the shells chained into loops, the free faces of the solids, the edges
// shared by three shells or more, the edges where the shells bend, the shells that run an edge the same way, and the
// edges where the property, the thickness or the material changes.
void addFreeEdgesQuery(NamedQueryRegistry& registry);
void addFreeFacesQuery(NamedQueryRegistry& registry);
void addNonManifoldEdgesQuery(NamedQueryRegistry& registry);
void addFeatureEdgesQuery(NamedQueryRegistry& registry);
void addNormalFlipsQuery(NamedQueryRegistry& registry);
void addPropertyBoundariesQuery(NamedQueryRegistry& registry);

void addT3Queries(NamedQueryRegistry& registry);

} // namespace oor::query
