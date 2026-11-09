#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One NOLIN4 card: a nonlinear load that is a negative variable raised to a power (selected by the Case
// Control command NONLINEAR).
//
// Card layout: NOLIN4  SID  GI  CI  S  GJ  CJ  A
// The load at point GI, component CI, is minus S times (-X) raised to the power A for X < 0 and 0 otherwise, where X
// is the displacement of point GJ, component CJ (a velocity when CJ is 10 more than the component: 11 to 16 for a grid
// point, 10 for a scalar or extra point). CI and CJ are 0 (blank) for a scalar or extra point. NOLIN3 is the positive
// range.
struct NonlinearNegativePowerLoad4Card
{
    base::EntityId id = 0;
    base::EntityId nodeI = 0;
    std::int32_t componentsI = 0;
    double scaleFactor = 0.0;
    base::EntityId nodeJ = 0;
    std::int32_t componentsJ = 0;
    double exponent = 0.0; // the exponent
};

} // namespace oor::model
