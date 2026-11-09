#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/thermal/ThermalRuns.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One group of a TEMPBC card: a temperature and the grid points that have it, a single point (first == last) or "first
// THRU last" with an optional "BY n".
struct TemperatureBoundaryGroup
{
    double temperature = 0.0;
    ThermalRun grids;

    [[nodiscard]] bool operator==(const TemperatureBoundaryGroup&) const noexcept = default;
};

// One TEMPBC card: the temperature boundary conditions of a heat transfer analysis.
//
// Card layout: TEMPBC  SID  TYPE  TEMP1  GID1  TEMP2  GID2  TEMP3  GID3
//                     TEMP4  GID4  -etc.-
// or with a range: ... TEMP1 GID1 "THRU" GID2 "BY" INC. TYPE is STAT (blank: constant, selected by `SPC = SID` in the
// Case Control) or TRAN (time varying: a TLOADi names SID as its EXCITEID, transient analysis only). The groups
// continue over as many lines as needed.
struct TemperatureBoundaryCard
{
    base::EntityId id = 0;
    std::string kind = "Static";
    std::vector<TemperatureBoundaryGroup> groups;
};

} // namespace oor::model
