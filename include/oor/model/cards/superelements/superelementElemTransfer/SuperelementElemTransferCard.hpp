#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <vector>

namespace oor::model
{

// One SEELT card: elements that a superelement takes from the one downstream of it.
//
// Card layout: SEELT  SEID  EID1  EID2  EID3  EID4  EID5  EID6  EID7
//                    EID8  EID9  -etc.-
// or:         SEELT  SEID  EID1  "THRU"  EID2
// The elements are kept as written: single elements and THRU ranges, in order. An element connected only to exterior
// points of an upstream superelement is a boundary element, assigned to the downstream one; the card reassigns it
// upstream. When the deck has a BEGIN SUPER the card assigns the elements to SEID whether they are boundary
// elements or not, as SESET does for grid points, and SEID 0 is then a valid number; the card does not know
// whether the deck has one, so 0 is always accepted. The elements may be plot elements, general elements or structural
// elements. SEELT belongs in the main Bulk Data and is ignored inside a part: the elements are
// those of the residual structure. SEID is the number of a superelement the card defines (IdSpace::Group).
struct SuperelementElemTransferCard
{
    base::EntityId superelementId = 0;
    std::vector<IdRun> elements;

    [[nodiscard]] bool operator==(const SuperelementElemTransferCard&) const noexcept = default;
};

} // namespace oor::model
