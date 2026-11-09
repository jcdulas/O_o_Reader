#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One RADMT card: the tables that make the radiation properties of a RADM depend on temperature. The card is a
// companion of the RADM of the same RADMID, as a MATT1 is of a MAT1: the quantity on the RADM is always multiplied by
// the value of the table, so the tables are factors that stay between 0 and 1.
//
// Card layout: RADMT  RADMID  T(A)  T(e1)  T(e2)  T(e3)  T(e4)  T(e5)  T(e6)        fields 1 to 8
//                    T(e7)   -etc.-                                                 fields 9 on
// T(A) is the TABLEMj of the absorptivity and T(ei) that of the emissivity of the wave band i (the bands of the
// RADBND). A table field is blank or zero for a property that does not depend on temperature, and both are 0
// here, so the position in the list is kept: the table of band 3 is the third value even when bands 1 and 2 have none.
// The list goes up to the last field that is not blank (a trailing zero is not kept). The ID of the RADM is the ID of
// the card (IdSpace::PerCardType: one RADMT for each RADM); how many tables the list may have is the NUMBER of the
// RADBND, a rule between cards.
struct RadiationPropTemperatureCard
{
    base::EntityId id = 0;
    base::EntityId absorptivityTableId = 0;         // the TABLEMj of the absorptivity, 0 when blank
    std::vector<base::EntityId> emissivityTableIds; // the TABLEMj of the emissivity of each band, 0 where blank

    [[nodiscard]] bool operator==(const RadiationPropTemperatureCard&) const noexcept = default;
};

} // namespace oor::model
