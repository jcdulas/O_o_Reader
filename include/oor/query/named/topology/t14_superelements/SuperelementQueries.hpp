#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family T14, superelements and parts: the parts of the deck and the superelements with their type, downstream
// superelement, boundary search and file, and the boundary points of each superelement with the downstream point each
// is (superelements and parts for the solver).
void addPartsTableQuery(NamedQueryRegistry& registry);
void addPartBoundariesQuery(NamedQueryRegistry& registry);
void addSuperelementInteriorsQuery(NamedQueryRegistry& registry);

void addT14Queries(NamedQueryRegistry& registry);

} // namespace oor::query
