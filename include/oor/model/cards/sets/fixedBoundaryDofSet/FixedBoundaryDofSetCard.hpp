#pragma once

#include "oor/model/cards/sets/DofLists.hpp"

#include <vector>

namespace oor::model
{

// One BSET card: degrees of freedom of the fixed analysis set (b-set).
//
// Card layout: BSET  ID1  C1  ID2  C2  ID3  C3  ID4  C4
// and, for more than four points, the same pairs on the continuation lines. IDi is a grid or scalar point, Ci the
// digits of its components as one number (0 for a scalar point). The card has no ID: it adds to the b-set of the model.
struct FixedBoundaryDofSetCard
{
    std::vector<DofPair> dofs;
};

} // namespace oor::model
