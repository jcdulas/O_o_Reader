#pragma once

#include "oor/model/design/DesignableCard.hpp"

namespace oor::model
{

// The fields of a PBEAML that a DVPREL1 drives (by name only: the format reference says a PBEAML supports PNAME and not
// FID, and does not let a DVPREL2 name it): DIM1, DIM2, ... and NSM, each at a station: DIM2 and DIM2(A) are end A,
// DIM2(B) the last station of the card (X/XB = 1.0), DIM2(1), DIM2(2), ... the stations after end A in the order of the
// card (the last one included).
[[nodiscard]] const DesignableCard& beamShapePropDesignFields();

} // namespace oor::model
