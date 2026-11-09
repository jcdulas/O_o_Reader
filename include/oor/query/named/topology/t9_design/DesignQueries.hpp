#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family T9, the design model: the design variables and what they drive, the responses and
// the constraints, the DLINK cards, the elements and the mass a variable drives, what the design model leaves out, and
// what a solver needs to replay the optimisation of a deck: the shape variables, the constants, the arguments of the
// responses, the unions of constraint sets, the design selections of each subcase, the screening, and each DEQATN as an
// expression program.
// They read the optimisation cards of the model (`model.design`) and need no result file.
void addDesignVariablesTableQuery(NamedQueryRegistry& registry);
void addDesignRelationsTableQuery(NamedQueryRegistry& registry);
void addDesignResponsesTableQuery(NamedQueryRegistry& registry);
void addDesignConstraintsTableQuery(NamedQueryRegistry& registry);
void addDesignLinksTableQuery(NamedQueryRegistry& registry);
void addDesignVariableElementsQuery(NamedQueryRegistry& registry);
void addDesignMassTableQuery(NamedQueryRegistry& registry);
void addDesignCoverageQuery(NamedQueryRegistry& registry);
void addShapeVariablesTableQuery(NamedQueryRegistry& registry);
void addDesignConstantsTableQuery(NamedQueryRegistry& registry);
void addDesignResponseArgumentsTableQuery(NamedQueryRegistry& registry);
void addDesignConstraintSetsTableQuery(NamedQueryRegistry& registry);
void addDesignSubcasesTableQuery(NamedQueryRegistry& registry);
void addDesignScreeningTableQuery(NamedQueryRegistry& registry);
void addDesignParametersTableQuery(NamedQueryRegistry& registry);
void addDesignEquationSymbolsQuery(NamedQueryRegistry& registry);
void addDesignEquationInstructionsQuery(NamedQueryRegistry& registry);

void addT9Queries(NamedQueryRegistry& registry);

} // namespace oor::query
