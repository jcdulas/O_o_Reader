#include "oor/analysis/validation/constraints/ConstraintChecks.hpp"

namespace oor::analysis
{

void registerConstraintChecks(CheckRegistry& registry)
{
    addSinglePointConstrListMissingNodeCheck(registry);
    addSinglePointConstrListEmptyRangeCheck(registry);
    addSpcOnRigidDependentCheck(registry);
    addRigidDependentTwiceCheck(registry);
    addMpcDependentTwiceCheck(registry);
    addSpcOnMpcDependentCheck(registry);
    addMpcSelfReferenceCheck(registry);
    addSpcConflictCheck(registry);
    addSetCycleCheck(registry);
    addRigidLoopCheck(registry);
    addRigidChainCheck(registry);
    addNoConstraintCheck(registry);
    addCyclicSupportOnePointCheck(registry);
}

} // namespace oor::analysis
