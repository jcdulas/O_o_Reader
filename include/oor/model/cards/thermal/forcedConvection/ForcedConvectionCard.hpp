#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One CONVM card: the forced convection boundary condition of a CHBDYP surface element of type FTUBE.
//
// Card layout: CONVM  EID  PCONID  FLMND  CNTMDOT  TA1  TA2  MDOT
// EID is the CHBDYP element, PCONID the PCONVM. FLMND (the film temperature point) and CNTMDOT (the point that holds
// the mass flow rate, or the ID of a CONTRLT thermostat) are 0 when blank. TA1 is the ambient point and a blank TA2 is
// TA1 (stored resolved, always written). MDOT is the multiplier of the mass flow rate: 1.0 when blank, and it must be
// positive when CNTMDOT is 0.
struct ForcedConvectionCard
{
    base::EntityId id = 0;
    base::EntityId convectionPropId = 0;
    base::EntityId filmNode = 0;
    base::EntityId massFlowNode = 0;
    base::EntityId ambientNode1 = 0;
    base::EntityId ambientNode2 = 0;
    double massFlowFactor = 1.0;
};

} // namespace oor::model
