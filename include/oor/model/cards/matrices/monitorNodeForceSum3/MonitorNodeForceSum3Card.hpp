#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <string>

namespace oor::model
{

// One MONPNT3 card: an integrated load monitor point that sums selected grid point forces. The forces at the grids of
// the SET1 GRIDSET, from the elements of the SET1 ELEMSET (none when blank) and the loads that XFLAG does not exclude,
// are summed about the point (X, Y, Z) of the system CP and printed in the system CD. The MONPNT3 is how shear, moment
// and torque of a cut of a structure are read.
//
// Card layout: MONPNT3  NAME  LABEL (fields 3 to 9: up to 56 characters)
//                      .     AXES  GRIDSET  ELEMSET  CP  X  Y  Z  XFLAG
//                      .     CD
// In this library's numbering (field 1 is NAME): LABEL is fields 2 to 8, AXES field 9, GRIDSET 10, ELEMSET 11, CP 12,
// X, Y and Z 13 to 15, XFLAG 16, CD 17. NAME is kept in upper case, the LABEL as written; AXES is a set of the digits 1
// to 6 in ascending order; XFLAG is a list of the letters S (SPC forces), M (MPC forces), A, L or P (applied loads), D
// (matrices and other types) and C (contact forces, SOL 400), upper case, blank for none excluded. CP defaults to 0 and
// CD to CP, which is kept resolved and always written.
struct MonitorNodeForceSum3Card
{
    std::string name;              // up to 8 characters
    std::string label;             // up to 56 characters, may be empty
    std::int32_t outputAxes = 0;   // the components to sum: digits 1 to 6, each at most once
    base::EntityId nodeSetId = 0;  // the SET1 that lists the grid points
    base::EntityId elementSet = 0; // the SET1 that lists the elements, 0 when blank
    base::EntityId positionFrameId = 0;
    double xCoord = 0.0;
    double yCoord = 0.0;
    double zCoord = 0.0;
    std::string excluded;                   // XFLAG
    base::EntityId displacementFrameId = 0; // CD blank: the system of CP

    [[nodiscard]] bool operator==(const MonitorNodeForceSum3Card&) const noexcept = default;
};

} // namespace oor::model
