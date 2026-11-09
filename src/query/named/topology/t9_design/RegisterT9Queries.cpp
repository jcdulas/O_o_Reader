#include "oor/query/named/topology/t9_design/DesignQueries.hpp"

namespace oor::query
{

void addT9Queries(NamedQueryRegistry& registry)
{
    addDesignVariablesTableQuery(registry);
    addDesignRelationsTableQuery(registry);
    addDesignResponsesTableQuery(registry);
    addDesignConstraintsTableQuery(registry);
    addDesignLinksTableQuery(registry);
    addDesignVariableElementsQuery(registry);
    addDesignMassTableQuery(registry);
    addDesignCoverageQuery(registry);
    addShapeVariablesTableQuery(registry);
    addDesignConstantsTableQuery(registry);
    addDesignResponseArgumentsTableQuery(registry);
    addDesignConstraintSetsTableQuery(registry);
    addDesignSubcasesTableQuery(registry);
    addDesignScreeningTableQuery(registry);
    addDesignParametersTableQuery(registry);
    addDesignEquationSymbolsQuery(registry);
    addDesignEquationInstructionsQuery(registry);
}

} // namespace oor::query
