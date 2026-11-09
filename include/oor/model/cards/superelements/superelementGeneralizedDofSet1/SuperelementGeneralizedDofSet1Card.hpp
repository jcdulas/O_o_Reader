#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One SEQSET1 card: the degrees of freedom C of a list of points that a superelement uses as generalized degrees of
// freedom (q-set).
//
// Card layout: SEQSET1  SEID  C  G1  G2  G3  G4  G5  G6
//                      G7  G8  -etc.-
// or:         SEQSET1  SEID  C  G1  "THRU"  G2
// SEID is a primary superelement, and the points are those of that superelement (its part, when the deck has one). C
// holds the digits of the components as one number (0 for scalar points). The points are kept as written: single points
// and THRU ranges, in order; the points between the ends of a range need not exist. The rules of SEQSET apply (SEQSET1
// is its alternate form). The card has no ID of its own.
struct SuperelementGeneralizedDofSet1Card
{
    base::EntityId superelementId = 0;
    std::int32_t components = 0;
    std::vector<IdRun> points;

    [[nodiscard]] bool operator==(const SuperelementGeneralizedDofSet1Card&) const noexcept = default;
};

} // namespace oor::model
