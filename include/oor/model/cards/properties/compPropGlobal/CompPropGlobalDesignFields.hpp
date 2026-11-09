#pragma once

#include "oor/model/design/DesignableCard.hpp"

namespace oor::model
{

// The fields of a PCOMPG that a DVPREL1 or DVPREL2 drives: Z0, NSM, SB, TREF and GE, and the thickness and angle of a
// ply, T1, T2, ... and THETA1, THETA2, ..., numbered in the order of the card. By position Z0 is field 3, NSM 4, SB 5,
// TREF 7, GE 8, and the plies are one to a line (global ply ID, MID, T, THETA, SOUT, GEFLG) from field 12 on the second
// line, 22 on the third, ...: the thickness of the first ply is field 14, its angle 15, the thickness of the second ply
// 24. A ply designed by its global ply ID (a DVPREL1 of TYPE GPLY) is not applied.
[[nodiscard]] const DesignableCard& compPropGlobalDesignFields();

} // namespace oor::model
