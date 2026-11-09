#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One RBAR1 card: a rigid bar (alternative format): GB moves with GA in the components CB.
//
// Card layout: RBAR1  EID  GA  GB  CB  ALPHA  TREF
// CB are the components at GB that move with GA, the digits 1 to 6 as one number; a blank CB is 123456.
// ALPHA is the thermal expansion coefficient (0.0 when blank), TREF the reference temperature (0.0 when blank). The
// rigid elements share the ID space of the elements.
struct RigidBar1Card
{
    base::EntityId id = 0;
    base::EntityId nodeA = 0;
    base::EntityId nodeB = 0;
    std::int32_t componentsB = 123456;
    double alphaCoefficient = 0.0;
    double referenceTemperature = 0.0;
};

} // namespace oor::model
