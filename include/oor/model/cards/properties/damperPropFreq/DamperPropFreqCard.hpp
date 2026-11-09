#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One PDAMPT card: the frequency dependent property of the PDAMP with the same ID.
//
// Card layout:
//   PDAMPT  PID  TBID
//
// The ID is the one of the PDAMP it extends (the card has an ID space of its own: one PDAMPT per PDAMP). TBID is the
// TABLEDi entry of the damping force per unit velocity against frequency, 0 when blank.
struct DamperPropFreqCard
{
    base::EntityId id = 0;
    base::EntityId dampingTableId = 0;
};

} // namespace oor::model
