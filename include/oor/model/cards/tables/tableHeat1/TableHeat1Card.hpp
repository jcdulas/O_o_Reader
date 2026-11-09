#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One TABLEH1 card: a function f(y) of one variable for the convection heat transfer coefficient or the Nusselt number
// that a TABLEHT refers to, given by points.
//
// Card layout:
//   TABLEH1  TID
//            y1   f1     y2     f2     y3     f3     -etc.-   ENDT
// The points follow each other from field 9, two fields each, over as many continuation lines as needed, and the word
// ENDT ends the table. y must be ascending. Between the points the function is linear; outside it is
// the value of the first or of the last point (no extrapolation). The card keeps the independent
// variable in `x` and the dependent one in `y`, as the other tables do.
//
// A pair that has SKIP in a field is not kept: the card does not keep what the program ignores.
struct TableHeat1Card
{
    base::EntityId id = 0;
    std::vector<double> x; // the independent variable (y1, y2, ... of the format reference)
    std::vector<double> y; // the dependent variable (f1, f2, ...)
};

} // namespace oor::model
