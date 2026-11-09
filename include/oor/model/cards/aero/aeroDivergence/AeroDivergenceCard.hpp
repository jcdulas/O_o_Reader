#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One DIVERG card: the Mach numbers of a static aeroelastic divergence analysis, selected by the Case Control command
// DIVERG = SID.
//
// Card layout: DIVERG  SID  NROOT  M1  M2  M3  M4  M5  M6
//                     M7   M8  -etc.-
// NROOT is the number of the lowest divergence dynamic pressures that are printed, with their eigenvectors (all of them
// when there are fewer); the format reference gives the default 1 for a blank field and no other limit, and a count of
// roots is at least one here. The Mach numbers follow from field 3, over as many continuation lines as needed: at least
// one, 0.0 or more, all different. A blank field ends the list, so a number after a blank would
// be lost and is refused.
struct AeroDivergenceCard
{
    base::EntityId id = 0;
    std::int32_t rootCount = 1;
    std::vector<double> machs;
};

} // namespace oor::model
