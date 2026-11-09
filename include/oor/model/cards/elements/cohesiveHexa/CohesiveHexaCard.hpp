#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>

namespace oor::model
{

// One CIFHEX card (SOL 400): a linear or quadratic three-dimensional cohesive interface element with eight to twenty
// nodes, used to simulate the onset and growth of a delamination between two bodies.
//
// Card layout:
//   CIFHEX  EID  PID  G1   G2   G3   G4   G5   G6        fields 1 to 8
//           G7   G8   G9   G10  G11  G12  G13  G14       fields 9 to 16
//           G15  G16  G17  G18  G19  G20                 fields 17 to 22
//
// G1 to G4 are the corners of the bottom face and G5 to G8 the corners of the top face; G9 to G12 are the points on the
// edges of the bottom face and G17 to G20 the ones on the edges of the top face, G13 to G16 the points halfway
// between the faces, which only make the element compatible with the twenty-node hexahedron (the figure of
// the format). `nodes` always has room for the twenty, in the order of the card, and a point that is blank on the
// card is 0, so that the position of a point says what it is.
//
// EID is a positive integer below 100,000,000, unique among the elements of every kind. PID names a PCOHE.
// G1 to G8 are required, the other points optional and every point given is unique (the format asks for distinct
// positive integers of every group). The format is not consistent about the optional points: one rule asks for the
// eight corners alone or for all twenty points, while the element is quadratic with G9-G12 and G17-G20 only (G13-G16
// are there for the compatibility with the hexahedron), and each optional point is described as optional on its own.
// The card follows the descriptions and the format: it refuses nothing about which optional points are given, so a deck
// that the solver takes is read; whether the element is linear or quadratic is a question for the model, not for the
// card.
struct CohesiveHexaCard
{
    static constexpr std::size_t NODE_COUNT = 20;
    static constexpr std::size_t CORNER_COUNT = 8; // G1 to G8 are required

    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, NODE_COUNT> nodes = {};
};

} // namespace oor::model
