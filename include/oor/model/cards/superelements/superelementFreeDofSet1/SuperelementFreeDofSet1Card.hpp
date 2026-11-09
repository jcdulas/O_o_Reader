#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One SECSET1 card: the degrees of freedom C of a list of exterior points of a superelement that are free (c-set).
//
// Card layout: SECSET1  SEID  C  G1  G2  G3  G4  G5  G6
//                      G7  G8  G9  -etc.-
// or:         SECSET1  SEID  C  G1  "THRU"  G2
// or:         SECSET1  SEID  "ALL"
// SEID is the superelement, and the points are those of that superelement (its part, when the deck has one). C holds
// the digits of the components as one number (0 for scalar points). The points are kept as written: single points and
// THRU ranges, in order. With the word ALL (where C is) every exterior degree of freedom is free: `all` is set and the
// card has no component and no point. In the alternate forms the points need not exist or be exterior, and the ones
// that are not give one warning and are ignored. The card has no ID of its own.
struct SuperelementFreeDofSet1Card
{
    base::EntityId superelementId = 0;
    std::int32_t components = 0;
    std::vector<IdRun> points;
    bool all = false;

    [[nodiscard]] bool operator==(const SuperelementFreeDofSet1Card&) const noexcept = default;
};

} // namespace oor::model
