#pragma once

#include "oor/analysis/validation/engine/CheckRegistry.hpp"

// Family 6: coordinate systems. A system, or a CP or CD, that names a system that does not exist is reported by
// ref.unresolved; these are the systems that exist and cannot be resolved.

namespace oor::analysis
{

void addCoordinateCycleCheck(CheckRegistry& registry);
void addCoordinateDegenerateCheck(CheckRegistry& registry);
void addNodeOnAxisCheck(CheckRegistry& registry);

// Adds the checks of the family, in the order they run.
void registerCoordinateChecks(CheckRegistry& registry);

} // namespace oor::analysis
