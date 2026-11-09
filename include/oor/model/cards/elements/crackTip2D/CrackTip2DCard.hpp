#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>

namespace oor::model
{

// One CRAC2D card: a two-dimensional crack tip element (a dummy element of the format reference: the deck needs an
// ADUM8 entry for it, a rule between cards that is not checked here).
//
// Card layout:
//   CRAC2D  EID  PID  G1   G2   G3   G4   G5   G6
//           G7   G8   G9   G10  G11  G12  G13  G14
//           G15  G16  G17  G18
//
// PID names a PRAC2D. The element has two forms, drawn in the format reference figure of the element:
//   - the full crack: G1 to G10 are required and G11 to G18 are optional (the quadrilateral form);
//   - the symmetric half crack: G1 to G7 are required, G11 to G14 are optional, and G8 to G10 and G15 to G18 must not
//     be given.
// The card does not say which form it is: G8 to G10 all blank is the half crack, so `nodes` keeps the 18
// slots, 0 where a field is blank, and the position of a slot is its G number.
struct CrackTip2DCard
{
    static constexpr std::size_t NODE_COUNT = 18;

    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, NODE_COUNT> nodes = {};

    // True when G8, G9 and G10 are all blank: the symmetric half-crack option.
    [[nodiscard]] bool isHalfCrack() const noexcept { return nodes[7] == 0 && nodes[8] == 0 && nodes[9] == 0; }
};

} // namespace oor::model
