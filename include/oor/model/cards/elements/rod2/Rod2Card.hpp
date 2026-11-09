#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One CROD card: a rod element between two grid points.
//
// Card layout:
//   CROD  EID  PID  G1  G2
struct Rod2Card
{
    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, 2> nodes = {};
};

} // namespace oor::model
