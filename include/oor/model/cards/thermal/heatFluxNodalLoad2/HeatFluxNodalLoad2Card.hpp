#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>

namespace oor::model
{

// One QBDY2 card: the heat flux at each grid point of one surface element.
//
// Card layout: QBDY2  SID  EID  Q01  Q02  Q03  Q04  Q05  Q06  Q07  Q08
// The element is a CHBDYE, CHBDYG or CHBDYP element. A blank flux is 0.0 (the format reference allows a blank):
// `heatFlux` has the eight values, and a value is written only when it is not zero.
struct HeatFluxNodalLoad2Card
{
    static constexpr std::size_t POINT_COUNT = 8;

    base::EntityId id = 0;
    base::EntityId elemId = 0;
    std::array<double, POINT_COUNT> heatFlux = {};
};

} // namespace oor::model
