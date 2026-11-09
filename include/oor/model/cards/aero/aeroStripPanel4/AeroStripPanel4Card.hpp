#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// One CAERO4 card: an aerodynamic macro element for strip theory, a panel cut into strips that span the whole chord.
//
// Card layout: CAERO4  EID  PID  CP  NSPAN  LSPAN
//                     X1   Y1   Z1  X12    X4     Y4  Z4  X43
// The planform is the trapezoid of CAERO1: point 1 (X1, Y1, Z1) and point 4 (X4, Y4, Z4) are the ends of the leading
// edge, in the coordinate system CP (0 is the basic system); point 2 is X12 behind point 1 and point 3 X43 behind point
// 4, along the x axis of the aerodynamic coordinate system. A chord that is 0.0 makes a triangle; the
// two are never both 0.0. PID names a PAERO4 (the strip properties).
//
// The strips are NSPAN equal divisions of the span when NSPAN is positive, else the division points of the AEFACT LSPAN
// (fractions of the span, one more than the strips). LSPAN is used only when NSPAN is zero, and the card
// keeps both when a deck gives both, as CAERO1 does. The strips are numbered from EID on (they must not
// clash with the grid, scalar and extra points; a rule between cards that is not checked here), so the card owns one ID
// a strip (the ReferenceTarget::AeroBox of a spline).
//
// Fields 6 to 8 are blank cells of the Card layout: a value there, or after X43, is refused.
struct AeroStripPanel4Card
{
    base::EntityId id = 0;
    base::EntityId propId = 0;
    base::EntityId positionFrameId = 0;
    std::int32_t spanBoxCount = 0;
    base::EntityId spanDivisionListId = 0;
    std::array<double, 3> point1 = {}; // X1, Y1, Z1: the inboard end of the leading edge, in CP
    double rootChord = 0.0;
    std::array<double, 3> point4 = {}; // X4, Y4, Z4: the outboard end of the leading edge, in CP
    double tipChord = 0.0;
};

} // namespace oor::model
