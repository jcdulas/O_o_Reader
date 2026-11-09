#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One CAERO3 card: the edges of an aerodynamic surface for the Mach box method (SOL 145, 146 and 200 only).
// The card gives the planform; the cranks of the edges and the control surfaces are on the PAERO3 that PID names.
//
// Card layout: CAERO3  EID  PID  CP  LISTW  LISTC1  LISTC2
//                     X1   Y1   Z1  X12    X4      Y4      Z4  X43
// The planform is the trapezoid of CAERO1: point 1 (X1, Y1, Z1) and point 4 (X4, Y4, Z4) are the ends of the leading
// edge, in the coordinate system CP (0 is the basic system); point 2 is X12 behind point 1 and point 3 X43 behind point
// 4, along the x axis of the aerodynamic coordinate system. X12 is positive, X43 is not negative (it may be 0.0, a
// triangle); the two are not both zero.
//
// The Mach boxes are not part of the model: the card lists the aerodynamic grid points, which do not depend on the Mach
// number. LISTW is the AEFACT whose values are the (x, y) pairs of the points that carry the structural interpolation
// of the wing, in the system of the element, and LISTC1 and LISTC2 are the AEFACT cards of the points of
// the first and second control surface, when there are any (blank is 0: the card keeps zeros). The points are numbered
// from EID on, those of LISTW first, then LISTC1, then LISTC2: the card owns one ID a pair (the
// ReferenceTarget::AeroBox of a SPLINE1, SPLINE2 or SPLINE3). The rules between cards are not checked here: each list
// holds an even number of values, LISTC1 is given for one control surface and both lists for two (the NCTRL of the
// PAERO3), and the EID is unique among the aerodynamic elements (the format says all the elements; the model keeps the
// CAERO1 to CAERO5 together and apart from the structural elements, as CAERO1 does).
//
// Fields 7 and 8 are blank cells of the Card layout: a value there, or after X43, is refused. A blank X1, Y1, Z1, X4,
// Y4, Z4 or X43 is 0.0, as for the other CAERO cards.
struct AeroMachBoxPanel3Card
{
    base::EntityId id = 0;
    base::EntityId propId = 0;
    base::EntityId positionFrameId = 0;
    base::EntityId wingPointListId = 0;
    base::EntityId controlPointListId1 = 0; // 0 when blank
    base::EntityId controlPointListId2 = 0; // 0 when blank
    std::array<double, 3> point1 = {};      // X1, Y1, Z1: the inboard end of the leading edge, in CP
    double rootChord = 0.0;
    std::array<double, 3> point4 = {}; // X4, Y4, Z4: the outboard end of the leading edge, in CP
    double tipChord = 0.0;
};

} // namespace oor::model
