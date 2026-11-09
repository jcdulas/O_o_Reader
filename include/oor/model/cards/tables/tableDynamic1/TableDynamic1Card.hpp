#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/tables/TableSupport.hpp"

#include <vector>

namespace oor::model
{

// One TABLED1 card: a function y(x) for dynamic loads (of time or of frequency), given by points.
//
// Card layout:
//   TABLED1  TID  XAXIS  YAXIS
//            x1   y1     x2     y2     x3     y3     -etc.-   ENDT
// The points follow each other from field 9, two fields each, over as many continuation lines as needed, and the word
// ENDT ends the table. x is ascending or descending; a repeated x makes a jump. Between the points the function is
// linear, in the logarithm of x when XAXIS is LOG and in the logarithm of y when YAXIS is LOG, and outside it carries
// on from the two first or the two last points.
//
// A pair that has SKIP in a field is not kept: the card does not keep what the program ignores.
struct TableDynamic1Card
{
    base::EntityId id = 0;
    TableAxis xAxisKind = TableAxis::Linear;
    TableAxis yAxisKind = TableAxis::Linear;
    std::vector<double> x;
    std::vector<double> y;
};

} // namespace oor::model
