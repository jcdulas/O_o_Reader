#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactRuns.hpp"

#include <vector>

namespace oor::model
{

// One TIC3 card (SOL 700): an initial velocity field of grid points made of a translation and a rotation about a grid
// point. As the TIC, it is selected by the IC command of Case Control and several cards share one set ID. Any number of
// TIC3 cards may be used; a grid point that no card of the set names has no initial velocity.
//
// Card layout, three lines (fields 4, 5 to 8, 15 and 16 are blank cells of the grid):
//   TIC3  SID  G  (blank)  SCALE                                       fields 1 to 4
//         XVEL  YVEL  ZVEL  XROT  YROT  ZROT                           fields 9 to 14
//         G1  G2  THRU  G3  BY  G4  ...                                fields 17 on, any number
// G is the grid point at the centre of rotation; the rotation components are radians per unit of time and
// reach the three rotations of a six degree of freedom grid point. SCALE is 1.0 when blank and scales the
// whole field, so it is a static default. The grid points are IDs, ranges "a THRU b" and "a THRU b BY n" (ContactRuns:
// a run is kept as a run, written on one line, and goes upwards); a blank field ends a list on the format, so
// the reader skips the padding of the continuation lines, and the list must be given. The ends of a THRU range are
// allowed not to exist (BY lets points be skipped: the format asks the points of the range to exist, and the reference
// check only looks at the ends).
// The keyword of a range is THRU, and no other spelling is read.
struct InitialVelocityFieldCard
{
    base::EntityId id = 0;   // the set ID that IC selects, shared by the cards of the set
    base::EntityId node = 0; // the centre of rotation
    double scaleFactor = 1.0;
    double xVelocity = 0.0; // the translation velocity
    double yVelocity = 0.0;
    double zVelocity = 0.0;
    double xRotation = 0.0; // the rotation velocity about G
    double yRotation = 0.0;
    double zRotation = 0.0;
    std::vector<ContactRun> grids; // the grid points that get the velocity
};

} // namespace oor::model
