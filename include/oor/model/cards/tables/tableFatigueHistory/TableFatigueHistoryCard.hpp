#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One TABLFTG card: the cyclic loading history of a fatigue analysis, a list of the values of the load at the points of
// its time history (an FTGLOAD names it by its TID). In a modal analysis (SOL 103) the values are the modal
// participation factors of one mode.
//
// Card layout:
//   TABLFTG  TID
//            y1   y2   y3   y4   y5   y6   y7   ENDT
// The values follow each other from field 9, on as many continuation lines as needed, and the word ENDT ends the list.
// There are no x values: the history is a sequence, and for the rainflow cycle counting that a fatigue analysis does
// only the order of the values matters, so nothing says how the points are spaced. Every value is a
// real and none may be blank (the format gives no default); at least one is needed.
//
// The TID is unique among the TABLFTG cards (IdSpace::PerCardType, the category FatigueTable that an FTGLOAD names).
struct TableFatigueHistoryCard
{
    base::EntityId id = 0;
    std::vector<double> ordinate;
};

} // namespace oor::model
