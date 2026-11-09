#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/dynamics/DynamicLoadFields.hpp"

namespace oor::model
{

// One RLOAD2 card: a frequency response excitation P(f) = A * B(f) * exp(i * (phi(f) + phase - 2 * pi * f * tau)).
//
// Card layout: RLOAD2  SID  EXCITEID  DELAYI/DELAYR  DPHASEI/DPHASER  TB/RB  TP/RP  TYPE
// Four fields hold an ID or a real, and each is two members here (the ID is 0 and the real 0.0 when the field is blank,
// and at most one of them is set):
//  - DELAYI/DELAYR: the ID of a DELAY card (delayId) or a fixed time delay (delay);
//  - DPHASEI/DPHASER: the ID of a DPHASE card (phaseId) or a fixed phase angle in degrees (phase);
//  - TB/RB: the ID of the TABLEDi that gives B(f) (amplitudeTableId) or a constant B (amplitude), which is not zero;
//  - TP/RP: the ID of the TABLEDi that gives phi(f), in degrees (phaseTableId), or a constant phi (phaseAngle).
// TYPE is one of the first four types (no SOL 700 type).
struct FreqResponseLoad2Card
{
    base::EntityId id = 0;
    base::EntityId excitationId =
        0; // the set ID of a DAREA or static load set (the set of an SPCD for enforced motion)
    base::EntityId delayId = 0;
    double delay = 0.0;
    base::EntityId phaseId = 0;
    double phase = 0.0;
    base::EntityId amplitudeTableId = 0;
    double amplitude = 0.0;
    base::EntityId phaseTableId = 0;
    double phaseAngle = 0.0;
    ExcitationType kind = ExcitationType::Load;
};

} // namespace oor::model
