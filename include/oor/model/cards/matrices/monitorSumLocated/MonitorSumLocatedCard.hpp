#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/matrices/MonitorSumTerms.hpp"

#include <string>

namespace oor::model
{

// One MONSUM1 card: a new monitor result that is the weighted sum of existing monitor results, with a location. The
// location (CP, X, Y, Z) and the output system CD are for reference only and do not change the sum.
// Only the force and moment monitor results can be summed (AMONPNT1, SMONPNT1 and MONPNT3).
//
// Card layout:  MONSUM1  NAME  LABEL (fields 3 to 9: up to 56 characters)
//                       CP  X  Y  Z  CD
//                       NEWAXES  MTYP1  NAME1A  AXES1A  COEF1A  NAME2A  AXES2A  COEF2A
//                                       NAME3A  AXES3A  COEF3A  etc.
//                                MTYP2  NAME2B  ...
// and the alternate layout, whose lines have the MTYPE first and the NEWAXISj second (`MonitorSumTerms.hpp`). In this
// library's numbering (field 1 is NAME): LABEL is fields 2 to 8, CP, X, Y, Z and CD are fields 9 to 13 of the line
// after the head, and the terms start on the line after that. CP defaults to 0 and CD to CP, which is kept resolved and
// always written; a blank AXES is the NEWAXES and a blank COEF is 1.0 as for a MONSUM.
//
// The location line ends with CD: no field of the layout follows it, so the card refuses a value beyond CD. NAME
// and the names inside the card name monitor results, not IDs: they are kept in upper case and no reference is visited
// for them. CP and CD are coordinate systems.
struct MonitorSumLocatedCard
{
    std::string name;  // up to 8 characters, upper case
    std::string label; // up to 56 characters, may be empty
    base::EntityId positionFrameId = 0;
    double xCoord = 0.0;
    double yCoord = 0.0;
    double zCoord = 0.0;
    base::EntityId displacementFrameId = 0; // CD blank: the system of CP
    MonitorSumBody body;

    [[nodiscard]] bool operator==(const MonitorSumLocatedCard&) const noexcept = default;
};

} // namespace oor::model
