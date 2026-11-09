#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/dynamics/DynamicLoadFields.hpp"

namespace oor::model
{

// One RLOAD1 card: a frequency response excitation P(f) = A * (C(f) + i * D(f)) * exp(i * (phi - 2 * pi * f * tau)).
//
// Card layout: RLOAD1  SID  EXCITEID  DELAYI/DELAYR  DPHASEI/DPHASER  TC/RC  TD/RD  TYPE
// Four fields hold an ID or a real, and each is two members here (the ID is 0 and the real 0.0 when the field is blank,
// and at most one of them is set):
//  - DELAYI/DELAYR: the ID of a DELAY card (delayId) or a fixed time delay (delay);
//  - DPHASEI/DPHASER: the ID of a DPHASE card (phaseId) or a fixed phase angle in degrees (phase);
//  - TC/RC: the ID of the TABLEDi that gives C(f) (realTableId) or a constant C (realValue);
//  - TD/RD: the same for D(f) (imaginaryTableId, imaginaryValue).
// TC/RC or TD/RD may be blank, but not both. TYPE is one of the first four types (no SOL 700 type).
struct FreqResponseLoad1Card
{
    base::EntityId id = 0;
    base::EntityId excitationId =
        0; // the set ID of a DAREA or static load set (the set of an SPCD for enforced motion)
    base::EntityId delayId = 0;
    double delay = 0.0;
    base::EntityId phaseId = 0;
    double phase = 0.0;
    base::EntityId realTableId = 0;
    double realValue = 0.0;
    base::EntityId imaginaryTableId = 0;
    double imaginaryValue = 0.0;
    ExcitationType kind = ExcitationType::Load;
};

} // namespace oor::model
