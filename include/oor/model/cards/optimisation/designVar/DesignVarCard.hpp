#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>
#include <string>

namespace oor::model
{

// One DESVAR card: a design variable of a SOL 200 optimisation.
//
// Card layout: DESVAR  ID  LABEL  XINIT  XLB  XUB  DELXV  DDVAL           fields 1 to 7
// The format reference wants XLB <= XINIT <= XUB; the card reads a value that breaks that and the check
// `design.variable-bounds` reports it (the card is not refused: a model being built passes through such a state).
// DELXV, the fraction the variable may change in one cycle, is blank when it comes from the DELX of the DOPTPRM (0.5
// without one), so it is not resolved here. DDVAL is the number of a DDVAL card that lists the discrete values of the
// variable (blank for a continuous one): a reference to a DDVAL (ReferenceTarget::DiscreteValueSet).
struct DesignVarCard
{
    static constexpr double NO_LOWER_BOUND = -1.0e20;
    static constexpr double NO_UPPER_BOUND = 1.0e20;

    base::EntityId id = 0;  // unique among the design variables
    std::string entryLabel; // for printing only: 1 to 8 characters
    double initialValue = 0.0;
    double lowerBound = NO_LOWER_BOUND;
    double upperBound = NO_UPPER_BOUND;
    std::optional<double> designVarMoveLimit; // > 0 when given
    base::EntityId discreteValuesId = 0;      // 0: continuous
};

} // namespace oor::model
