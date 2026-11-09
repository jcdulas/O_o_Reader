#include "oor/analysis/validation/case_control/CaseControlChecks.hpp"

namespace oor::analysis
{

void registerCaseControlChecks(CheckRegistry& registry)
{
    addCaseUnresolvedReferenceCheck(registry);
    addCaseUnresolvedSetCheck(registry);
    addCaseDuplicateSetCheck(registry);
    addCaseEmptySetCheck(registry);
    addCaseDuplicateCommandCheck(registry);
    addCaseContradictoryCommandCheck(registry);
    addCaseDuplicateSubcaseCheck(registry);
    addCaseCombinationWithoutFactorsCheck(registry);
    addCaseSetMemberMissingCheck(registry);
    addCaseTemperatureMaterialInitialCheck(registry);
    addCaseTemperatureMaterialNonlinearCheck(registry);
}

} // namespace oor::analysis
