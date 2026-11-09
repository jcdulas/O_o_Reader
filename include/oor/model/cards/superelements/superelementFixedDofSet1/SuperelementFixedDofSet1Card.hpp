#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One SEBSET1 card: the degrees of freedom C of a list of exterior points of a superelement that are fixed (b-set).
//
// Card layout: SEBSET1  SEID  C  G1  G2  G3  G4  G5  G6
//                      G7  G8  G9  -etc.-
// or:         SEBSET1  SEID  C  G1  "THRU"  G2
// SEID is the superelement, and the points are those of that superelement (its part, when the deck has one). C holds
// the digits of the components as one number (0 for scalar points). The points are kept as written: single points and
// THRU ranges, in order. The rules of SEBSET apply (SEBSET1 is its alternate form). The card has no ID of its own.
struct SuperelementFixedDofSet1Card
{
    base::EntityId superelementId = 0;
    std::int32_t components = 0;
    std::vector<IdRun> points;

    [[nodiscard]] bool operator==(const SuperelementFixedDofSet1Card&) const noexcept = default;
};

} // namespace oor::model
