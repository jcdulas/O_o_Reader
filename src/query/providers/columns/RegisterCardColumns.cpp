// Registers the stored columns of every card family (one file a family in this folder).

#include "oor/query/providers/ColumnExport.hpp"

namespace oor::query::column_export
{

void registerAeroColumns(StoredColumnCatalogue& catalogue);
void registerConstraintsColumns(StoredColumnCatalogue& catalogue);
void registerCoordinateSystemsColumns(StoredColumnCatalogue& catalogue);
void registerDynamicsColumns(StoredColumnCatalogue& catalogue);
void registerElementsColumns(StoredColumnCatalogue& catalogue);
void registerExplicitColumns(StoredColumnCatalogue& catalogue);
void registerLoadsColumns(StoredColumnCatalogue& catalogue);
void registerMaterialsColumns(StoredColumnCatalogue& catalogue);
void registerMatricesColumns(StoredColumnCatalogue& catalogue);
void registerNodesColumns(StoredColumnCatalogue& catalogue);
void registerNonlinearContactColumns(StoredColumnCatalogue& catalogue);
void registerOptimisationColumns(StoredColumnCatalogue& catalogue);
void registerParametersColumns(StoredColumnCatalogue& catalogue);
void registerPropertiesColumns(StoredColumnCatalogue& catalogue);
void registerRigidElementsColumns(StoredColumnCatalogue& catalogue);
void registerSetsColumns(StoredColumnCatalogue& catalogue);
void registerSolutionControlColumns(StoredColumnCatalogue& catalogue);
void registerSuperelementsColumns(StoredColumnCatalogue& catalogue);
void registerTablesColumns(StoredColumnCatalogue& catalogue);
void registerThermalColumns(StoredColumnCatalogue& catalogue);

void registerCardColumns(StoredColumnCatalogue& catalogue)
{
    registerAeroColumns(catalogue);
    registerConstraintsColumns(catalogue);
    registerCoordinateSystemsColumns(catalogue);
    registerDynamicsColumns(catalogue);
    registerElementsColumns(catalogue);
    registerExplicitColumns(catalogue);
    registerLoadsColumns(catalogue);
    registerMaterialsColumns(catalogue);
    registerMatricesColumns(catalogue);
    registerNodesColumns(catalogue);
    registerNonlinearContactColumns(catalogue);
    registerOptimisationColumns(catalogue);
    registerParametersColumns(catalogue);
    registerPropertiesColumns(catalogue);
    registerRigidElementsColumns(catalogue);
    registerSetsColumns(catalogue);
    registerSolutionControlColumns(catalogue);
    registerSuperelementsColumns(catalogue);
    registerTablesColumns(catalogue);
    registerThermalColumns(catalogue);
}

} // namespace oor::query::column_export
