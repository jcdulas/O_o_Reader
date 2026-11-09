#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One MDRJNT card: a rigid joint that connects two coinciding grid points that lie in two different modules (the
// multi-module counterpart of the RJOINT, whose rules apply). A module is a BEGIN MODULE section of the deck, a part
// of the deck; module 0 (a blank field) is the main Bulk Data.
//
// Card layout: MDRJNT  EID  MODA  GA  MODB  GB  CB
// CB are the components at GB that move with GA, the digits 1 to 6 as one number; a blank CB is 123456,
// which makes the two points one, and a smaller set makes a mechanical joint. The points must
// coincide (not checked here, the geometry decides, and the points are in modules the model does not
// hold). EID is an integer above zero (the format of the MDRJNT gives no upper limit, unlike the other cards of the
// family). The card belongs in the main Bulk Data (a rule of the deck, not of the card). a common deck has
// a stray 1 in field 8, which the layout does not use: the card refuses it.
struct ModuleRigidJointCard
{
    base::EntityId id = 0;
    base::EntityId moduleIdA = 0; // the module of GA, 0 for the main Bulk Data
    base::EntityId nodeA = 0;
    base::EntityId moduleIdB = 0; // the module of GB
    base::EntityId nodeB = 0;
    std::int32_t componentsB = 123456; // the components at GB that move with GA
};

} // namespace oor::model
