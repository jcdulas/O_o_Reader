#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One TABLED2 card: a function of x - X1 for dynamic loads, given by points: y = yT(x - X1).
//
// Card layout:
//   TABLED2  TID  X1
//            x1   y1   x2   y2   x3   y3   -etc.-   ENDT
// The points follow each other from field 9, two fields each, over as many continuation lines as needed, and the word
// ENDT ends the table. x is ascending or descending; a repeated x makes a jump. The function is linear between the
// points and carries on from the two first or the two last points outside. A blank X1 is 0.0 (the format reference
// gives no default).
//
// A pair that has SKIP in a field is not kept: the card does not keep what the program ignores.
struct TableDynamic2Card
{
    base::EntityId id = 0;
    double abscissaShift = 0.0; // the table parameter: the value of x where the points' own x is 0
    std::vector<double> x;
    std::vector<double> y;
};

} // namespace oor::model
