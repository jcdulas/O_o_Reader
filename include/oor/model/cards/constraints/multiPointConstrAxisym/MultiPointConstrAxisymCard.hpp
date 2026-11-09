#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One term of an MPCAX equation: coefficient Aj times component Cj of ring RIDj at harmonic HIDj.
struct MultiPointConstrAxisymTerm
{
    base::EntityId ringId = 0;  // RIDj: a RINGAX point
    std::int32_t harmonic = 0;  // HIDj
    std::int32_t component = 0; // Cj: one of 1 to 6
    double coefficient = 0.0;   // Aj

    [[nodiscard]] bool operator==(const MultiPointConstrAxisymTerm&) const noexcept = default;
};

// One MPCAX card: a multipoint constraint equation of a conical shell, the sum of Aj * u(RIDj, HIDj, Cj) = 0. The first
// term is the dependent degree of freedom, so its coefficient is not zero.
//
// Card layout: MPCAX  SID  .  .  .  RID1  HID1  C1  A1
//                    RID2  HID2  C2  A2  -etc.-
// The terms are four fields each, from field 5 on (fields 2 to 4 are blank), continuing over the continuation lines.
// HID may be 0 (the format reference asks for a positive one, but harmonic 0 exists).
struct MultiPointConstrAxisymCard
{
    base::EntityId id = 0;
    std::vector<MultiPointConstrAxisymTerm> terms;
};

} // namespace oor::model
