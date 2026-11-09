#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// An independent degree of freedom of a transfer function: the term (A0 + A1 p + A2 p^2) of the point G, component C.
struct TransferFunctionTerm
{
    base::EntityId node = 0;
    std::int32_t components = 0; // 1 to 6 for a grid point, 0 for a scalar or extra point
    double coefficientA0 = 0.0;
    double coefficientA1 = 0.0;
    double coefficientA2 = 0.0;
};

// One TF card: a dynamic transfer function (B0 + B1 p + B2 p^2) ud + sum of (A0(i) + A1(i) p + A2(i) p^2) ui = 0, where
// ud is the dependent coordinate (GD, CD), ui the independent ones and p the derivative with respect to time (selected
// by TFL in Case Control; several TF cards share a set ID).
//
// Card layout:  TF  SID  GD  CD  B0  B1  B2
//                  G(1) C(1) A0(1) A1(1) A2(1)       (one line per independent degree of freedom, optional)
//                  -etc.-
// The lines of the terms start at field 9 and go eight fields a line (the term k at field 9 + 8 k).
struct TransferFunctionCard
{
    base::EntityId id = 0;
    base::EntityId nodeD = 0;
    std::int32_t componentsD = 0;
    double coefficientB0 = 0.0;
    double coefficientB1 = 0.0;
    double coefficientB2 = 0.0;
    std::vector<TransferFunctionTerm> terms;
};

} // namespace oor::model
