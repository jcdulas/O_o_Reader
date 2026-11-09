#pragma once

#include "oor/model/cards/sets/DofLists.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One BNDFRE1 card: the degrees of freedom C of a list of points in the free boundary set (c-set). It is the CSET1 card
// under another name (either may be used, or both).
//
// Card layout: BNDFRE1  C  ID1  ID2  ID3  ID4  ID5  ID6  ID7
//                      ID8  ID9  -etc.-
// or:         BNDFRE1  C  ID1  "THRU"  ID2
// or:         BNDFRE1  "ALL"
// The list is kept as written: single points and THRU ranges, in order. C holds the digits of the components as one
// number (0 for scalar points). With the word ALL every a-set degree of freedom is free: `all` is set, and the card has
// no component and no point. The card has no ID.
struct FreeBoundaryDofs1Card
{
    std::int32_t components = 0;
    std::vector<IdRun> nodes;
    bool all = false;
};

} // namespace oor::model
