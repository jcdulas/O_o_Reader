#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// One CAERO5 card: an aerodynamic macro element for piston theory, a panel cut into strips that span the whole chord.
//
// Card layout: CAERO5  EID  PID  CP  NSPAN  LSPAN  NTHRY  NTHICK
//                     X1   Y1   Z1  X12    X4     Y4     Z4      X43
// The planform, the strips and their numbering are those of CAERO4: point 1 (X1, Y1, Z1) and point 4 (X4, Y4, Z4) are
// the ends of the leading edge in the system CP (0 is the basic system), point 2 is X12 behind point 1 and point 3 X43
// behind point 4 along the aerodynamic x axis, a chord of 0.0 makes a triangle and the two are never both 0.0. NSPAN
// equal strips when it is positive, else the division points of the AEFACT LSPAN. PID names a PAERO5.
//
// NTHRY selects the theory that gives the aerodynamic coefficients: 0 or blank is piston theory, 1 is van Dyke's theory
// without a sweep correction, 2 is van Dyke's theory with the correction for the sweep of the leading edge. Another
// passage of the format pairs 0 and 1 the other way round; the description of the field is the one followed, and the
// card only keeps the number. NTHICK is the ID of an AEFACT that gives the thickness integrals directly; zero or blank
// means the integrals are computed from the PAERO5 data, and the card keeps 0.
//
// Field 8 is a blank cell of the Card layout: a value there, or after X43, is refused.
struct AeroPistonPanel5Card
{
    base::EntityId id = 0;
    base::EntityId propId = 0;
    base::EntityId positionFrameId = 0;
    std::int32_t spanBoxCount = 0;
    base::EntityId spanDivisionListId = 0;
    std::int32_t theory = 0;                    // 0 (piston theory), 1 or 2 (van Dyke's theory)
    base::EntityId thicknessIntegralListId = 0; // an AEFACT of thickness integrals, 0 when blank
    std::array<double, 3> point1 = {};          // X1, Y1, Z1: the inboard end of the leading edge, in CP
    double rootChord = 0.0;
    std::array<double, 3> point4 = {}; // X4, Y4, Z4: the outboard end of the leading edge, in CP
    double tipChord = 0.0;
};

} // namespace oor::model
