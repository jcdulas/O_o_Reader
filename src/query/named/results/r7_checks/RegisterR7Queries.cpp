#include "oor/query/named/results/r7_checks/CheckQueries.hpp"

namespace oor::query
{

void addR7Queries(NamedQueryRegistry& registry)
{
    addEquilibriumCheckQuery(registry);
    addMassCheckQuery(registry);
}

} // namespace oor::query
