#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One CGAP card: a gap or friction element between two grid points.
//
// Card layout (orientation by vector, or by a grid point in field 5 with fields 6 and 7 blank):
//   CGAP  EID  PID  GA  GB  X1  X2  X3  CID
//   CGAP  EID  PID  GA  GB  GO          CID
//
// Field 5 is an integer (GO) or a real (X1): `orientationNode` and `orientation` are the two forms. A zero vector is
// how `orientation` says "no vector" (an orientation of length 0 means nothing), so `orientationNode == 0` with a zero
// `orientation` is the blank form, where the element axis is the line from GA to GB. X2 and X3 are read whether or not
// X1 is there (a deck that gives only X3 means the vector (0, 0, X3)). CID is the element coordinate system and must be
// given when GA and GB are coincident.
struct Gap2Card
{
    static constexpr base::EntityId NO_FRAME_ID = -1; // CID blank: the element system comes from GO or X

    base::EntityId id = 0;
    base::EntityId propId = 0; // stored resolved: a blank PID is EID
    base::EntityId nodeA = 0;
    base::EntityId nodeB = 0;

    base::EntityId orientationNode = 0;     // the grid point that gives the orientation vector; 0: none
    std::array<double, 3> orientation = {}; // X1 X2 X3: the orientation vector; all zero: none

    base::EntityId frameId = NO_FRAME_ID; // NO_FRAME_ID: blank; 0 is the basic system

    [[nodiscard]] bool hasVector() const noexcept
    {
        return orientation[0] != 0.0 || orientation[1] != 0.0 || orientation[2] != 0.0;
    }
};

} // namespace oor::model
