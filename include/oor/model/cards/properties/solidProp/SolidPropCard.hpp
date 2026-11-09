#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <string_view>

namespace oor::model
{

// One PSOLID card: the property of solid elements (CHEXA, CPENTA, CTETRA).
//
// Card layout:
//   PSOLID  PID  MID  CORDM  IN  STRESS  ISOP  FCTN
//
// IN, STRESS and ISOP take an integer or a word on the card; they are kept as the word (0 is BUBBLE, 2 is TWO, ...)
// and an empty text is a blank, whose meaning depends on the element type and so is not resolved. CORDM is the
// material coordinate system: 0 the basic system, -1 the element system, -2 the standard element system.
struct SolidPropCard
{
    static constexpr std::string_view DEFAULT_ELEMENT_FUNCTION = "SolidMechanics";

    base::EntityId id = 0;
    base::EntityId matId = 0;
    base::EntityId matFrameId = 0;
    std::string integration;                               // IN: BUBBLE, GAUSS, TWO or THREE; empty for blank
    std::string stressLocation;                            // STRESS: GRID or GAUSS; empty for blank
    std::string integrationOrder;                          // ISOP: REDUCED or FULL; empty for blank
    std::string elementFunction{DEFAULT_ELEMENT_FUNCTION}; // SMECH, PFLUID, FFLUID, PORO or PSLDSHL
};

} // namespace oor::model
