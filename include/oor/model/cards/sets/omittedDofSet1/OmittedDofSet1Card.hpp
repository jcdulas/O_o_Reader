#pragma once

#include "oor/model/cards/sets/DofLists.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One OMIT1 card: the degrees of freedom C of a list of points in the omitted set (o-set).
//
// Card layout: OMIT1  C  ID1  ID2  ID3  ID4  ID5  ID6  ID7
//                    ID8  ID9  ID10  -etc.-
// or:         OMIT1  C  ID1  "THRU"  ID2
// The list is kept as written: single points and THRU ranges, in order. C holds the digits of the components as one
// number (0 for scalar points). The card has no ID.
struct OmittedDofSet1Card
{
    std::int32_t components = 0;
    std::vector<IdRun> nodes;
};

} // namespace oor::model
