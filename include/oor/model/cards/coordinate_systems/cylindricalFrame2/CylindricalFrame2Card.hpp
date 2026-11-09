#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One CORD2C card: a cylindrical coordinate system defined by three points.
//
// Card layout:
//   CORD2C  CID  RID  A1  A2  A3  B1  B2  B3
//           C1   C2   C3
//
// The points are given in the system RID (0 or blank: the basic system). A is the origin, B a point on the z-axis and
// C a point in the plane of the azimuthal origin. The card holds what was written: the axes are not computed here, and
// the three points are not checked for being distinct and noncolinear (the format reference leaves that to the geometry
// processor).
struct CylindricalFrame2Card
{
    base::EntityId id = 0;
    base::EntityId referenceFrameId = 0;
    std::array<double, 3> pointA = {};
    std::array<double, 3> pointB = {};
    std::array<double, 3> pointC = {};
};

} // namespace oor::model
