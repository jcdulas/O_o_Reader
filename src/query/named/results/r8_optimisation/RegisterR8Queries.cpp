#include "oor/query/named/results/r8_optimisation/OptimisationQueries.hpp"

namespace oor::query
{

void addR8Queries(NamedQueryRegistry& registry)
{
    addDesignHistoryQuery(registry);
    addDesignVariableHistoryQuery(registry);
    addResponseHistoryQuery(registry);
    addConstraintHistoryQuery(registry);
    addSensitivitiesTableQuery(registry);
    addConstraintDriversQuery(registry);
    addVariableImportanceQuery(registry);
    addVariablesAtBoundsQuery(registry);
    addLinearisedPredictionQuery(registry);
    addDesignComparisonQuery(registry);
    addMassHistoryQuery(registry);
    addSizingTableQuery(registry);
}

} // namespace oor::query
