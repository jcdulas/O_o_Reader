#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One PVISC1 card: the property of a nonlinear damper of SOL 700, whose force depends on the velocity through a TABLED1
// (the x values of the table are velocities, the y values forces; the values between are interpolated).
//
// Card layout:
//   PVISC1  PID  TABLE                       fields 1 and 2
// Both fields are required. Unlike the PVISC, the format shows one property in a card (not two), so a card is one row.
// The PID is a property number, in the space of every property.
struct ViscousDamperPropNonlinearCard
{
    base::EntityId id = 0;      // unique among the properties
    base::EntityId tableId = 0; // the TABLED1 with the force against the velocity
};

} // namespace oor::model
