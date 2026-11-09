#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>

namespace oor::model
{

// One GRIDS card: a slot surface point of the acoustic cavity (slot) analysis, a scalar degree of freedom (the
// pressure) with a two-dimensional location and a slot width. The card needs an AXSLOT card in the deck, which is a
// rule between cards. The CSLOT3 and CSLOT4 elements and the SLBDY boundary list name these points.
//
// Card layout:
//   GRIDS  ID  R  Z  W  IDF
//
// The ID is in the node ID space. R is the radial location in the basic system and may be any real but zero; Z the
// axial one. For plotting R is the basic X coordinate, Z the basic Y coordinate and the slot width W the basic Z
// coordinate (the pressure is plotted in the basic Z direction): the geometry of a slot element follows that rule,
// which is not the card's. W is the slot width or thickness, zero or more, and when it is blank the AXSLOT default WD
// applies, which only the model can tell, so a blank W is kept blank (an empty optional, a NaN in the
// column) and not a default.
//
// IDF is blank (0 here) or the number of a GRIDF fluid point: it is given only when the point is on an interface with
// the central cavity, and the CAXIFi elements next to it name it. The format reference asks the IDF to be
// defined on a GRIDF card too: a node reference. When IDF is given, R must be above zero.
struct AcousticSlotSurfacePointCard
{
    base::EntityId id = 0;
    double radius = 0.0;
    double zCoord = 0.0;
    std::optional<double> slotWidth; // blank: the WD of the AXSLOT
    base::EntityId fluidPointId = 0; // blank: not on an interface
};

} // namespace oor::model
