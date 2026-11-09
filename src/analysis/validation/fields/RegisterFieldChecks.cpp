#include "oor/analysis/validation/fields/FieldChecks.hpp"

namespace oor::analysis
{

void registerFieldChecks(CheckRegistry& registry)
{
    addShellNoMaterialCheck(registry);
    addShellPropShearWithoutBendingCheck(registry);
    addShellPropCouplingCheck(registry);
    addOffsetNeedsMaterialsCheck(registry);
    addBarOrientationCheck(registry);
    addPointMassInertiaNoMassCheck(registry);
    addParameterKindCheck(registry);
    addParameterValueCheck(registry);
    addParameterRepeatedCheck(registry);
}

} // namespace oor::analysis
