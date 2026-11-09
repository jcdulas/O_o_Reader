#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// One DAREA card: scale (area) factors of a load on one or two degrees of freedom.
//
// Card layout: DAREA  SID  P1  C1  A1  P2  C2  A2
// A card holds one or two scale factors: the second group is optional, P2 is 0
// when it is absent (then C2 and A2 are 0 too). Ci is one component, 1 to 6 for a grid point and 0 for an extra or
// scalar point; Ai is the scale factor. The SID is the set ID that the EXCITEID of RLOAD and TLOAD cards refers to.
struct DynamicLoadScaleCard
{
    base::EntityId id = 0;
    std::array<base::EntityId, 2> points{0, 0};
    std::array<std::int32_t, 2> components{0, 0};
    std::array<double, 2> scaleFactors{0.0, 0.0};

    [[nodiscard]] bool hasSecondGroup() const noexcept { return points[1] != 0; }
};

} // namespace oor::model
