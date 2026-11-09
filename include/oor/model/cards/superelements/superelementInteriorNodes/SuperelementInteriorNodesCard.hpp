#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <vector>

namespace oor::model
{

// One SESET card: the interior grid points of a (list) superelement.
//
// Card layout: SESET  SEID  G1  G2  G3  G4  G5  G6  G7
//                    G8  G9  -etc.-
// or:         SESET  SEID  G1  "THRU"  G2
// The points are kept as written: single points and THRU ranges, in order. SEID names a primary superelement; the card
// is the main way of defining one (the other is the SEID field of the GRID). A grid point appears once in all the SESET
// cards of a deck (a check between cards), and the solver does not report a point that no card defines,
// so the references of the list may name nothing: the model leaves them to a check
// (`superelement.superelement-interior-nodes-missing-node`). All the degrees of freedom of the points are interior (the
// o-set) to the superelement. SESET belongs in the main Bulk Data and is ignored inside a part: the points
// are those of the residual structure.
struct SuperelementInteriorNodesCard
{
    base::EntityId superelementId = 0;
    std::vector<IdRun> grids;

    [[nodiscard]] bool operator==(const SuperelementInteriorNodesCard&) const noexcept = default;
};

} // namespace oor::model
