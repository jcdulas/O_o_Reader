#pragma once

#include "oor/analysis/validation/engine/CheckRegistry.hpp"

// Family 11: geometry, opt-in. Looks at the positions of the grid points in the basic system.

namespace oor::analysis
{

void addCoincidentNodesCheck(CheckRegistry& registry);

// Adds the checks of the family, in the order they run.
void registerGeometryChecks(CheckRegistry& registry);

} // namespace oor::analysis
