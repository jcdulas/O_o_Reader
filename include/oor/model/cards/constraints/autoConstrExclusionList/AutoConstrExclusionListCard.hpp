#pragma once

#include "oor/model/cards/sets/DofLists.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One SPCOFF1 card: the degrees of freedom C of a list of points that the AUTOSPC operation must not constrain.
//
// Card layout: SPCOFF1  C  G1  G2  G3  G4  G5  G6  G7
//                      G8  G9  -etc.-
// or:         SPCOFF1  C  G1  "THRU"  G2
// The list is kept as written: single points and THRU ranges, in order. C holds the digits of the components as one
// number (0 for scalar points). The card has no ID: it applies to the whole model.
struct AutoConstrExclusionListCard
{
    std::int32_t components = 0;
    std::vector<IdRun> nodes;
};

} // namespace oor::model
