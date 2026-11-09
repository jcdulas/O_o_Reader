#include "oor/query/named/results/r1_nodal/NodalResultQueries.hpp"

namespace oor::query
{

void addR1Queries(NamedQueryRegistry& registry)
{
    addDisplacementsTableQuery(registry);
    addVelocitiesTableQuery(registry);
    addAccelerationsTableQuery(registry);
    addMaxDisplacementQuery(registry);
    addRelativeDisplacementQuery(registry);
    addSinglePointConstrForcesTableQuery(registry);
    addMultiPointConstrForcesTableQuery(registry);
    addReactionResultantQuery(registry);
}

} // namespace oor::query
