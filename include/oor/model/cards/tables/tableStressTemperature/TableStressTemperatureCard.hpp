#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One TABLEST card: the stress dependent tables of a nonlinear elastic material at several temperatures. A MATS1 of
// type NLELAST refers to it, and it refers to the TABLES1 cards that hold the stress-strain curve at each temperature.
//
// Card layout:
//   TABLEST  TID
//            T1    TID1   T2    TID2   T3    TID3   -etc.-   ENDT
// The pairs follow each other from field 9, two fields each, over as many continuation lines as needed, and the word
// ENDT ends the table. The temperatures are ascending.
struct TableStressTemperatureCard
{
    base::EntityId id = 0;
    std::vector<double> temperatures;   // T1, T2, ...
    std::vector<base::EntityId> tables; // TID1, TID2, ...: the TABLES1 of each temperature
};

} // namespace oor::model
