#pragma once

#include "oor/model/cards/sets/DofLists.hpp"

#include <vector>

namespace oor::model
{

// One SPCOFF card: degrees of freedom that the AUTOSPC operation must not constrain.
//
// Card layout: SPCOFF  G1  C1  G2  C2  G3  C3  G4  C4
// and, for more than four points, the same pairs on the continuation lines. Gi is a grid or scalar point, Ci the digits
// of its components as one number (0 for a scalar point). The card has no ID: it applies to the whole model.
struct AutoConstrExclusionCard
{
    std::vector<DofPair> dofs;
};

} // namespace oor::model
