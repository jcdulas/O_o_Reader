#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>

namespace oor::model
{

// One CIFPENT card (SOL 400): a linear or quadratic three-dimensional cohesive interface element with six to fifteen
// nodes (a wedge), used to simulate the onset and growth of a delamination between two bodies.
//
// Card layout:
//   CIFPENT  EID  PID  G1   G2   G3   G4   G5   G6       fields 1 to 8
//            G7   G8   G9   G10  G11  G12  G13  G14      fields 9 to 16
//            G15                                         field 17
//
// G1 to G3 are the corners of the bottom triangle and G4 to G6 the corners of the top triangle; G7 to G9 are the points
// on the edges of the bottom triangle and G13 to G15 the ones on the edges of the top triangle, G10 to G12 the points
// halfway between the faces, which only make the element compatible with the fifteen-node pentahedron (the figure of
// the format). `nodes` always has room for the fifteen, in the order of the card, and a point that is blank on
// the card is 0, so that the position of a point says what it is.
//
// EID is a positive integer below 100,000,000, unique among the elements of every kind. PID names a PCOHE.
// G1 to G6 are required, the other points optional and every point given is unique (the format asks for distinct
// positive integers of every group). As for the CIFHEX, the format is not consistent about the optional points (the six
// corners alone or all fifteen points; quadratic with G7-G9 and G13-G15; each one optional on its own), and the card
// refuses nothing about which of them are given.
struct CohesivePentaCard
{
    static constexpr std::size_t NODE_COUNT = 15;
    static constexpr std::size_t CORNER_COUNT = 6; // G1 to G6 are required

    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, NODE_COUNT> nodes = {};
};

} // namespace oor::model
