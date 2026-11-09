#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One CPLSTN4 card: a four-node plane strain element (a card of another implementation: it is not in the format
// reference).
//
// Layout:
//   CPLSTN4  EID  PID  G1  G2  G3  G4  THETA
struct PlaneStrainQuad4Card
{
    base::EntityId id = 0;
    base::EntityId propId = 0; // stored resolved: a blank PID is EID
    std::array<base::EntityId, 4> nodes = {};
    double thetaInDegrees = 0.0; // material property orientation angle
};

} // namespace oor::model
