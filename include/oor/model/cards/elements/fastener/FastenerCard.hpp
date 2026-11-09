#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <string>

namespace oor::model
{

// One CFAST card: a fastener between two surface patches.
//
// Card layout:
//   CFAST  EID  PID  TYPE  IDA  IDB  GS  GA  GB
//          XS   YS   ZS
//
// TYPE is "PROP" (IDA and IDB are the IDs of two PSHELL or PCOMP properties that define the patches) or "ELEM" (they
// are the IDs of two shell elements). GS is the grid point that locates the fastener, GA and GB the piercing points
// on the two patches; all three may be blank (0). XS, YS and ZS locate the fastener in the basic system and are
// required when neither GS nor GA is given: they are all given or all blank (`location` is NaN, NaN, NaN then).
struct FastenerCard
{
    static constexpr double NO_COORDINATE = std::numeric_limits<double>::quiet_NaN();

    base::EntityId id = 0;
    base::EntityId propId = 0; // stored resolved: a blank PID is EID
    std::string kind = "Property";
    base::EntityId patchIdA = 0;
    base::EntityId patchIdB = 0;
    base::EntityId spotNode = 0;
    base::EntityId nodeA = 0;
    base::EntityId nodeB = 0;
    std::array<double, 3> location = {NO_COORDINATE, NO_COORDINATE, NO_COORDINATE}; // XS YS ZS

    [[nodiscard]] bool hasLocation() const noexcept
    {
        return !std::isnan(location[0]) || !std::isnan(location[1]) || !std::isnan(location[2]);
    }
};

} // namespace oor::model
