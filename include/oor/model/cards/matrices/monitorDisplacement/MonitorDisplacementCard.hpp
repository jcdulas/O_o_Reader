#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <string>

namespace oor::model
{

// One MONDSP1 card: a displacement monitor point. A virtual point at (X, Y, Z) of the system CP moves as the weighted
// average of the grid points of an AECOMP or AECOMPL (COMP), as the dependent point of a virtual RBE3 would (INDDOF are
// the components of the grids that are sampled). The displacement is printed in the system CD. Like a MONPNT1 it is
// structural or aerodynamic by its COMP, and a structural and an aerodynamic point may share a NAME (not reported yet:
// the AECOMP cards are typed).
//
// Card layout: MONDSP1  NAME  LABEL (fields 3 to 9: up to 56 characters)
//                      .     AXES  COMP  CP  X  Y  Z  CD  INDDOF
// In this library's numbering (field 1 is NAME): LABEL is fields 2 to 8, AXES field 9, COMP 10, CP 11, X, Y and Z 12 to
// 14, CD 15, INDDOF 16. NAME and COMP are kept in upper case, the LABEL as written; AXES and INDDOF are sets of the
// digits 1 to 6 in ascending order. CP defaults to 0, CD to CP (kept resolved and always written) and INDDOF to 123.
struct MonitorDisplacementCard
{
    static constexpr std::int32_t DEFAULT_SAMPLED_COMPONENTS = 123;

    std::string name;            // up to 8 characters
    std::string label;           // up to 56 characters, may be empty
    std::int32_t outputAxes = 0; // the components to monitor: digits 1 to 6, each at most once
    std::string component;       // COMP: the name of an AECOMP or AECOMPL entry
    base::EntityId positionFrameId = 0;
    double xCoord = 0.0;
    double yCoord = 0.0;
    double zCoord = 0.0;
    base::EntityId displacementFrameId = 0;                      // CD blank: the system of CP
    std::int32_t sampledComponents = DEFAULT_SAMPLED_COMPONENTS; // the components of the grids that define the point

    [[nodiscard]] bool operator==(const MonitorDisplacementCard&) const noexcept = default;
};

} // namespace oor::model
