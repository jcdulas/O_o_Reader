#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>

namespace oor::model
{

// One CACINF4 card: the base of an acoustic conjugate infinite element, a quadrilateral of four acoustic grid points.
// The element goes from this base out to infinity along the lines through the pole that its PACINF gives.
//
// Card layout: CACINF4  EID  PID  G1  G2  G3  G4
// EID is a positive integer below 100,000,000 and unique among all the elements; PID names a PACINF; the
// four points are positive IDs. The format reference does not say the points must be distinct (it says it of the
// cohesive elements), so the card does not refuse a repeated point. The element normal follows the right-hand rule on
// G1, G2, G3, G4: a normal that points into the fluid is reversed by the solver, which is a rule of the
// model, not of the card.
struct AcousticInfiniteQuad4Card
{
    static constexpr std::size_t NODE_COUNT = 4;

    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, NODE_COUNT> nodes = {};
};

} // namespace oor::model
