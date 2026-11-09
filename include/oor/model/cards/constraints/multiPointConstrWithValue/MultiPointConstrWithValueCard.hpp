#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One independent term of a non-homogeneous multipoint constraint: coefficient Ai times component Ci of point Gi.
struct MultiPointConstrWithValueTerm
{
    base::EntityId node = 0;    // Gi: a grid or scalar point
    std::int32_t component = 0; // Ci: one of 1 to 6 for a grid point, 0 (blank) for a scalar point
    double coefficient = 0.0;   // Ai: the format reference default is 0.0

    [[nodiscard]] bool operator==(const MultiPointConstrWithValueTerm&) const noexcept = default;
};

// One MPCY card: the multipoint constraint equation  Am * u(Gm, Cm) + sum of Ai * u(Gi, Ci) = Ym, which is the MPC with
// a right hand side. The first group is the dependent degree of freedom (an equation marker: with PARAM,AUTOMSET,YES
// The solver may choose another one), then come the independent terms.
//
// Card layout: MPCY  SID  GM  CM  AM  YM
//                   .    G1  C1  A1  G2  C2  A2
//                   .    G3  C3  A3  -etc.-
// The independent terms go two by two on a continuation line, in the second to fourth and the fifth to seventh field of
// the line (the first field of the line and the last one are blank), as the terms of an MPC do: the first term is in
// field 10 of the card, the third in field 18. The coefficient AM is not zero; YM is 0.0 when blank; a coefficient Ai
// of a blank field is 0.0. A card may hold no independent term (Am * um = Ym). The component of a scalar point is blank
// or 0 and the card keeps 0; the format reference also lets the dependent term of a scalar point be 1, which is kept as
// it is. The SID is the number the MPC Case Control command selects, so it is in the constraint category, and an MPCD
// of the same SID in the load category gives the right hand side a value of a load step.
struct MultiPointConstrWithValueCard
{
    base::EntityId id = 0;
    base::EntityId node = 0;    // GM
    std::int32_t component = 0; // CM
    double coefficient = 0.0;   // AM
    double value = 0.0;         // YM
    std::vector<MultiPointConstrWithValueTerm> terms;
};

} // namespace oor::model
