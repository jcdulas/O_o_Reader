#pragma once

#include "oor/analysis/validation/engine/CheckRegistry.hpp"

// Family 1: references and duplicate IDs. Every check is one class in one file of src/analysis/validation/references/,
// with the function that adds it to the registry.

namespace oor::analysis
{

void addUnresolvedReferenceCheck(CheckRegistry& registry);
void addDuplicateIdCheck(CheckRegistry& registry);
void addOrphanPropertyCheck(CheckRegistry& registry);
void addOrphanMaterialCheck(CheckRegistry& registry);
void addOrphanCoordinateCheck(CheckRegistry& registry);
void addOrphanNodeCheck(CheckRegistry& registry);

// Adds the checks of the family, in the order they run.
void registerReferenceChecks(CheckRegistry& registry);

} // namespace oor::analysis
