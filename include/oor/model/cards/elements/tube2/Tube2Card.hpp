#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One CTUBE card: a tension-compression-torsion tube element between two grid points.
//
// Card layout:
//   CTUBE  EID  PID  G1  G2
struct Tube2Card
{
    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, 2> nodes = {};
};

} // namespace oor::model
