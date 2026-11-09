#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One TABLEM4 card: a polynomial for material properties, y = sum of Ai * ((x - X1) / X2)^i for i = 0 to N, where x is
// first brought into [X3, X4].
//
// Card layout:
//   TABLEM4  TID  X1  X2  X3  X4
//            A0   A1  A2  A3  A4  A5  -etc.-  ENDT
// The coefficients follow each other from field 9, over as many continuation lines as needed, and the word ENDT ends
// the table. X2 is not zero and X3 is below X4.
//
// A negative TID makes the table a thermal strain table rather than a coefficient of thermal expansion table for the
// MATT1, MATT2 and MATT8 cards that refer to it: the ID is kept as written, and it is not zero.
// The format reference multiplies the function by a factor z that comes from the MAT entry that uses the table; the
// table alone is the function without it.
struct TableMaterial4Card
{
    base::EntityId id = 0;
    double abscissaShift = 0.0; // the table parameters
    double abscissaScale = 1.0;
    double abscissaMinimum = 0.0;
    double abscissaMaximum = 1.0;
    std::vector<double> coefficients; // A0, A1, ...
};

} // namespace oor::model
