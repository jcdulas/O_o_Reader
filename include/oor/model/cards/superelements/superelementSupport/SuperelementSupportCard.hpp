#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <vector>

namespace oor::model
{

// One SESUP card: the fictitious supports of a superelement, the determinate reactions of a free-body analysis.
//
// Card layout: SESUP  SEID  ID1  C1  ID2  C2  ID3  C3
// and, for more than three points, the pairs ID C on the continuation lines, four to a line (field 9 on). IDi is a grid
// or scalar point, Ci the digits of its components as one number (0 for a scalar point). SEID is a primary
// superelement, and the points are exterior points of that superelement (the points of its part, when the deck has
// one). The degrees of freedom form members of the r-set, which no other set shares, and must be enough to
// discard the free body modes. The card is not allowed for part superelements, which use SUPORT.
// The card has no ID of its own.
struct SuperelementSupportCard
{
    base::EntityId superelementId = 0;
    std::vector<DofPair> dofs;

    [[nodiscard]] bool operator==(const SuperelementSupportCard&) const noexcept = default;
};

} // namespace oor::model
