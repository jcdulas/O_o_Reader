#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace oor::model
{

// One SEUSET1 card: the degrees of freedom C of a list of points of a superelement in a set the user names.
//
// Card layout: SEUSET1  SEID  SNAME  C  G1  G2  G3  G4  G5
//                      G6  G7  -etc.-
// or:         SEUSET1  SEID  SNAME  C  G1  "THRU"  G2
// SEID is the superelement, and the points are those of that superelement (its part, when the deck has one). SNAME is
// one to four characters or ZERO followed by a set name (see SEUSET), kept in upper case. C holds the digits of the
// components as one number (0 for scalar points). The points are kept as written: single points and THRU ranges, in
// order; all the points of a range are in the set. The card has no ID of its own.
struct SuperelementUserDofSet1Card
{
    base::EntityId superelementId = 0;
    std::string setName;
    std::int32_t components = 0;
    std::vector<IdRun> points;

    [[nodiscard]] bool operator==(const SuperelementUserDofSet1Card&) const noexcept = default;
};

} // namespace oor::model
