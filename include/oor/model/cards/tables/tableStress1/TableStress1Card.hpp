#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One TABLES1 card: a function y(x) for stress dependent material properties (the stress-strain curve of a MATS1, the
// creep parameters of a CREEP, the test data of a MATHP), given by points.
//
// Card layout:
//   TABLES1  TID  TYPE
//            x1   y1     x2     y2     x3     y3     -etc.-   ENDT
// The points follow each other from field 9, two fields each, over as many continuation lines as needed, and the word
// ENDT ends the table. x is ascending or descending; a repeated x makes a jump (not between the two first or the two
// last points). Between the points the function is linear, and outside it carries on from the two first or the two last
// points. TYPE is 1 or 2 (blank is 1): the kind of stress-strain curve in SOL 400 (1 total strain, 2 plastic strain).
//
// A pair that has SKIP in a field is not kept: the card does not keep what the program ignores.
struct TableStress1Card
{
    base::EntityId id = 0;
    int kind = 1;
    std::vector<double> x;
    std::vector<double> y;
};

} // namespace oor::model
