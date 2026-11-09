#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <string>

namespace oor::model
{

// One MONPNT1 card: an integrated load monitor point. The loads over the grid points of an AECOMP or AECOMPL (COMP) are
// integrated about the point (X, Y, Z) of the system CP and printed in the system CD, for statics, dynamics and static
// aeroelastic trim. The COMP field says whether it is a structural or an aerodynamic monitor point; a structural and an
// aerodynamic point may have the same NAME, two of the same kind may not (the model knows the kind only once the
// AECOMP cards are typed, so a repeated NAME is not reported yet).
//
// Card layout: MONPNT1  NAME  LABEL (fields 3 to 9: up to 56 characters)
//                      .     AXES  COMP  CP  X  Y  Z  CD
// In this library's numbering (field 1 is NAME): LABEL is fields 2 to 8, AXES field 9, COMP 10, CP 11, X, Y and Z 12 to
// 14, CD 15. NAME and COMP are kept in upper case, the LABEL as written; AXES is a set of the digits 1 to 6 kept in
// ascending order; CP defaults to 0 and CD to CP, which is kept resolved and always written.
struct MonitorIntegratedLoad1Card
{
    std::string name;            // up to 8 characters
    std::string label;           // up to 56 characters, may be empty
    std::int32_t outputAxes = 0; // the components to monitor: digits 1 to 6, each at most once
    std::string component;       // COMP: the name of an AECOMP or AECOMPL entry
    base::EntityId positionFrameId = 0;
    double xCoord = 0.0;
    double yCoord = 0.0;
    double zCoord = 0.0;
    base::EntityId displacementFrameId = 0; // CD blank: the system of CP

    [[nodiscard]] bool operator==(const MonitorIntegratedLoad1Card&) const noexcept = default;
};

} // namespace oor::model
