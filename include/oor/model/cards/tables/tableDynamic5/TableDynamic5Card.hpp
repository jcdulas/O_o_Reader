#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One TABLED5 card: a function f(x, y) of two variables for dynamic loads, given by a TABLED1 to TABLED4 table of y for
// each of several values of x.
//
// Card layout:
//   TABLED5  TID
//            x1  TID1  x2  TID2  x3  TID3  -etc.-   ENDT
// The pairs follow each other from field 9, two fields each, over as many continuation lines as needed, and the word
// ENDT ends the table. The x are ascending; TIDi is the ID of the TABLEDi that gives the function of y at xi. Between
// the xi the function is interpolated linearly.
struct TableDynamic5Card
{
    base::EntityId id = 0;
    std::vector<double> x;              // the first variable, ascending
    std::vector<base::EntityId> tables; // the TABLED1 to TABLED4 that give the function of y at each x
};

} // namespace oor::model
