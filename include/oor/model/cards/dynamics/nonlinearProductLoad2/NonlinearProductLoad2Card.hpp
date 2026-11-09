#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One NOLIN2 card: a nonlinear load that is the product of two variables (selected by the Case Control command
// NONLINEAR).
//
// Card layout: NOLIN2  SID  GI  CI  S  GJ  CJ  GK  CK
// The load at point GI, component CI, is S times XJ times XK, where XJ is the displacement of point GJ, component CJ,
// and XK that of point GK, component CK (each a velocity when its component is 10 more than the component: 11 to 16 for
// a grid point, 10 for a scalar or extra point). The components are 0 (blank) for a scalar or extra point. The three
// points may be the same.
struct NonlinearProductLoad2Card
{
    base::EntityId id = 0;
    base::EntityId nodeI = 0;
    std::int32_t componentsI = 0;
    double scaleFactor = 0.0;
    base::EntityId nodeJ = 0;
    std::int32_t componentsJ = 0;
    base::EntityId nodeK = 0;
    std::int32_t componentsK = 0;
};

} // namespace oor::model
