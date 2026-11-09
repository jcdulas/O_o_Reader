#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

#include <vector>

namespace oor::model
{

// One SEBNDRY card: grid points of a part superelement for the automatic search of the boundary with another
// superelement, or with all the others.
//
// Card layout: SEBNDRY  SEIDA  SEIDB  GIDA1  GIDA2  GIDA3  GIDA4  GIDA5  GIDA6
//                      GIDA7  GIDA8  -etc.-
// or:         SEBNDRY  SEIDA  SEIDB  GIDA1  "THRU"  GIDA2
// SEIDA is the partitioned superelement whose points the list names (the points are those of that part, which has its
// own ID space); SEIDB is another superelement or the word ALL, which is also what a blank SEIDB means (it is kept as
// ALL, no number). The points are kept as written: single points and THRU ranges, in order. The card belongs in the
// main Bulk Data and is a fatal error for a superelement whose METHOD is MANUAL.
struct SuperelementBoundarySearchCard
{
    base::EntityId superelementIdA = 0;
    SuperelementOrAll superelementIdB; // nothing is ALL
    std::vector<IdRun> grids;

    [[nodiscard]] bool operator==(const SuperelementBoundarySearchCard&) const noexcept = default;
};

} // namespace oor::model
