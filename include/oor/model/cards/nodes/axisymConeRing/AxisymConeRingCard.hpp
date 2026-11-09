#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One RINGAX card: a ring of a conical shell model (an axisymmetric structure described by harmonics). The card needs
// an AXIC card in the deck, which is a rule between cards.
//
// Card layout (the fields the format leaves empty must stay blank):
//   RINGAX  ID  (blank)  R  Z  (blank)  (blank)  PS
//
// The ID is in the node ID space: the format reference asks it to be unique among the POINTAX, RINGAX and SECTAX
// numbers, and a POINTAX shares the space with it. A SECTAX, which is typed apart (its own numbers), is the rule
// between cards. A Case Control SET names the ring by ID + 1000000 * (harmonic + 1), which is not a number
// of this card.
//
// R is the radius (above zero) and Z the axial position of the ring. PS holds the permanent single-point constraints as
// digits of 1 to 6 without repeats (the library keeps them in ascending order, 0 for none); every harmonic of the ring
// loses those degrees of freedom.
struct AxisymConeRingCard
{
    base::EntityId id = 0;
    double radius = 0.0;
    double zCoord = 0.0;
    std::int32_t permanentConstraints = 0; // 0 = no permanent constraint
};

} // namespace oor::model
