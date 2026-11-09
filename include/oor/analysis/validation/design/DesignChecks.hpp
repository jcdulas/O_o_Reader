#pragma once

#include "oor/analysis/validation/engine/CheckRegistry.hpp"

// The design family (SOL 200 and the other design optimisation sequences): what the cards DESVAR, DLINK, DTABLE,
// DEQATN, DVPREL2, DVMREL2, DVCREL2, DRESP1, DRESP2 and DCONSTR say that the reference checks do not see. A design
// variable, an equation, a property, a material, an element or a response that does not exist is `ref.unresolved` (the
// cards say what they refer to), and a duplicated ID is `ref.duplicate-id`; the checks here are about the values:
// bounds that exclude the initial value, a label no DTABLE defines, an equation that needs more arguments than its
// relation gives, responses that contain one another in a circle. One class per check, as in the other families. The
// relation checks read the design model (`model/design`: what each relation drives, and the functions of each card type
// that say which names are fields).

namespace oor::analysis
{

// A DRESP1, DRESP2 or DRESP3 with the ID of a response of another card type.
void addResponseIdRepeatedCheck(CheckRegistry& registry);
// A DESVAR whose initial value is outside its bounds, or whose lower bound is above its upper bound.
void addVariableBoundsCheck(CheckRegistry& registry);
// A DLINK whose independent variable is the dependent variable of a DLINK (its own included).
void addLinkDependentCheck(CheckRegistry& registry);
// A label that two DTABLE or DTABLE2 entries both define.
void addConstantRepeatedCheck(CheckRegistry& registry);
// A DEQATN the library keeps as text because its equations do not parse.
void addEquationSyntaxCheck(CheckRegistry& registry);
// A DVPREL2, DVMREL2, DVCREL2 or DRESP2 that gives its DEQATN fewer values than the equation has arguments.
void addEquationArgumentsCheck(CheckRegistry& registry);
// A constant that a DVxREL2, DRESP2 or DRESP3 names and no DTABLE or DTABLE2 defines.
void addUndefinedConstantCheck(CheckRegistry& registry);
// A DVPREL2 on a property card the format reference says it does not support (PBEND, PBARL, PBEAML).
void addRelationTypeCheck(CheckRegistry& registry);
// A DRESP1 of a response type that has no region, with a REGION.
void addResponseRegionCheck(CheckRegistry& registry);
// DRESP2 cards that contain one another in a circle.
void addResponseCycleCheck(CheckRegistry& registry);
// A DCONSTR with a lower bound above its upper bound.
void addConstraintBoundsCheck(CheckRegistry& registry);
// A DCONSTR set that bounds the same response twice.
void addConstraintRepeatedCheck(CheckRegistry& registry);

// A relation that names a field the card it drives does not have or a relation cannot drive (the PNAME of a DVPREL, the
// MPNAME of a DVMREL, the CPNAME of a DVCREL), judged by the functions of `model/design`.
void addRelationTargetCheck(CheckRegistry& registry);
// A relation whose limits exclude the value it gives at the initial values of the design variables.
void addRelationBoundsCheck(CheckRegistry& registry);
// A DRESP1 whose ATTA or ATTB does not fit its type of response.
void addResponseAttributesCheck(CheckRegistry& registry);
// A SOL 200 deck with design variables and constraints that has no DESOBJ.
void addNoObjectiveCheck(CheckRegistry& registry);
// A design variable no relation, link or response uses.
void addUnusedVariableCheck(CheckRegistry& registry);

// Adds the checks of the family, in the order they run.
void registerDesignChecks(CheckRegistry& registry);

} // namespace oor::analysis
