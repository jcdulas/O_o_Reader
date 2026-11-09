#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>

namespace oor::model
{

// One CRAC3D card: a three-dimensional crack tip element (a dummy element of the format reference: the deck needs an
// ADUM9 entry for it, a rule between cards that is not checked here).
//
// Card layout: CRAC3D  EID  PID  G1  G2  G3  G4  G5  G6, then eight grid points to each continuation line (G7 to G14,
// G15 to G22, ... G55 to G62) and a last line with G63 and G64. PID names a PRAC3D and is positive, like
// every other property ID (the format gives it a negative range by mistake).
//
// The element is a block of two faces (G1 to G18 and G19 to G36) and a mid-plane (G37 to G46, with the surface points
// G47 to G64); the figures of the format reference give the numbering. The two forms the format describes:
//   - the full crack: G1 to G10 and G19 to G28 are required, the other points are optional, and G37 to G46 are either
//     all given or all blank (a fatal error otherwise);
//   - the symmetric half crack: G1 to G7 and G19 to G25 are required, and these must not be given:
//     G8 to G10, G15 to G18, G26 to G28, G33 to G36, G43 to G46, G51 to G55 and G60 to G64.
// The card does not say which form it is: G8 to G10 all blank is the half crack. `nodes` keeps the 64 slots, 0 where a
// field is blank, and the position of a slot is its G number. The midside points of the two faces "should" come in
// pairs: that is advice, not checked.
struct CrackTip3DCard
{
    static constexpr std::size_t NODE_COUNT = 64;

    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, NODE_COUNT> nodes = {};

    // True when G8, G9 and G10 are all blank: the symmetric half-crack option.
    [[nodiscard]] bool isHalfCrack() const noexcept { return nodes[7] == 0 && nodes[8] == 0 && nodes[9] == 0; }
};

} // namespace oor::model
