#pragma once

#include "oor/analysis/validation/engine/CheckRegistry.hpp"

// Family 8: Case Control. The sets and subcases of the Case Control section against each other and against the Bulk
// Data. They judge the section, which only the residual structure has, so they run once.

namespace oor::analysis
{

void addCaseUnresolvedReferenceCheck(CheckRegistry& registry);
void addCaseUnresolvedSetCheck(CheckRegistry& registry);
void addCaseDuplicateSetCheck(CheckRegistry& registry);
void addCaseEmptySetCheck(CheckRegistry& registry);
void addCaseDuplicateCommandCheck(CheckRegistry& registry);
void addCaseContradictoryCommandCheck(CheckRegistry& registry);
void addCaseDuplicateSubcaseCheck(CheckRegistry& registry);
void addCaseCombinationWithoutFactorsCheck(CheckRegistry& registry);
void addCaseSetMemberMissingCheck(CheckRegistry& registry);
void addCaseTemperatureMaterialInitialCheck(CheckRegistry& registry);
void addCaseTemperatureMaterialNonlinearCheck(CheckRegistry& registry);

// Adds the checks of the family, in the order they run.
void registerCaseControlChecks(CheckRegistry& registry);

} // namespace oor::analysis
