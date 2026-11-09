#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <vector>

namespace oor::model
{

// One SEQSET card: the generalized degrees of freedom (q-set) a superelement uses in the generalized dynamic reduction
// or the component mode synthesis.
//
// Card layout: SEQSET  SEID  ID1  C1  ID2  C2  ID3  C3
// and, for more than three points, the pairs ID C on the continuation lines, four to a line (field 9 on). IDi is a grid
// or scalar point, Ci the digits of its components as one number (0 for a scalar point). SEID is a primary
// superelement, and the points are those of that superelement (its part, when the deck has one). The degrees of freedom
// may not be given for another superelement; they are interior to the residual structure and form
// members of the q-set, which no other set shares; the card gives the superelement its connection, so no
// CSUPEXT is needed. The card has no ID of its own.
struct SuperelementGeneralizedDofSetCard
{
    base::EntityId superelementId = 0;
    std::vector<DofPair> dofs;

    [[nodiscard]] bool operator==(const SuperelementGeneralizedDofSetCard&) const noexcept = default;
};

} // namespace oor::model
