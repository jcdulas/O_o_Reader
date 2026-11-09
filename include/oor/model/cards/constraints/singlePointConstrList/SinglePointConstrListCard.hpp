#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace oor::model
{

// A run of grid points of an SPC1 card: a single point (first == last) or "first THRU last".
struct NodeRun
{
    base::EntityId first = 0;
    base::EntityId last = 0;

    [[nodiscard]] bool operator==(const NodeRun&) const noexcept = default;
};

// One SPC1 card: single point constraints on a list of grid points.
//
// Card layout: SPC1  SID  C  G1  G2  G3  G4  G5  G6
//                   G7  G8  G9  -etc.-
// or:         SPC1  SID  C  G1  "THRU"  G2
// The list is kept as written: single points and THRU ranges, in order.
struct SinglePointConstrListCard
{
    base::EntityId id = 0;
    std::int32_t components = 0; // the digits of C as one number: 123 for "123", 0 for scalar points
    std::vector<NodeRun> nodes;
};

} // namespace oor::model
