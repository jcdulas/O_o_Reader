#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <vector>

namespace oor::model
{

// One SECSET card: degrees of freedom of the exterior points of a superelement that are free (c-set) in the component
// mode synthesis.
//
// Card layout: SECSET  SEID  ID1  C1  ID2  C2  ID3  C3
// and, for more than three points, the pairs ID C on the continuation lines, four to a line (field 9 on). IDi is a grid
// or scalar point, Ci the digits of its components as one number (0 for a scalar point). SEID is the superelement, and
// the points are those of that superelement (its part, when the deck has one). Exterior points are fixed (b-set) unless
// listed here or on an SESUP; the degrees of freedom form members of the c-set, which no other set shares.
// The card has no ID of its own.
struct SuperelementFreeDofSetCard
{
    base::EntityId superelementId = 0;
    std::vector<DofPair> dofs;

    [[nodiscard]] bool operator==(const SuperelementFreeDofSetCard&) const noexcept = default;
};

} // namespace oor::model
