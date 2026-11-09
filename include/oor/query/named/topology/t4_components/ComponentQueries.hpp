#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family T4, components and zones: the components, the property zones, the chains of 1D elements and the panels, and
// the hooks for companion packages (membership, load case groups, boundaries, revision stamps, constraint stamps), the
// groups a caller gives and the zones.
void addConnectedComponentsQuery(NamedQueryRegistry& registry);
void addLoadCaseGroupsQuery(NamedQueryRegistry& registry);
void addComponentBoundariesQuery(NamedQueryRegistry& registry);
void addPropertyZonesQuery(NamedQueryRegistry& registry);
void addElementChainsQuery(NamedQueryRegistry& registry);
void addGroupRevisionsQuery(NamedQueryRegistry& registry);
void addConstraintStampsQuery(NamedQueryRegistry& registry);
void addZoneBoundariesQuery(NamedQueryRegistry& registry);
void addEnclosedElementsQuery(NamedQueryRegistry& registry);

void addT4Queries(NamedQueryRegistry& registry);

} // namespace oor::query
