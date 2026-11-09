#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One viscous damper property of a PVISC card.
//
// Card layout (a card defines one or two properties, each of three fields, the second one after a blank field):
//   PVISC  PID1  CE1  CR1  (blank)  PID2  CE2  CR2
//
// A card with two properties gives two ViscousDamperPropCard values, each written as a card of its own. A blank CR is
// 0.0 (it is a real, and decks often leave it out).
struct ViscousDamperPropCard
{
    base::EntityId id = 0;
    double extensionalDamping = 0.0; // viscous damping for extension, force per unit velocity
    double rotationalDamping = 0.0;  // viscous damping for rotation, moment per unit velocity
};

} // namespace oor::model
