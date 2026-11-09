#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family R1, nodal results: displacements, velocities and accelerations, the largest displacement, the displacement of
// a point relative to another, the forces of the constraints and their resultant.
void addDisplacementsTableQuery(NamedQueryRegistry& registry);
void addVelocitiesTableQuery(NamedQueryRegistry& registry);
void addAccelerationsTableQuery(NamedQueryRegistry& registry);
void addMaxDisplacementQuery(NamedQueryRegistry& registry);
void addRelativeDisplacementQuery(NamedQueryRegistry& registry);
void addSinglePointConstrForcesTableQuery(NamedQueryRegistry& registry);
void addMultiPointConstrForcesTableQuery(NamedQueryRegistry& registry);
void addReactionResultantQuery(NamedQueryRegistry& registry);

void addR1Queries(NamedQueryRegistry& registry);

} // namespace oor::query
