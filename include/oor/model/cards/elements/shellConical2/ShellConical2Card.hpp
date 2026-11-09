#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One CCONEAX card: an axisymmetric conical shell element between two rings, with a PCONEAX property.
//
// Card layout:
//   CCONEAX  EID  PID  RA  RB
//
// RA and RB are the IDs of two different RINGAX points (the format reference: an AXIC entry must be present too).
// RINGAX is not a typed card yet, so they are not references the library resolves or renumbers.
struct ShellConical2Card
{
    base::EntityId id = 0;
    base::EntityId propId = 0; // stored resolved: a blank PID is EID
    base::EntityId ringIdA = 0;
    base::EntityId ringIdB = 0;
};

} // namespace oor::model
