#pragma once

#include "oor/model/design/DesignableCard.hpp"

namespace oor::model
{

// The fields of a PCOMP that a DVPREL1 or DVPREL2 drives: Z0, NSM, SB, TREF and GE, and the thickness and angle of a
// ply, T1, T2, ... and THETA1, THETA2, ..., numbered in the order of the card (for a symmetric laminate only the plies
// the card gives exist). By position Z0 is field 3, NSM 4, SB 5, TREF 7, GE 8, and the plies start at field 12, two to
// a line with four fields each (MID, T, THETA, SOUT): the thickness of the first ply is field 13, its angle 14, the
// thickness of the second ply 17.
[[nodiscard]] const DesignableCard& compPropDesignFields();

} // namespace oor::model
