#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One CMASS4 card: a scalar mass element between two scalar points, with its own mass value (no property).
//
// Card layout:
//   CMASS4  EID  M  S1  S2
//
// S1 and S2 are scalar points; 0 (or blank) is a grounded end, but not both. A card may carry a second element in
// fields 5 to 8 (the format reference shows one per card; a deck may have two): each one is a card of its own here.
struct MassScalarInlineCard
{
    base::EntityId id = 0;
    double mass = 0.0; // mass
    base::EntityId scalarPoint1 = 0;
    base::EntityId scalarPoint2 = 0;
};

} // namespace oor::model
