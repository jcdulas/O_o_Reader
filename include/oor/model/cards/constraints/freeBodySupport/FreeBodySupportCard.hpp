#pragma once

#include "oor/model/cards/sets/DofLists.hpp"

#include <vector>

namespace oor::model
{

// One SUPORT card: the reference degrees of freedom of a free body (the r-set), which determine its rigid body motion.
//
// Card layout: SUPORT  ID1  C1  ID2  C2  ID3  C3  ID4  C4
// and, for more than four points, the same pairs on the continuation lines (up to 24 degrees of freedom on a card). IDi
// is a grid or scalar point, Ci the digits of its components as one number (0 for a scalar point). The card has no ID:
// it applies to the whole model.
struct FreeBodySupportCard
{
    std::vector<DofPair> dofs;
};

} // namespace oor::model
