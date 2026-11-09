#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One SPHERE card (SOL 700): a spherical shape that a TICEUL1 region (a TICREG of type SPHERE) or an EULFREG region of
// type SPHERE uses to give initial conditions or an acceleration field to the Eulerian elements inside it.
//
// Card layout: SPHERE  VID  X  Y  Z  RADIUS            fields 1 to 5
// The centre is the origin when blank. Only this entry of the format reference is typed here: the shapes of the initial
// conditions are named by the VID of the region cards, whose TYPE (SURF, SPHERE, CYLINDER, BOX, ELEM) tells which card
// holds the shape. SPHERE has no other entry of the same name, and no keyword tells it from another form.
struct SphereShapeCard
{
    base::EntityId id = 0; // unique among the shapes of an Euler region
    double centerX = 0.0;  // the centre
    double centerY = 0.0;
    double centerZ = 0.0;
    double radius = 0.0; // greater than zero
};

} // namespace oor::model
