#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One TABLEM3 card: a function of (x - X1) / X2 for material properties, given by points: y = yT((x - X1) / X2).
//
// Card layout:
//   TABLEM3  TID  X1  X2
//            x1   y1  x2   y2   x3   y3   -etc.-   ENDT
// The points follow each other from field 9, two fields each, over as many continuation lines as needed, and the word
// ENDT ends the table. x is ascending or descending; a repeated x makes a jump. The function is linear between the
// points and carries on from the two first or the two last points outside. X2 is not zero.
//
// A negative TID makes the table a thermal strain table rather than a coefficient of thermal expansion table for the
// MATT1, MATT2 and MATT8 cards that refer to it: the ID is kept as written, and it is not zero.
// The format reference multiplies the function by a factor z that comes from the MAT entry that uses the table; the
// table alone is the function without it.
//
// A pair that has SKIP in a field is not kept: the card does not keep what the program ignores.
struct TableMaterial3Card
{
    base::EntityId id = 0;
    double abscissaShift = 0.0; // the table parameters
    double abscissaScale = 1.0;
    std::vector<double> x;
    std::vector<double> y;
};

} // namespace oor::model
