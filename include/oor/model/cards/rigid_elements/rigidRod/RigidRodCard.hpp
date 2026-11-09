#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One RROD card: a pin-ended element that is rigid in translation.
//
// Card layout: RROD  EID  GA  GB  CMA  CMB  ALPHA  TREF
// CMA and CMB name the one dependent translational component (1, 2 or 3), at GA or at GB: one of them holds it and the
// other is blank (0). With the Lagrange method both may be blank. ALPHA
// is the thermal expansion coefficient (0.0 when blank), TREF the reference temperature (0.0 when blank).
// The rigid elements share the ID space of the elements.
struct RigidRodCard
{
    base::EntityId id = 0;
    base::EntityId nodeA = 0;
    base::EntityId nodeB = 0;
    std::int32_t componentsAtA = 0;
    std::int32_t componentsAtB = 0;
    double alphaCoefficient = 0.0;
    double referenceTemperature = 0.0;
};

} // namespace oor::model
