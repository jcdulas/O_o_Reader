#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One ROTORSE card: the rotor line model of a rotor that is placed in a superelement (the alternative to a ROTORG when
// superelements are used).
//
// Card layout: ROTORSE  ROTORID  SEID  SEOPT
// SEOPT says how the gyroscopic terms are calculated: 1 takes the A-set matrices of the superelement (a 3D rotor model
// in the superelement whose boundary is the collinear rotor line), 2 takes its G-set matrices (the superelement is the
// line model itself). A blank SEOPT is 1, a default that does not depend on another field, so it is stored as 1 and
// written as a blank. The ROTORID is shared with ROTORG, ROTOR and ROTORAX in the category Rotor, and the format
// reference asks the ROTORIDs of the ROTORSE, ROTORAX and ROTORG cards to be different (the check
// `compat.rotor-id-repeated`).
struct RotorSuperelementCard
{
    static constexpr std::int32_t DEFAULT_SUPERELEMENT_OPTION = 1;

    base::EntityId id = 0;
    base::EntityId superelementId = 0; // the superelement that holds the rotor
    std::int32_t superelementOption = DEFAULT_SUPERELEMENT_OPTION;
};

} // namespace oor::model
