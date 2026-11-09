#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One CBEAM3 card: a three-node beam element (ends A and B and a middle point C).
//
// Card layout (orientation by vector, or by a grid point in field 6 with fields 7 and 8 blank):
//   CBEAM3  EID  PID  GA   GB   GC   X1   X2   X3
//           W1A  W2A  W3A  W1B  W2B  W3B  W1C  W2C
//           W3C  TWA  TWB  TWC  SA   SB   SC
//   CBEAM3  EID  PID  GA   GB   GC   GO
//           (the same continuation lines)
//
// Field 6 is an integer (GO) or a real (X1): `orientationNode` and `orientation` are the two forms; the orientation is
// required, so exactly one of `orientationNode` and a non-zero `orientation` is given (a vector of length 0 means none;
// GO cannot be GA, GB or GC). GC, SA, SB and SC may be blank (0: without GC the element is a two-node beam). SA, SB and
// SC are the grid or scalar points of the warping degrees of freedom at the three nodes.
struct Beam3Card
{
    base::EntityId id = 0;
    base::EntityId propId = 0;
    base::EntityId nodeA = 0;
    base::EntityId nodeB = 0;
    base::EntityId nodeC = 0; // 0: blank

    base::EntityId orientationNode = 0;     // the grid point that gives the orientation vector; 0: none
    std::array<double, 3> orientation = {}; // X1 X2 X3: the orientation vector; all zero: none

    std::array<double, 3> offsetA = {}; // W1A W2A W3A: offset vector at end A, in the displacement system of GA
    std::array<double, 3> offsetB = {}; // W1B W2B W3B: offset vector at end B
    std::array<double, 3> offsetC = {}; // W1C W2C W3C: offset vector at the middle point
    std::array<double, 3> twist = {};   // TWA TWB TWC: pretwist angles in degrees at A, B and C
    base::EntityId warpingPointA = 0;   // 0: blank
    base::EntityId warpingPointB = 0;
    base::EntityId warpingPointC = 0;

    [[nodiscard]] bool hasVector() const noexcept
    {
        return orientation[0] != 0.0 || orientation[1] != 0.0 || orientation[2] != 0.0;
    }
};

} // namespace oor::model
