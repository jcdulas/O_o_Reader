#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>

namespace oor::model
{

// One CAABSF card: a frequency-dependent acoustic absorber element on the fluid-structure boundary.
//
// Card layout: CAABSF  EID  PID  G1  G2  G3  G4
// One grid point is a point impedance, two a line impedance, three a triangle and four a quadrilateral face. A blank
// PID is EID (format reference): stored resolved and always written. `nodes` has the grid points, 0 where one is absent
// (G1 is required).
struct AcousticAbsorberCard
{
    static constexpr std::size_t NODE_COUNT = 4;

    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, NODE_COUNT> nodes = {};
};

} // namespace oor::model
