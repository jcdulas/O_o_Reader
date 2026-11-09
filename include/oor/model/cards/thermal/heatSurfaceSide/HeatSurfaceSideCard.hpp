#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One CHBDYE card: a boundary condition surface element that is one side of a heat conduction element.
//
// Card layout: CHBDYE  EID  EID2  SIDE  IVIEWF  IVIEWB  RADMIDF  RADMIDB
// EID2 is the conduction element and SIDE (1 to 6) the side of it (the numbering of the format: the faces of
// the solid elements, side 1 the top of a shell, 2 to 5 its edges, 6 its bottom, the sides of a line element). A blank
// VIEW or RADM number is 0 (the format reference lets BDYOR give a default; BDYOR is not typed).
struct HeatSurfaceSideCard
{
    base::EntityId id = 0;
    base::EntityId conductionElemId = 0;
    int sideCode = 1;
    base::EntityId frontViewId = 0;
    base::EntityId backViewId = 0;
    base::EntityId frontRadiationPropId = 0;
    base::EntityId backRadiationPropId = 0;
};

} // namespace oor::model
