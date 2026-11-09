#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family T7, mass: the mass of the model by property, material, element type, component and set of elements, with
// centres of gravity and inertia, and the non-structural mass cards.
void addMassByPropertyQuery(NamedQueryRegistry& registry);
void addMassByMaterialQuery(NamedQueryRegistry& registry);
void addMassByTypeQuery(NamedQueryRegistry& registry);
void addMassByComponentQuery(NamedQueryRegistry& registry);
void addMassBySetQuery(NamedQueryRegistry& registry);
void addMassPropertiesQuery(NamedQueryRegistry& registry);
void addNonStructMassTableQuery(NamedQueryRegistry& registry);

void addT7Queries(NamedQueryRegistry& registry);

} // namespace oor::query
