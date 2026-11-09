#pragma once

#include "oor/analysis/validation/engine/CheckRegistry.hpp"

// Family 10: parts. Each part (BEGIN SUPER=n) has its own ID spaces, so the same IDs in two parts are fine; what is not
// fine is a card that names an ID that exists only in another part.

namespace oor::analysis
{

void addForeignReferenceCheck(CheckRegistry& registry);

// Adds the checks of the family, in the order they run.
void registerPartChecks(CheckRegistry& registry);

} // namespace oor::analysis
