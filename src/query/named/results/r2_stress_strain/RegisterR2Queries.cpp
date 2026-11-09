#include "oor/query/named/results/r2_stress_strain/StressStrainQueries.hpp"

namespace oor::query
{

void addR2Queries(NamedQueryRegistry& registry)
{
    addStressTableQuery(registry);
    addStrainTableQuery(registry);
    addStressEnvelopeQuery(registry);
    addCriticalElementsQuery(registry);
    addStressByPropertyQuery(registry);
    addStressByMaterialQuery(registry);
    addCompositePlyTableQuery(registry);
    addCriticalPlyQuery(registry);
}

} // namespace oor::query
