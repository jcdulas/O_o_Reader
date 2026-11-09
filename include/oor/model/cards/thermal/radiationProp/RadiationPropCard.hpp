#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

// One RADM card: the radiation properties of the surface elements that name it (RADMIDF and RADMIDB).
//
// Card layout: RADM  RADMID  ABSORP  EMIS1  EMIS2  EMIS3  EMIS4  EMIS5 -etc.-
// The absorptivity may be blank (a deck may give the emissivity only); the emissivities are one per
// wavelength band of a RADBND card (not typed), as many as the card has. A value is a fraction (or the multiplier of a
// temperature function curve), so it cannot be negative.
struct RadiationPropCard
{
    base::EntityId id = 0;
    std::optional<double> absorptivity;
    std::vector<double> emissivity;
};

} // namespace oor::model
