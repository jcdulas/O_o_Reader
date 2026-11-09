#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One PELAST card: the frequency dependent or nonlinear properties of the PELAS with the same ID.
//
// Card layout:
//   PELAST  PID  TKID  TGEID  TKNID
//
// The ID is the one of the PELAS it extends (the card has an ID space of its own: one PELAST per PELAS). The three
// others are TABLEDi entries, 0 when blank: the force per unit displacement against frequency, the structural damping
// against frequency, and the nonlinear force against displacement.
struct SpringPropFreqCard
{
    base::EntityId id = 0;
    base::EntityId stiffnessTableId = 0;
    base::EntityId structuralDampingTableId = 0;
    base::EntityId nonlinearForceTableId = 0;
};

} // namespace oor::model
