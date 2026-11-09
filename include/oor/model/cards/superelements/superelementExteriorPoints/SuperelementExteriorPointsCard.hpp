#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <vector>

namespace oor::model
{

// One CSUPEXT card: the exterior points of a primary superelement.
//
// Card layout: CSUPEXT  SEID  GP1  GP2  GP3  GP4  GP5  GP6  GP7
//                      GP8  -etc.-
// or:         CSUPEXT  SEID  GP1  "THRU"  GP2
// The points are kept as written: single points and THRU ranges, in order. A grid or scalar point is exterior to a
// superelement when structural, rigid or plot elements connect it, and an MPC does not count: this card
// gives the connectivity the elements do not. The points are those of the downstream superelement or the residual
// structure, that is, of the main Bulk Data; scalar points may be interior to the residual structure (SEID 0) only.
// The card is for primary superelements: CSUPER is the card of the secondary ones. SEID is the number of
// the superelement it defines (IdSpace::Group).
struct SuperelementExteriorPointsCard
{
    base::EntityId superelementId = 0;
    std::vector<IdRun> points;

    [[nodiscard]] bool operator==(const SuperelementExteriorPointsCard&) const noexcept = default;
};

} // namespace oor::model
