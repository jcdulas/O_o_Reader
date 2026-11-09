#include "oor/query/named/results/r5_margins/MarginQueries.hpp"

namespace oor::query
{

void addR5Queries(NamedQueryRegistry& registry)
{
    addMarginsQuery(registry);
    addMarginSummaryQuery(registry);
}

} // namespace oor::query
