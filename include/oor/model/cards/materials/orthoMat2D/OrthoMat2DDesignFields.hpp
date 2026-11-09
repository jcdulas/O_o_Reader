#pragma once

#include "oor/model/design/DesignableCard.hpp"

namespace oor::model
{

// The fields of a MAT8 that a DVMREL1 or DVMREL2 drives, by the name the format reference gives them: E1, E2, NU12,
// G12, G1Z, G2Z, RHO, A1, A2, TREF, XT, XC, YT, YC, S, GE, F12 and STRN. XC and YC blank are XT and YT; the card stores
// them resolved and a design of XT does not move them.
[[nodiscard]] const DesignableCard& orthoMat2DDesignFields();

} // namespace oor::model
