#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One BOLT card: a rigid bolt made of multi-point constraints between pairs of grid points on the two sides of its
// cut (SOLs 101, 103, 400): each pair (GTi, GBi) is tied to the control grid point GRIDC in all six degrees of freedom,
// so that the load or enforced motion of GRIDC pretensions the bolt. In 3D contact the bottom grid points are replaced
// by the top ones on the contact surface. (The BOLT of another implementation of the format, with no TOP and BOTTOM
// keyword, is another layout and is not read.)
//
// Card layout: BOLT  ID  GRIDC
//             (continuation)  TOP  GT1  GT2  GT3 ... GT7 / GT8  GT9 ...
//             (continuation)  BOTTOM  GB1  GB2  GB3 ... GB7 / GB8  GB9 ...
// The two lists have the same number of grid points, one pair after the other; the lists go on over continuation lines.
struct BoltConnectionCard
{
    base::EntityId id = 0;                   // the ID of the bolt (an element ID in the format reference)
    base::EntityId controlNode = 0;          // the control grid point (not connected to any element, not in BOTTOM)
    std::vector<base::EntityId> topNodes;    // GT1, GT2, ...
    std::vector<base::EntityId> bottomNodes; // GB1, GB2, ...
};

} // namespace oor::model
