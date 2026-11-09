#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One CBUSH card: a generalized spring-and-damper element.
//
// Card layout (orientation by vector, or by a grid point in field 5 with fields 6 and 7 blank):
//   CBUSH  EID  PID  GA   GB   X1   X2   X3   CID
//          S    OCID S1   S2   S3
//   CBUSH  EID  PID  GA   GB   GO             CID
//
// Field 5 is an integer (GO) or a real (X1): `orientationNode` and `orientation` are the two forms. Both can be blank,
// then the element axis is the line from GA to GB; a zero vector is how `orientation` says "no vector" (an orientation
// of length 0 means nothing), so `orientationNode == 0` with a zero `orientation` is that case. GB blank (0) is a
// grounded end.
struct BushingCard
{
    static constexpr base::EntityId NO_FRAME_ID = -1;        // CID blank: the element system comes from GO or X
    static constexpr base::EntityId NO_OFFSET_FRAME_ID = -1; // OCID blank or -1: the spring-damper is on the line GA-GB
    static constexpr double DEFAULT_S = 0.5;

    base::EntityId id = 0;
    base::EntityId propId = 0; // stored resolved: a blank PID is EID
    base::EntityId nodeA = 0;
    base::EntityId nodeB = 0;

    base::EntityId orientationNode = 0;     // the grid point that gives the orientation vector; 0: none
    std::array<double, 3> orientation = {}; // X1 X2 X3: the orientation vector; all zero: none

    base::EntityId frameId = NO_FRAME_ID; // NO_FRAME_ID: blank; 0 is the basic system
    double offsetFraction =
        DEFAULT_S; // location of the spring-damper along GA-GB, used when OCID is NO_OFFSET_FRAME_ID
    base::EntityId offsetFrameId = NO_OFFSET_FRAME_ID;
    std::array<double, 3> offset = {}; // S1 S2 S3: the spring-damper offset in the system OCID

    [[nodiscard]] bool hasVector() const noexcept
    {
        return orientation[0] != 0.0 || orientation[1] != 0.0 || orientation[2] != 0.0;
    }
};

} // namespace oor::model
