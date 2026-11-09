#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One RBAR card: a rigid bar with six degrees of freedom at each end.
//
// Card layout: RBAR  EID  GA  GB  CNA  CNB  CMA  CMB  ALPHA
//                   TREF
// CNA and CNB are the independent components at GA and GB, CMA and CMB the dependent ones, each the digits 1 to 6 as
// one number (0 when blank). Two defaults depend on other fields and are stored resolved (and always written): when CNA
// and CNB are both blank, CNA is 123456; when CMA and CMB are both blank, every component that is not
// independent at its end is dependent. ALPHA is the thermal expansion coefficient (0.0 when blank), TREF
// the reference temperature (0.0 when blank). The rigid elements share the ID space of the elements.
struct RigidBarCard
{
    base::EntityId id = 0;
    base::EntityId nodeA = 0;
    base::EntityId nodeB = 0;
    std::int32_t independentComponentsA = 0;
    std::int32_t independentComponentsB = 0;
    std::int32_t componentsAtA = 0;
    std::int32_t componentsAtB = 0;
    double alphaCoefficient = 0.0;
    double referenceTemperature = 0.0;
};

} // namespace oor::model
