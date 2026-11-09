#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// One CBEND card: a curved beam, curved pipe or elbow element between two grid points.
//
// Card layout (orientation by vector, or by a grid point in field 5 with fields 6 and 7 blank):
//   CBEND  EID  PID  GA  GB  X1  X2  X3  GEOM
//   CBEND  EID  PID  GA  GB  GO          GEOM
//
// Field 5 is an integer (GO) or a real (X1): `orientationNode` and `orientation` are the two forms. The orientation is
// required: a vector of length 0 means none, so exactly one of `orientationNode` and a non-zero `orientation` is given.
// GEOM (1 to 4) tells how the bend is defined (centre of curvature, a tangent point, ...); it has no default.
struct CurvedBeam2Card
{
    base::EntityId id = 0;
    base::EntityId propId = 0; // stored resolved: a blank PID is EID
    base::EntityId nodeA = 0;
    base::EntityId nodeB = 0;

    base::EntityId orientationNode = 0;     // the grid point that gives the orientation vector; 0: none
    std::array<double, 3> orientation = {}; // X1 X2 X3: the orientation vector; all zero: none

    std::int32_t geometryOption = 1; // 1 to 4

    [[nodiscard]] bool hasVector() const noexcept
    {
        return orientation[0] != 0.0 || orientation[1] != 0.0 || orientation[2] != 0.0;
    }
};

} // namespace oor::model
