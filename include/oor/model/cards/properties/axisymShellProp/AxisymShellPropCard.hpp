#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// The analysis a PAXISYM is for (the ANAL field): structural, heat transfer or both. The enumerators are numbered in
// the order of AXISYM_SHELL_PROP_ANALYSIS_WORDS.
enum class AxisymShellPropAnalysis : std::uint8_t
{
    Structural,    // IS
    Heat,          // IH
    StructuralHeat // ISH (the default)
};

inline constexpr std::array<std::string_view, 3> AXISYM_SHELL_PROP_ANALYSIS_WORDS{"Structural", "Heat",
                                                                                  "StructuralAndHeat"};

// One PAXISYM card (SOL 400): the property of the axisymmetric line element CAXISYM.
//
// Card layout:  PAXISYM  PID  MID  (blank)  T1  T2  ANAL        fields 1 to 6
// Field 3 is a blank cell of the layout, and T1 is field 4. PID is a
// property ID, which must be unique among all the properties: the card is a primary property. MID is a
// structural material (MAT1, MAT2, MAT3, MAT8, MATORT or MATHE) or, for heat transfer, a MAT4 or a MAT5: the
// reference is to the material space, which falls back to the thermal materials when no structural one has the ID.
// T1 is the thickness at G1 (greater than 0.0) and has no default, so it is required. T2 is the thickness at G2 (the
// format says 0.0 or more): for an element with two grid points only T1 counts, and for a three-point element a T2 that
// is blank or 0.0 means a constant thickness, so a blank T2 is stored as 0.0 and written as a blank. ANAL is
// IS, IH or ISH and is ISH when blank. That PAXISYM is not supported in the NLPERF SOL 400 and
// the absence of composite materials are rules of the solution that the card does not look at.
struct AxisymShellPropCard
{
    base::EntityId id = 0;
    base::EntityId matId = 0;
    double thickness1 = 0.0;
    double thickness2 = 0.0; // 0.0 when blank: a constant thickness
    AxisymShellPropAnalysis analysisType = AxisymShellPropAnalysis::StructuralHeat;
};

} // namespace oor::model
