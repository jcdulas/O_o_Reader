#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One EOSJWL card (SOL 700): the Jones-Wilkins-Lee equation of state of the detonation products of a high explosive,
// with an optional afterburning term. A MATDEUL names it in its EID field, and a DETSPH gives the detonation model.
//
// Card layout: EOSJWL  EID  A  B  R1  R2  OMEGA  P0  Q            fields 1 to 8
//                     a  m  N                                     fields 9 to 11
// The grid of the format calls the first field MID and its list of fields calls it EID: it is the number of the
// equation of state. The format gives every constant as a real with the default 0.0, and Q as zero or more; the format
// says the constants Q, a, m and n are zero by default. The card reads them all as reals that are zero when blank and
// gives no range except Q >= 0. The afterburning rate is a (1 - lambda)^m p^n, so field 9 and field 2 are both an "a"
// of the format reference: the first is the constant A of the pressure (`a`), the second the rate coefficient (`aBurn`,
// `a_burn` in Python and A_BURN in the messages). Afterburning takes place only when Q, a, m and n are all positive;
// that is a rule of the model, the card accepts any combination.
struct JonesWilkinsLeeStateEquationCard
{
    base::EntityId id = 0;     // unique among the equations of state
    double coefficientA = 0.0; // constants A, B, R1, R2 and omega of the pressure
    double coefficientB = 0.0;
    double coefficientR1 = 0.0;
    double coefficientR2 = 0.0;
    double omega = 0.0;
    double initialPressure = 0.0; // initial pressure
    double afterburnEnergy = 0.0; // afterburning energy constant, not negative
    double afterburnRate = 0.0;   // afterburning rate coefficient (the second line's a)
    double coefficientM = 0.0;    // afterburning exponent of (1 - lambda)
    double coefficientN = 0.0;    // afterburning exponent of the pressure
};

} // namespace oor::model
