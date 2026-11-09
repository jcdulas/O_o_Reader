#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One MODTRAK card: the mode tracking of a design optimization (SOL 200; selected by MODTRAK in Case Control).
//
// Card layout: MODTRAK  SID  LOWRNG  HIGHRNG  MTFILTER
// The designed modes are looked for among the modes LOWRNG to HIGHRNG; two modes are the same mode when their mass
// normalised cross orthogonality is greater than MTFILTER. A blank LOWRNG is 0, a blank HIGHRNG is the number of
// eigenvalues extracted (which the card does not know: 0 here, written blank), a blank MTFILTER is 0.9.
struct ModeTrackingCard
{
    base::EntityId id = 0;
    std::int32_t lowRange = 0;   // the lowest mode number of the search range
    std::int32_t highRange = 0;  // the highest one; 0 is blank
    double trackingFilter = 0.9; // the filtering parameter of the cross orthogonality check
};

} // namespace oor::model
