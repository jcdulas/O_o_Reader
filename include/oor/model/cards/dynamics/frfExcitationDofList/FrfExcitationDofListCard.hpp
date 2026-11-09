#pragma once

#include "oor/model/cards/sets/DofLists.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One FRFXIT1 card: degrees of freedom where unit loads are applied to generate frequency response functions, for a
// list of points that share their components.
//
// Card layout: FRFXIT1  C  PNTID1  PNTID2  PNTID3  PNTID4  PNTID5  PNTID6  PNTID7
// or:         FRFXIT1  C  PNTID1  "THRU"  PNTID2
// C holds the digits 1 to 6 of the components as one number in ascending order (123 for "321"), 0 or blank for scalar
// points. The points are grid or scalar points of this model, listed as written: single points and THRU ranges, in
// order (the points of a range need not all exist). The format draws one line of at most seven entries; the
// card reads the list over as many fields as the deck gives, as the other lists of points do, and writes a longer one
// on continuation lines. The card has no ID of its own.
struct FrfExcitationDofListCard
{
    std::int32_t components = 0;
    std::vector<IdRun> pointIds;

    [[nodiscard]] bool operator==(const FrfExcitationDofListCard&) const noexcept = default;
};

} // namespace oor::model
