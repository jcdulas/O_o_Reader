#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One CPLSTN3 card: a three-node plane strain element (a card of another implementation: it is not in the format
// reference).
//
// Layout:
//   CPLSTN3  EID  PID  G1  G2  G3  THETA
struct PlaneStrainTria3Card
{
    base::EntityId id = 0;
    base::EntityId propId = 0; // stored resolved: a blank PID is EID
    std::array<base::EntityId, 3> nodes = {};
    double thetaInDegrees = 0.0; // material property orientation angle
};

} // namespace oor::model
