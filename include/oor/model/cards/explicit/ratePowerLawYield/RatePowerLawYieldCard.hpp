#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// The default of C, the minimum yield stress.
inline constexpr double RATE_POWER_LAW_YIELD_DEFAULT_MINIMUM = 1.0E20;

// One YLDRPL card (SOL 700): the rate power law yield model. The yield stress depends on the effective plastic strain p
// and on the effective strain rate r through a power of each: y = max(C, A + B p^N r^M). A MATDEUL names the model in
// its YID field.
//
// Card layout: YLDRPL  YID  A  B  N  M  C            fields 1 to 6
// A is the initial yield stress (greater than zero, required); B (0.0) is the hardening parameter, N (1.0) the
// hardening exponent, M (1.0) the strain rate exponent and C (1.E20) the minimum yield stress. The format reference
// gives no sign for B, N, M and C, so the card only asks for finite numbers. The grid writes N and M in lower case and
// their description in upper case; the card reads and reports them as N and M.
struct RatePowerLawYieldCard
{
    base::EntityId id = 0;                                      // unique among the yield models
    double coefficientA = 0.0;                                  // initial yield stress, greater than zero
    double coefficientB = 0.0;                                  // hardening parameter
    double coefficientN = 1.0;                                  // hardening exponent
    double coefficientM = 1.0;                                  // strain rate exponent
    double coefficientC = RATE_POWER_LAW_YIELD_DEFAULT_MINIMUM; // minimum yield stress
};

} // namespace oor::model
