#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

#include <vector>

namespace oor::model
{

// One SEEXCLD card: grid points of a part superelement that are left out of its connection to another superelement, or
// to all the others.
//
// Card layout: SEEXCLD  SEIDA  SEIDB  GIDA1  GIDA2  GIDA3  GIDA4  GIDA5  GIDA6
//                      GIDA7  GIDA8  -etc.-
// or:         SEEXCLD  SEIDA  SEIDB  GIDA1  "THRU"  GIDA2
// The layout is that of SEBNDRY. SEIDA is the superelement whose points the list names (the points are those of that
// part, which has its own ID space); SEIDB is another superelement or the word ALL, which is also what a blank SEIDB
// means (kept as ALL, no number). SEIDA and SEIDB name parts or the residual structure, the card belongs in
// the main Bulk Data and is a fatal error after a BEGIN SUPER, and it works only when the deck has part
// superelements. The points are kept as written: single points and THRU ranges, in order.
struct SuperelementExcludedNodesCard
{
    base::EntityId superelementIdA = 0;
    SuperelementOrAll superelementIdB; // nothing is ALL
    std::vector<IdRun> grids;

    [[nodiscard]] bool operator==(const SuperelementExcludedNodesCard&) const noexcept = default;
};

} // namespace oor::model
