#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>

namespace oor::model
{

// One FFCONTR card (SOL 700): the pressure inside a closed volume that holds a fluid and a gas, for containers where
// the motion of the fluid does not matter (loading a bottle from the top, hot filling). The fluid is incompressible and
// the gas above it is ideal and isothermal (p*V constant), or follows the temperature when a table of it is given. The
// pressure is uniform in the volume and varies with time, as for a gas bag.
//
// Card layout: FFCONTR  FFID  SID  FVOL  PATM  TEMPTAB  DENSTAB  TACTIVE            fields 1 to 7
// SID is the BSURF, BCBOX, BCPROP, BCMATL or BCSEG that bounds the volume (its normals are reversed when they have to
// be). FVOL is the volume of fluid and PATM the atmospheric pressure, from which the constant of the gas law comes.
// TEMPTAB is a TABLED1 of the temperature of the container against time and DENSTAB a TABLED1 of the fluid density
// against temperature (the volume of the fluid then follows it; without a TEMPTAB the DENSTAB is not used). TACTIVE is
// the time at which the container is closed: before it the pressure inside is the ambient one. It has no default on the
// format, so a blank one stays blank (None in Python).
//
// The format gives FVOL as zero or more in the description and PATM as positive; the card reads them so.
struct FluidFilledContainerCard
{
    base::EntityId id = 0;
    base::EntityId surfaceId = 0;          // the closed surface
    double fluidVolume = 0.0;              // the volume of fluid, not negative
    double atmosphericPressure = 0.0;      // the atmospheric pressure, greater than zero
    base::EntityId temperatureTableId = 0; // a TABLED1, 0 when blank
    base::EntityId densityTableId = 0;     // a TABLED1, 0 when blank
    std::optional<double> activationTime;
};

} // namespace oor::model
