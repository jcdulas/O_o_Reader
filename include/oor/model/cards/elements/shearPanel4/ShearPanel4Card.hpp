#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One CSHEAR card: a shear panel element with four corner grid points.
//
// Card layout:
//   CSHEAR  EID  PID  G1  G2  G3  G4
//
// The grid points are listed in order around the perimeter of the panel.
struct ShearPanel4Card
{
    base::EntityId id = 0;
    base::EntityId propId = 0; // stored resolved: a blank PID is EID
    std::array<base::EntityId, 4> nodes = {};
};

} // namespace oor::model
