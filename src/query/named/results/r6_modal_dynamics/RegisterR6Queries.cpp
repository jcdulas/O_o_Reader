#include "oor/query/named/results/r6_modal_dynamics/ModalQueries.hpp"

namespace oor::query
{

void addR6Queries(NamedQueryRegistry& registry)
{
    addModesTableQuery(registry);
    addBucklingFactorsQuery(registry);
    addModeShapeMaxQuery(registry);
    addStrainEnergyByPropertyQuery(registry);
    addFrfTableQuery(registry);
    addPeakResponseQuery(registry);
    addTimeHistoryQuery(registry);
    addMaxOverTimeQuery(registry);
}

} // namespace oor::query
