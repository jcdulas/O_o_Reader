#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One CELAS4 card: a scalar spring between two scalar points, with its own stiffness (no property).
//
// Card layout:
//   CELAS4  EID  K  S1  S2
//
// S1 and S2 are scalar points; 0 (or blank) is a grounded end, but not both. A card may carry a second element in
// fields 5 to 8 (the format reference shows one per card; a deck may have two): each one is a card of its own here.
struct SpringScalarInlineCard
{
    base::EntityId id = 0;
    double stiffness = 0.0; // stiffness
    base::EntityId scalarPoint1 = 0;
    base::EntityId scalarPoint2 = 0;
};

} // namespace oor::model
