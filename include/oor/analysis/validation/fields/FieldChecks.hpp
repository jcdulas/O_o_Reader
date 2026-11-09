#pragma once

#include "oor/analysis/validation/engine/CheckRegistry.hpp"

// Family 2: required fields and value rules. The cards refuse bad values when they are read or edited; these are the
// rules that need more than one card, or the geometry, or a field that is only wrong in the company of another.

namespace oor::analysis
{

void addShellNoMaterialCheck(CheckRegistry& registry);
void addShellPropShearWithoutBendingCheck(CheckRegistry& registry);
void addShellPropCouplingCheck(CheckRegistry& registry);
void addOffsetNeedsMaterialsCheck(CheckRegistry& registry);
void addBarOrientationCheck(CheckRegistry& registry);
void addPointMassInertiaNoMassCheck(CheckRegistry& registry);
void addParameterKindCheck(CheckRegistry& registry);
void addParameterValueCheck(CheckRegistry& registry);
void addParameterRepeatedCheck(CheckRegistry& registry);

// Adds the checks of the family, in the order they run.
void registerFieldChecks(CheckRegistry& registry);

} // namespace oor::analysis
