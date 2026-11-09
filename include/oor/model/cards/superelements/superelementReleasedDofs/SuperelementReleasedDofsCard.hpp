#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One RELEASE card: degrees of freedom of exterior grid points of a superelement that its connection leaves free.
//
// Card layout: RELEASE  SEID  C  G1  G2  G3  G4  G5  G6
//                      G7  G8  -etc.-
// or:         RELEASE  SEID  C  G1  "THRU"  G2
// or:         RELEASE  SEID  C  "ALL"           (no continuation line)
// SEID is a superelement (never the residual structure), C the digits of the components as one number (1 to
// 6, each at most once). The points are kept as written: single points and THRU ranges, in order, or `all` for the word
// ALL, which releases every exterior grid point. A point must be an exterior grid point of the superelement,
// which the model leaves to the check of the connection; the points of a THRU range need not be. The card
// applies to the superelement solution sequences (SOL 101 to 200) and not to part superelements, whose
// points live in the part: the points are those of the superelement SEID all the same.
struct SuperelementReleasedDofsCard
{
    base::EntityId superelementId = 0;
    std::int32_t components = 0;
    bool all = false;          // the word ALL: every exterior point
    std::vector<IdRun> points; // empty with ALL

    [[nodiscard]] bool operator==(const SuperelementReleasedDofsCard&) const noexcept = default;
};

} // namespace oor::model
