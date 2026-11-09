#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One DCONSTR card: the bounds of one response, in a set of design constraints.
//
// Card layout: DCONSTR  DCID  RID  LALLOW/LID  UALLOW/UID  LOWFQ  HIGHFQ          fields 1 to 6
// A bound is a number, or (written as an integer) the number of a TABLEDi that gives it as a function of frequency, for
// the frequency responses; the card then keeps the table in LID or UID and the number stays at its default. LALLOW and
// UALLOW are the bounds, -1.0E20 and 1.0E20 when blank, and LOWFQ and HIGHFQ the range of frequencies the bounds apply
// to. The set DCID is selected by DESSUB or DESGLB, or named by a DCONADD; a response appears in it once.
struct DesignConstrCard
{
    static constexpr double NO_LOWER_BOUND = -1.0e20;
    static constexpr double NO_UPPER_BOUND = 1.0e20;
    static constexpr double NO_HIGH_FREQUENCY = 1.0e20;

    base::EntityId constraintSetId = 0; // the set of constraints (shared by the cards of the set)
    base::EntityId responseId = 0;      // the DRESP1, DRESP2 or DRESP3 that is bounded
    double lowerAllowable = NO_LOWER_BOUND;
    base::EntityId lowerAllowableTableId = 0; // a TABLEDi of lower bounds, instead of LALLOW
    double upperAllowable = NO_UPPER_BOUND;
    base::EntityId upperAllowableTableId = 0; // a TABLEDi of upper bounds, instead of UALLOW
    double lowFrequency = 0.0;
    double highFrequency = NO_HIGH_FREQUENCY;
};

} // namespace oor::model
