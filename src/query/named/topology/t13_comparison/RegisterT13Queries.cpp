#include "oor/query/named/topology/t13_comparison/ComparisonQueries.hpp"

namespace oor::query
{

void addT13Queries(NamedQueryRegistry& registry)
{
    addModelDiffQuery(registry);
}

} // namespace oor::query
