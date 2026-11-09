#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One ACSRCE card: an acoustic source given as power against frequency (selected by DLOAD like an RLOADi card).
//
// Card layout: ACSRCE  SID  EXCITEID  DELAYI/DELAYR  DPHASEI/DPHASER  TP/RP  RHO  B
// Three fields hold an ID or a real, and each is two members here (the ID is 0 and the real 0.0 when the field is
// blank, and at most one of them is set):
//  - DELAYI/DELAYR: the ID of a DELAY card (delayId) or a fixed time delay (delay);
//  - DPHASEI/DPHASER: the ID of a DPHASE card (phaseId) or a fixed phase angle in degrees (phase);
//  - TP/RP: the ID of the TABLEDi that gives the power P(f) (powerTableId) or a constant power (power, not zero).
// RHO (the density) and B (the bulk modulus) of the fluid are positive. The EXCITEID names a DAREA set or the SLOAD set
// an LSEQ picks.
struct AcousticSourceLoadCard
{
    base::EntityId id = 0;
    base::EntityId excitationId = 0;
    base::EntityId delayId = 0;
    double delay = 0.0;
    base::EntityId phaseId = 0;
    double phase = 0.0;
    base::EntityId powerTableId = 0;
    double power = 0.0;
    double density = 0.0;
    double bulkModulus = 0.0;
};

} // namespace oor::model
