#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One CQUADX4 card: a four-node axisymmetric quadrilateral ring element (a card of another implementation: it is not in
// the format reference).
//
// Layout:
//   CQUADX4  EID  PID  G1  G2  G3  G4  THETA
struct AxisymQuad4Card
{
    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, 4> nodes = {};
    double thetaInDegrees = 0.0; // material property orientation angle
};

} // namespace oor::model
