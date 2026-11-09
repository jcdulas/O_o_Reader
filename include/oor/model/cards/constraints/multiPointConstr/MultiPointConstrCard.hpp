#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One term of a multipoint constraint equation: coefficient Aj times component Cj of point Gj.
struct MultiPointConstrTerm
{
    base::EntityId node = 0;    // Gj: a grid or scalar point
    std::int32_t component = 0; // Cj: one of 1 to 6 for a grid point, 0 (blank) for a scalar point
    double coefficient = 0.0;   // Aj

    [[nodiscard]] bool operator==(const MultiPointConstrTerm&) const noexcept = default;
};

// One MPC card: a multipoint constraint equation, the sum of Aj * u(Gj, Cj) = 0. The first term is the dependent
// degree of freedom, so its coefficient is not zero.
//
// Card layout: MPC  SID  G1  C1  A1  G2  C2  A2
//                  .    G3  C3  A3  G4  C4  A4
//                  .    G5  C5  A5  -etc.-
// The terms go two by two on a line, in the second to fourth and the fifth to seventh field of the line (the first
// field of a continuation line and the last one of every line are blank).
struct MultiPointConstrCard
{
    base::EntityId id = 0;
    std::vector<MultiPointConstrTerm> terms;
};

} // namespace oor::model
