#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>

namespace oor::model
{

// One CIFQDX card (SOL 400): a linear or quadratic axisymmetric cohesive interface element with four to eight nodes,
// used to simulate the onset and growth of a delamination between two axisymmetric bodies.
//
// Card layout:
//   CIFQDX  EID  PID  G1   G2   G3   G4   G5   G6        fields 1 to 8
//           G7   G8                                      fields 9 and 10
//
// G1 and G2 are the ends of the bottom line and G3 and G4 the ends of the top line; G5 is the point halfway along the
// bottom line (G1-G5-G2) and G7 the one along the top line (G3-G7-G4), G6 and G8 the points on the sides, which only
// make the element compatible with the eight-node axisymmetric quadrilateral. `nodes` always has
// room for the eight, in the order of the card, and a point that is blank on the card is 0, so that the position of a
// point says what it is. The element lies in the x-y plane of the basic system, r along x and z along y, and
// may be infinitely thin: the two lines may coincide in space.
//
// EID is a positive integer below 100,000,000, unique among the elements of every kind. PID names a PCOHE.
// G1 to G4 are required, the other points optional, and every point given is unique (the format asks for distinct
// positive integers). The format says the element is quadratic when G5 and G7 are given; the card does not check that
// they come together.
struct CohesiveAxisymQuadCard
{
    static constexpr std::size_t NODE_COUNT = 8;
    static constexpr std::size_t CORNER_COUNT = 4; // G1 to G4 are required

    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, NODE_COUNT> nodes = {};
};

} // namespace oor::model
