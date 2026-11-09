#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <vector>

namespace oor::model
{

// One SUPORT1 card: the reference degrees of freedom of a free body (the r-set) that the SUPORT1 Case Control command
// selects by SID. They are added to those of the SUPORT cards.
//
// Card layout: SUPORT1  SID  ID1  C1  ID2  C2  ID3  C3
//                      ID4  C4  -etc.-
// SID is the set the Case Control command names; it is a constraint set ID (IdSpace::Group). IDi is a grid or scalar
// point, Ci the digits of its components as one number (0 for a scalar point). Field 8 is blank, so the fourth pair is
// in field 9, the first of the continuation line (up to 18 degrees of freedom on a card).
struct FreeBodySupportSetCard
{
    base::EntityId id = 0;
    std::vector<DofPair> dofs;
};

} // namespace oor::model
