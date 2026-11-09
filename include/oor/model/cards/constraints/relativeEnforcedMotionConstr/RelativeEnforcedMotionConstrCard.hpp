#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// One SPCR card (SOL 400): the value of an enforced relative motion of one or two grid points, taken with respect to
// the position they had at the previous load step. It is the companion of the SPCD, which gives the total motion.
//
// Card layout: SPCR  SID  G1  C1  D1  G2  C2  D2
// The second group is optional: G2 is 0 when it is absent (then C2 and D2 are 0 too). Ci holds the digits of the
// components as one number (123 for "123"; the format reference says a blank or 0 is the same as 1, which matters for a
// scalar point, and the card keeps the 0 it reads); Di is the value of the enforced motion, 0.0 when blank (the format
// reference gives it no default). SID is selected by the LOAD Case Control command in the static solutions,
// so it is a load set ID like the SID of an SPCD: an SPCR and an SPCD may have the same SID but not the same degree of
// freedom.
struct RelativeEnforcedMotionConstrCard
{
    base::EntityId id = 0;
    std::array<base::EntityId, 2> nodes{0, 0};
    std::array<std::int32_t, 2> components{0, 0};
    std::array<double, 2> enforced{0.0, 0.0};

    [[nodiscard]] bool hasSecondGroup() const noexcept { return nodes[1] != 0; }
};

} // namespace oor::model
