#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/matrices/MonitorSumTerms.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace oor::model
{

// A monitor result that a MONSUMT adds: its kind and its NAME.
struct MonitorSumTotalMember
{
    MonitorResultType type = MonitorResultType::AeroPoint;
    std::string name; // upper case

    [[nodiscard]] bool operator==(const MonitorSumTotalMember&) const noexcept = default;
};

// One MONSUMT card: a new monitor result that is the sum of existing force and moment monitor results (AMONPNT1,
// SMONPNT1 and MONPNT3) with the moments transferred to the point (X, Y, Z) of the system CP (the sum
// of the partial rigid body vectors of the monitor points). The sum fills the components AXES of the new result in the
// system CD.
//
// Card layout: MONSUMT  NAME  LABEL (fields 3 to 9: up to 56 characters)
//                      AXES  CP  X  Y  Z  CD
//                      MTYP1  NAME1a  NAME1b  NAME1c  NAME1d  NAME1e  NAME1f  NAME1g
//                      MTYP2  NAME2a  ...
// In this library's numbering (field 1 is NAME): LABEL is fields 2 to 8, AXES, CP, X, Y, Z and CD are fields 9 to 14 of
// the line after the head, and each line after that holds an MTYP in its first field and up to seven names in the
// others. A line whose MTYP is blank goes on with the type above it (the format draws no such line, but a type that has
// more than seven names needs one). The card keeps the members in one list in the order of the lines; the members of
// one type in a row are written under one MTYP, seven to a line. CP defaults to 0 and CD to CP, which is kept resolved
// and always written.
//
// A class word (AMONPNT1, MONPNT3, ...) in a name field is read as the name of a monitor point, as the layout has it.
// The sum can name the results of another MONSUMT or a MONSUM1, so a name may be one of these cards; no reference is
// visited for a name, and NAME is kept in upper case. CP and CD are coordinate systems.
struct MonitorSumTotalCard
{
    std::string entryName;       // up to 8 characters, upper case
    std::string label;           // up to 56 characters, may be empty
    std::int32_t outputAxes = 0; // the components of the new result: digits 1 to 6, each at most once
    base::EntityId positionFrameId = 0;
    double xCoord = 0.0;
    double yCoord = 0.0;
    double zCoord = 0.0;
    base::EntityId displacementFrameId = 0; // CD blank: the system of CP
    std::vector<MonitorSumTotalMember> members;

    [[nodiscard]] bool operator==(const MonitorSumTotalCard&) const noexcept = default;
};

} // namespace oor::model
