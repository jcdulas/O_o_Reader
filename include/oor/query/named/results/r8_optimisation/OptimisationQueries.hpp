#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family R8, optimisation: the history of a design (objective, constraints, variables, responses), the sensitivities
// and what they say (the drivers of a constraint, the importance of a variable, the variables at their bounds), a first
// order prediction, the comparison of two cycles and the resizing of the zones that the margins call for.
void addDesignHistoryQuery(NamedQueryRegistry& registry);
void addDesignVariableHistoryQuery(NamedQueryRegistry& registry);
void addResponseHistoryQuery(NamedQueryRegistry& registry);
void addConstraintHistoryQuery(NamedQueryRegistry& registry);
void addSensitivitiesTableQuery(NamedQueryRegistry& registry);
void addConstraintDriversQuery(NamedQueryRegistry& registry);
void addVariableImportanceQuery(NamedQueryRegistry& registry);
void addVariablesAtBoundsQuery(NamedQueryRegistry& registry);
void addLinearisedPredictionQuery(NamedQueryRegistry& registry);
void addDesignComparisonQuery(NamedQueryRegistry& registry);
void addMassHistoryQuery(NamedQueryRegistry& registry);
void addSizingTableQuery(NamedQueryRegistry& registry);

void addR8Queries(NamedQueryRegistry& registry);

} // namespace oor::query
