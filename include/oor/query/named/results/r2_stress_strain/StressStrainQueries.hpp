#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family R2, stresses and strains: the tables, their envelopes over subcases, steps, layers and locations, the critical
// elements, the extremes by property and by material, and the plies of the laminates.
void addStressTableQuery(NamedQueryRegistry& registry);
void addStrainTableQuery(NamedQueryRegistry& registry);
void addStressEnvelopeQuery(NamedQueryRegistry& registry);
void addCriticalElementsQuery(NamedQueryRegistry& registry);
void addStressByPropertyQuery(NamedQueryRegistry& registry);
void addStressByMaterialQuery(NamedQueryRegistry& registry);
void addCompositePlyTableQuery(NamedQueryRegistry& registry);
void addCriticalPlyQuery(NamedQueryRegistry& registry);

void addR2Queries(NamedQueryRegistry& registry);

} // namespace oor::query
