#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <string>

namespace oor::model
{

// One CHBDYG card: a boundary condition surface element defined by its grid points.
//
// Card layout:
//   CHBDYG  EID  (blank)  TYPE  IVIEWF  IVIEWB  RADMIDF  RADMIDB
//           G1   G2       G3    G4      G5      G6       G7       G8
// TYPE is REV or REV1 (the edge of an axisymmetric element: two primary points and an optional mid-side point G3),
// AREA3 (G1 to G3), AREA4 (G1 to G4), AREA6 (the three corners G1 to G3 and up to three mid-side points) or AREA8 (the
// four corners G1 to G4 and up to four mid-side points). A blank TYPE (empty) takes the TYPE of the BDYOR when the deck
// is read (HeatSurfaceNodesTable::finishRead), and so does a blank VIEW or RADM number (0). The grid
// points are `nodes`, 0 where a point is absent.
struct HeatSurfaceNodesCard
{
    static constexpr std::size_t NODE_COUNT = 8;

    base::EntityId id = 0;
    std::string kind = "AreaQuad4";
    base::EntityId frontViewId = 0;
    base::EntityId backViewId = 0;
    base::EntityId frontRadiationPropId = 0;
    base::EntityId backRadiationPropId = 0;
    std::array<base::EntityId, NODE_COUNT> nodes = {};
};

} // namespace oor::model
