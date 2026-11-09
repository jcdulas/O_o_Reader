#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

// One SENQSET card: how many scalar points a superelement generates for its dynamic reduction.
//
// Card layout: SENQSET  SEID  N                                                   fields 1 and 2
// SEID is a partitioned superelement, or the word ALL, which asks for N generalized coordinates in every superelement
// but the residual structure; a card for a superelement overrides the one for ALL (kept as no number). N is
// the number of scalar points, 0 when blank. The card is only needed to have the solver make the points: the
// generalized coordinates a QSET, QSET1 or SEQSET card gives make it generate none, and PARAM,NQSET says the
// same. The card belongs in the main Bulk Data and is ignored after a BEGIN SUPER, and it works only when
// the deck has part superelements.
struct SuperelementGeneralizedPointCountCard
{
    SuperelementOrAll superelementId; // nothing is ALL
    base::EntityId count = 0;

    [[nodiscard]] bool operator==(const SuperelementGeneralizedPointCountCard&) const noexcept = default;
};

} // namespace oor::model
