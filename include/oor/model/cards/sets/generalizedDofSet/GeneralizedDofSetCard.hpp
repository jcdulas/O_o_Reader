#pragma once

#include "oor/model/cards/sets/DofLists.hpp"

#include <vector>

namespace oor::model
{

// One QSET card: degrees of freedom of the generalized set (q-set).
//
// Card layout: QSET  ID1  C1  ID2  C2  ID3  C3  ID4  C4
// and, for more than four points, the same pairs on the continuation lines. IDi is a grid or scalar point, Ci the
// digits of its components as one number (0 for a scalar point). The card has no ID: it adds to the q-set of the model.
struct GeneralizedDofSetCard
{
    std::vector<DofPair> dofs;
};

} // namespace oor::model
