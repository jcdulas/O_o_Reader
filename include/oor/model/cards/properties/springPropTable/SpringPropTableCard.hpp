#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One PELAS1 card: the property of a spring of SOL 700 whose force-deflection curve is a TABLED1.
//
// Card layout:
//   PELAS1  PID  TID                         fields 1 and 2
// Both fields are required: the card has no default for either. The PID is a property number, unique among the PELAS
// and the PELAS1, which is the property space of every property type; the format says that only a CELAS1D
// may name the property, a rule of the model and not of the card. Unlike a PELAST, a PELAS1 has no PELAS
// that it extends.
struct SpringPropTableCard
{
    base::EntityId id = 0;      // unique among the properties
    base::EntityId tableId = 0; // the TABLED1 with the force against the deflection
};

} // namespace oor::model
