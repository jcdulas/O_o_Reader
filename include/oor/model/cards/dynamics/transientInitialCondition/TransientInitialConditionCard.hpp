#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One TIC card: the initial displacement and velocity of one degree of freedom of a transient analysis (selected by IC
// in Case Control; several TIC cards share a set ID).
//
// Card layout: TIC  SID  G  C  U0  V0
// G is a grid, scalar or extra point, or (C = -1) the number of a modal coordinate; C is a component 1 to 6 of a grid
// point, 0 (or blank) for a scalar or extra point, -1 for a modal coordinate.
struct TransientInitialConditionCard
{
    static constexpr std::int32_t MODAL = -1; // C of a modal coordinate

    base::EntityId id = 0;
    base::EntityId node = 0;
    std::int32_t components = 0;
    double initialDisplacement = 0.0; // the initial displacement
    double initialVelocity = 0.0;     // the initial velocity
};

} // namespace oor::model
