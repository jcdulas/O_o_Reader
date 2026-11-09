#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <optional>

namespace oor::model
{

// One CAXISYM card (SOL 400): an axisymmetric thick shell line element with two end points and, optionally, a third
// point between them on the curve of the element. It lies in the x-y plane of the basic system (x is the radius, y the
// axis), and its normal is on the left of the way from G1 to G2 (format reference figure).
//
// Card layout: CAXISYM  EID  PID  G1  G2  G3  NOFF
// EID is a positive integer below 100,000,000, unique among all the elements. PID names a PAXISYM (a
// property card, so the reference resolves). G1 and G2 are required, G3 is blank for a straight two-node element
// (stored as 0), and the points must be different ("Unique" on the format). The format reference recommends G3 in the
// middle third of the element: a recommendation, not a rule of the card. NOFF is the offset from the surface
// of the points to the reference plane of the element; the format gives it no default, so a blank one stays blank
// (nothing).
struct ShellAxisymCard
{
    static constexpr std::size_t NODE_COUNT = 3;
    static constexpr std::size_t END_COUNT = 2; // G1 and G2 are required

    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, NODE_COUNT> nodes = {}; // G1, G2, G3 (0 when the element has two nodes)
    std::optional<double> offset;                      // blank: no value given

    [[nodiscard]] bool hasMiddleNode() const noexcept { return nodes[2] != 0; }
};

} // namespace oor::model
