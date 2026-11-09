#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>

namespace oor::model
{

// One CTETRA card: a four-sided solid element with four to ten grid points.
//
// Card layout:
//   CTETRA  EID  PID  G1  G2  G3  G4  G5  G6
//           G7   G8   G9  G10
//
// G1 to G4 are the corners; G5 to G10 are the optional points on the edges. `nodes` always has room for all ten: an
// edge point that is blank on the card (or zero) is 0, so that the edge a point belongs to is its position.
struct SolidTetraCard
{
    static constexpr std::size_t CORNER_COUNT = 4;
    static constexpr std::size_t NODE_COUNT = 10;

    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, NODE_COUNT> nodes = {};

    [[nodiscard]] bool hasEdgeNodes() const noexcept
    {
        for (std::size_t index = CORNER_COUNT; index < NODE_COUNT; ++index)
        {
            if (nodes[index] != 0)
                return true;
        }
        return false;
    }
};

} // namespace oor::model
