#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One TABLEHT card: the heat transfer coefficient or the Nusselt number as a function of two variables, as a series of
// TABLEH1 tables, each for one value of the first variable. A PCONV with free convection refers to it.
//
// Card layout:
//   TABLEHT  TID
//            x1    TID1   x2    TID2   x3    TID3   -etc.-   ENDT
// The pairs follow each other from field 9, two fields each, over as many continuation lines as needed, and the word
// ENDT ends the table. The values of x are ascending.
struct TableHeatTwoVariablesCard
{
    base::EntityId id = 0;
    std::vector<double> x;              // x1, x2, ...: the values of the first variable
    std::vector<base::EntityId> tables; // TID1, TID2, ...: the TABLEH1 of each value
};

} // namespace oor::model
