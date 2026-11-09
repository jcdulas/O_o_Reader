#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One AEFACT card: a list of real numbers that an aerodynamic card names by its SID. What the numbers mean depends on
// the card that names the list: the division points of a CAERO (fractions of the span or the chord, one more than the
// number of boxes), the half-widths of a body (PAERO2), the (x, y) pairs of a Mach box (CAERO3), the angles, Mach
// numbers and deflections of a schedule (CSSCHD), the lift curve slopes of a strip (PAERO4).
//
// Card layout: AEFACT  SID  D1  D2  D3  D4  D5  D6  D7
//                     D8  D9  -etc.-
// The numbers run from field 2 to the last field used, with no blank between them. At least one is given.
struct AeroFactorListCard
{
    base::EntityId id = 0;
    std::vector<double> values;
};

} // namespace oor::model
