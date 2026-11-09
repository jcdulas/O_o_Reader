#pragma once

#include "oor/analysis/validation/engine/CheckRegistry.hpp"

// Family 4: constraints and rigid elements. The single point and multipoint constraints are judged on the sets the Case
// Control selects (all of them when it selects none); the rigid elements always apply.

namespace oor::analysis
{

void addSinglePointConstrListMissingNodeCheck(CheckRegistry& registry);
void addSinglePointConstrListEmptyRangeCheck(CheckRegistry& registry);
void addSpcOnRigidDependentCheck(CheckRegistry& registry);
void addRigidDependentTwiceCheck(CheckRegistry& registry);
void addMpcDependentTwiceCheck(CheckRegistry& registry);
void addSpcOnMpcDependentCheck(CheckRegistry& registry);
void addMpcSelfReferenceCheck(CheckRegistry& registry);
void addSpcConflictCheck(CheckRegistry& registry);
void addSetCycleCheck(CheckRegistry& registry);
void addRigidLoopCheck(CheckRegistry& registry);
void addRigidChainCheck(CheckRegistry& registry);
void addNoConstraintCheck(CheckRegistry& registry);
void addCyclicSupportOnePointCheck(CheckRegistry& registry);

// Adds the checks of the family, in the order they run.
void registerConstraintChecks(CheckRegistry& registry);

} // namespace oor::analysis
