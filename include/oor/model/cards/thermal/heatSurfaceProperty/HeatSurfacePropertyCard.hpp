#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>
#include <string>

namespace oor::model
{

// One CHBDYP card: a boundary condition surface element defined by grid points and a PHBDY property.
//
// Card layout:
//   CHBDYP  EID  PID  TYPE  IVIEWF  IVIEWB  G1  G2  G0
//           RADMIDF  RADMIDB  GMID  CE  E1  E2  E3
// TYPE is POINT (G1 only), LINE, ELCYL, FTUBE or TUBE (G1 and G2). G0 is the orientation grid point, CE the system of
// the orientation vector E1, E2, E3 (whose origin is G1), GMID the mid-side point of a LINE. A blank VIEW or RADM
// number, G0, GMID and CE are 0 (the format reference lets BDYOR give defaults; BDYOR is not typed). PID is the ID of a
// PHBDY, not a structural property.
struct HeatSurfacePropertyCard
{
    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::string kind = "Line";
    base::EntityId frontViewId = 0;
    base::EntityId backViewId = 0;
    base::EntityId node1 = 0;
    base::EntityId node2 = 0; // 0 for a POINT
    base::EntityId orientationNode = 0;
    base::EntityId frontRadiationPropId = 0;
    base::EntityId backRadiationPropId = 0;
    base::EntityId midsideNode = 0;
    base::EntityId directionFrameId = 0;
    std::optional<double> direction1;
    std::optional<double> direction2;
    std::optional<double> direction3;
};

} // namespace oor::model
