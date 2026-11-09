#pragma once

#include "oor/model/cards/sets/DofLists.hpp"

#include <vector>

namespace oor::model
{

// One CYAX card: the grid points that lie on the axis of symmetry of a cyclic symmetry model.
//
// Card layout: CYAX  G1  G2  G3  G4  G5  G6  G7  G8
//                   G9  G10  -etc.-
// The points are kept as written in `grids`: single grid points and "first THRU last" ranges, in order. A range stands
// for the points that exist between its ends and may not begin or end a line of the card (the writer sees to it; the
// reader does not mind where the words are). The card has no ID of its own (IdSpace::None). The format does not limit a
// deck to one CYAX.
struct CyclicAxisNodesCard
{
    std::vector<IdRun> grids;

    [[nodiscard]] bool operator==(const CyclicAxisNodesCard&) const noexcept = default;
};

} // namespace oor::model
