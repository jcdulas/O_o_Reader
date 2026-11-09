#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One CDAMP4 card: a scalar damper between two scalar points, with its own damping (no property).
//
// Card layout:
//   CDAMP4  EID  B  S1  S2
//
// S1 and S2 are scalar points; 0 (or blank) is a grounded end, but not both. A card may carry a second element in
// fields 5 to 8 (the format reference shows one per card; a deck may have two): each one is a card of its own here.
struct DamperScalarInlineCard
{
    base::EntityId id = 0;
    double damping = 0.0; // damping
    base::EntityId scalarPoint1 = 0;
    base::EntityId scalarPoint2 = 0;
};

} // namespace oor::model
