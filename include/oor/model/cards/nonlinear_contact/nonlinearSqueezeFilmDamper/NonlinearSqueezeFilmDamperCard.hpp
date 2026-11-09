#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace oor::model
{

// One NLRSFD card: a nonlinear transient radial squeeze film damper between two coincident grid points (a nonlinear
// load, selected with NONLINEAR = SID, like the NOLIN cards).
//
// Card layout: NLRSFD  SID  GA  GB  PLANE  BDIA  BLEN  BCLR  SOLN
//                     VISCO  PVAPCO  NPORT  PRES1  THETA1  PRES2  THETA2  NPNT
//                     OFFSET1  OFFSET2  GRPNAME  EVALNAME
//                     RDATA1  RDATA2  RDATA3  RDATA4  RDATA5  RDATA6  RDATA7  RDATA8
// With GRPNAME the damper takes its characteristics from an external service (CONNECT SERVICE) and the fields from BDIA
// to OFFSET2 are ignored: they may be blank, and are then 0.0 in the card. Without it, BDIA, BLEN, BCLR, VISCO, PVAPCO
// and NPORT are required, and so are PRES1 and THETA1 with one or two ports and PRES2 and THETA2 with two.
struct NonlinearSqueezeFilmDamperCard
{
    base::EntityId setId = 0; // the nonlinear load set (several cards may share it)
    base::EntityId nodeA = 0; // the inner grid point (the journal)
    base::EntityId nodeB = 0; // the outer grid point (the housing)
    std::string plane =
        "PlaneXy"; // XY, YZ or ZX (the format reference also writes XZ), in the displacement systems of GA and GB
    double bearingDiameter = 0.0;     // the journal diameter
    double bearingLength = 0.0;       // the damper length
    double bearingClearance = 0.0;    // the radial clearance
    std::string bearingKind = "Long"; // LONG or SHORT bearing
    double viscosity = 0.0;           // the dynamic viscosity of the lubricant
    double vapourPressure = 0.0;      // the vapour pressure of the lubricant
    std::int32_t portCount = 0;       // the number of lubrication ports: 1 or 2
    double pressure1 = 0.0;           // the boundary pressure of port 1
    double portAngle1 = 0.0;          // the angular position of port 1
    double pressure2 = 0.0;           // the boundary pressure of port 2
    double portAngle2 = 0.0;          // the angular position of port 2
    std::int32_t pointCount = 101;    // the number of finite difference points of the damper arc (odd, below 201)
    double offset1 = 0.0;             // the offset in the direction 1
    double offset2 = 0.0;             // the offset in the direction 2
    std::string serviceGroup;      // the group name of the CONNECT SERVICE statement; blank without an external service
    std::string serviceEvaluation; // the "evalname" argument passed to the service
    std::array<double, 8> serviceParameters{}; // the parameters "parm1" to "parm8" passed to the service
};

} // namespace oor::model
