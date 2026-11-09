#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/tables/TableSupport.hpp"

#include <vector>

namespace oor::model
{

// One TABLEM1 card: a function y(x) for material properties (of temperature), given by points.
//
// Card layout:
//   TABLEM1  TID  XAXIS  YAXIS
//            x1   y1     x2     y2     x3     y3     -etc.-   ENDT
// The points follow each other from field 9, two fields each, over as many continuation lines as needed, and the word
// ENDT ends the table. x is ascending or descending; a repeated x makes a jump. Between the points the function is
// linear, in the logarithm of x when XAXIS is LOG and in the logarithm of y when YAXIS is LOG, and outside it carries
// on from the two first or the two last points.
//
// A negative TID makes the table a thermal strain table rather than a coefficient of thermal expansion table for the
// MATT1, MATT2 and MATT8 cards that refer to it: the ID is kept as written, and it is not zero.
// XAXIS and YAXIS are only used for S-N curves; the card keeps them in any case.
//
// A pair that has SKIP in a field is not kept: the card does not keep what the program ignores.
struct TableMaterial1Card
{
    base::EntityId id = 0;
    TableAxis xAxisKind = TableAxis::Linear;
    TableAxis yAxisKind = TableAxis::Linear;
    std::vector<double> x;
    std::vector<double> y;
};

} // namespace oor::model
