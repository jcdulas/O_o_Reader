#pragma once

#include "oor/model/cards/sets/DofLists.hpp"

#include <vector>

namespace oor::model
{

// One BNDFREE card: degrees of freedom of the free boundary set (c-set) for the generalized dynamic reduction and the
// component mode synthesis. It is the CSET card under another name: either of them, or both, may be used,
// and the degrees of freedom form members of the c-set.
//
// Card layout: BNDFREE  ID1  C1  ID2  C2  ID3  C3  ID4  C4
// and, for more than four points, the same pairs on the continuation lines. IDi is a grid or scalar point, Ci the
// digits of its components as one number (0 for a scalar point). The card has no ID: it adds to the c-set of the model.
struct FreeBoundaryDofsCard
{
    std::vector<DofPair> dofs;
};

} // namespace oor::model
