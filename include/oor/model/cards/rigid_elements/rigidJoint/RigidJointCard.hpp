#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One RJOINT card: a rigid joint connecting two coinciding grid points.
//
// Card layout: RJOINT  EID  GA  GB  CB
// CB are the components at GB that move with GA, the digits 1 to 6 as one number; a blank CB is 123456, which makes the
// two points one; a smaller set makes a mechanical joint (12345 a hinge, 1234 a universal joint, 123 a
// spherical joint). The points must coincide (not checked here, the geometry decides). The rigid
// elements share the ID space of the elements.
struct RigidJointCard
{
    base::EntityId id = 0;
    base::EntityId nodeA = 0;
    base::EntityId nodeB = 0;
    std::int32_t componentsB = 123456;
};

} // namespace oor::model
