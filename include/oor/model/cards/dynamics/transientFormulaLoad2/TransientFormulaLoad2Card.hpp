#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/dynamics/DynamicLoadFields.hpp"

namespace oor::model
{

// One TLOAD2 card: a transient excitation given by a formula, for T1 <= t <= T2 and zero elsewhere:
//   P(t) = A * t'^B * exp(C * t') * cos(2 * pi * F * t' + P),   with t' = t - T1 - tau.
//
// Card layout:
//   TLOAD2  SID  EXCITEID  DELAYI/DELAYR  TYPE  T1  T2  F  P
//           C    B         US0            VS0
// DELAYI/DELAYR holds the ID of a DELAY card (delayId) or a fixed delay (delay); both blank is no delay. EXCITEID is
// the set ID of a DAREA or of a static load set for an applied load (the set of an SPCD for an enforced motion).
//
// A blank T2 is T1 (the format reference gives no default), so T2 is stored resolved and always written.
struct TransientFormulaLoad2Card
{
    base::EntityId id = 0;
    base::EntityId excitationId = 0;
    base::EntityId delayId = 0; // DELAYI; 0 is blank
    double delay = 0.0;         // DELAYR
    ExcitationType kind = ExcitationType::Load;
    double time1 = 0.0; // the time constants
    double time2 = 0.0;
    double frequency = 0.0;                 // F, in cycles per unit time
    double phase = 0.0;                     // P, in degrees
    double coefficientC = 0.0;              // the exponential coefficient
    double exponentB = 0.0;                 // the growth coefficient
    double initialDisplacementFactor = 0.0; // factor for the initial displacements of the enforced degrees of freedom
    double initialVelocityFactor = 0.0;     // factor for their initial velocities
};

} // namespace oor::model
