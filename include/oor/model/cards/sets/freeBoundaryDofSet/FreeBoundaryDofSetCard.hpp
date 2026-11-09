#pragma once

#include "oor/model/cards/sets/DofLists.hpp"

#include <vector>

namespace oor::model
{

// One CSET card: degrees of freedom of the free boundary set (c-set).
//
// Card layout: CSET  ID1  C1  ID2  C2  ID3  C3  ID4  C4
// and, for more than four points, the same pairs on the continuation lines. IDi is a grid or scalar point, Ci the
// digits of its components as one number (0 for a scalar point). The card has no ID: it adds to the c-set of the model.
struct FreeBoundaryDofSetCard
{
    std::vector<DofPair> dofs;
};

} // namespace oor::model
