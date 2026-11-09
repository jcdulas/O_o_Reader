#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// One RTRPLT1 card: a rigid triangular plate (alternative format): GB and GC move with GA in the components CMB and
// CMC.
//
// Card layout: RTRPLT1  EID  GA  GB  GC  CMB  CMC  ALPHA  TREF
// CMB and CMC are the components at GB and GC that move with GA, each the digits 1 to 6 as one number; a blank one is
// 123456. ALPHA is the thermal expansion coefficient (0.0 when blank), TREF the reference temperature
// (0.0 when blank). The rigid elements share the ID space of the elements.
struct RigidTriaPlate1Card
{
    base::EntityId id = 0;
    std::array<base::EntityId, 3> nodes{0, 0, 0};           // GA, GB, GC
    std::array<std::int32_t, 2> components{123456, 123456}; // CMB, CMC
    double alphaCoefficient = 0.0;
    double referenceTemperature = 0.0;
};

} // namespace oor::model
