#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One CYLINDR card (SOL 700): a cylindrical shape that a TICEUL1 region (a TICREG of type CYLINDER) or an EULFREG
// region of type CYLINDER uses to give initial conditions or an acceleration field to the Eulerian elements that lie
// fully or partly inside it. The name of the card has no final E, the type of the regions spells it CYLINDER.
//
// Card layout: CYLINDR  VID  XC1  YC1  ZC1  XC2  YC2  ZC2  RAD            fields 1 to 8
// The cylinder is the set of points within RAD of the axis between the two end points (all the coordinates are
// required). Only this entry of the format reference is typed here: the region cards choose between the shape cards by
// their TYPE, and CYLINDR has no other entry of the same name.
struct CylinderShapeCard
{
    base::EntityId id = 0; // unique among the shapes of an Euler region
    double centerX1 = 0.0; // the first end of the axis
    double centerY1 = 0.0;
    double centerZ1 = 0.0;
    double centerX2 = 0.0; // the second end of the axis
    double centerY2 = 0.0;
    double centerZ2 = 0.0;
    double radius = 0.0; // the radius, greater than zero
};

} // namespace oor::model
