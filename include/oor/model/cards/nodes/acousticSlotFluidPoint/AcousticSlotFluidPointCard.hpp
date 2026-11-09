#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One GRIDF card: an axisymmetric fluid point of the acoustic cavity (slot) analysis, a scalar degree of freedom (the
// pressure) with a two-dimensional location. The card needs an AXSLOT card in the deck, which is a rule between cards.
//
// Card layout:
//   GRIDF  ID  R  Z
//
// The ID is a number below 1000000 in the node ID space (unique with respect to the scalar,
// structural, fluid and extra points). R is the radial location in the basic system (above zero) and Z the axial one.
// For plotting the R location is the basic X coordinate and the Z location the basic Y coordinate (a geometry
// rule that is not the card's). The grid points on the slot boundaries are GRIDS cards, not GRIDF.
struct AcousticSlotFluidPointCard
{
    base::EntityId id = 0;
    double radius = 0.0;
    double zCoord = 0.0;
};

} // namespace oor::model
