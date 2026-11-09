#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One NOLIN1 card: a nonlinear load that is a tabular function of a displacement or a velocity (selected by the Case
// Control command NONLINEAR).
//
// Card layout: NOLIN1  SID  GI  CI  S  GJ  CJ  TID
// The load at point GI, component CI, is S times the table TID of the displacement of point GJ, component CJ (a
// velocity when CJ is 10 more than the component: 11 to 16 for a grid point, 10 for a scalar or extra point). CI and CJ
// are 0 (blank) for a scalar or extra point.
struct NonlinearTableLoad1Card
{
    base::EntityId id = 0;
    base::EntityId nodeI = 0;
    std::int32_t componentsI = 0;
    double scaleFactor = 0.0;
    base::EntityId nodeJ = 0;
    std::int32_t componentsJ = 0;
    base::EntityId tableId = 0; // a TABLEDi
};

} // namespace oor::model
