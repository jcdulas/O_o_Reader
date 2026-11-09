#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// One SPCD card: the value of an enforced motion (displacement, velocity or acceleration) of one or two grid points.
//
// Card layout: SPCD  SID  G1  C1  D1  G2  C2  D2
// The second group is optional: G2 is 0 when it is absent (then C2 and D2 are 0 too). Ci holds the digits of the
// components as one number (123 for "123", 0 for a scalar point); Di is the value of the enforced motion, 0.0 when
// blank. SID is selected by the LOAD Case Control command (static analysis) or by the EXCITEID of a dynamic load:
// it is a load set ID, like the SID of a FORCE, and the card is in the Load category.
struct EnforcedMotionConstrCard
{
    base::EntityId id = 0;
    std::array<base::EntityId, 2> nodes{0, 0};
    std::array<std::int32_t, 2> components{0, 0};
    std::array<double, 2> enforced{0.0, 0.0};

    [[nodiscard]] bool hasSecondGroup() const noexcept { return nodes[1] != 0; }
};

} // namespace oor::model
