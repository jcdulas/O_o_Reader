#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One TABLED4 card: a polynomial for dynamic loads, y = sum of Ai * ((x - X1) / X2)^i for i = 0 to N, where x is first
// brought into [X3, X4].
//
// Card layout:
//   TABLED4  TID  X1  X2  X3  X4
//            A0   A1  A2  A3  A4  A5  -etc.-  ENDT
// The coefficients follow each other from field 9, over as many continuation lines as needed, and the word ENDT ends
// the table. X2 is not zero and X3 is below X4.
struct TableDynamic4Card
{
    base::EntityId id = 0;
    double abscissaShift = 0.0; // the table parameters
    double abscissaScale = 1.0;
    double abscissaMinimum = 0.0;
    double abscissaMaximum = 1.0;
    std::vector<double> coefficients; // A0, A1, ...
};

} // namespace oor::model
