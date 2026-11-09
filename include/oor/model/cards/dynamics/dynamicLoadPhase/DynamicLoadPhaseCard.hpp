#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// One DPHASE card: phase leads of a dynamic load on one or two degrees of freedom.
//
// Card layout: DPHASE  SID  P1  C1  TH1  P2  C2  TH2
// A card holds one or two phase leads: the second group is optional, P2 is 0 when it is absent (then C2 and
// TH2 are 0 too). Ci is one component, 1 to 6 for a grid point and 0 for an extra or scalar point; THi is the phase
// lead in degrees. The SID is the set ID that the DPHASE field of RLOAD1, RLOAD2 and ACSRCE cards refers to.
struct DynamicLoadPhaseCard
{
    base::EntityId id = 0;
    std::array<base::EntityId, 2> points{0, 0};
    std::array<std::int32_t, 2> components{0, 0};
    std::array<double, 2> phaseAngles{0.0, 0.0};

    [[nodiscard]] bool hasSecondGroup() const noexcept { return points[1] != 0; }
};

} // namespace oor::model
