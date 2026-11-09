#include "oor/analysis/validation/design/DesignChecks.hpp"

namespace oor::analysis
{

void registerDesignChecks(CheckRegistry& registry)
{
    addResponseIdRepeatedCheck(registry);
    addVariableBoundsCheck(registry);
    addLinkDependentCheck(registry);
    addConstantRepeatedCheck(registry);
    addEquationSyntaxCheck(registry);
    addEquationArgumentsCheck(registry);
    addUndefinedConstantCheck(registry);
    addRelationTypeCheck(registry);
    addResponseRegionCheck(registry);
    addResponseCycleCheck(registry);
    addConstraintBoundsCheck(registry);
    addConstraintRepeatedCheck(registry);
    addRelationTargetCheck(registry);
    addRelationBoundsCheck(registry);
    addResponseAttributesCheck(registry);
    addNoObjectiveCheck(registry);
    addUnusedVariableCheck(registry);
}

} // namespace oor::analysis
