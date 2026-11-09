#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// One CAERO1 card: an aerodynamic macro element (a panel of Doublet-Lattice or ZONA51 boxes) given by two leading edge
// points and the chords at them.
//
// Card layout: CAERO1  EID  PID  CP  NSPAN  NCHORD  LSPAN  LCHORD  IGID
//                     X1   Y1   Z1  X12    X4      Y4     Z4      X43
// The panel is the trapezoid 1-2-3-4: point 1 (X1, Y1, Z1) and point 4 (X4, Y4, Z4) are the inboard and outboard ends
// of the leading edge, given in the coordinate system CP (0 is the basic system); point 2 is X12 behind point 1 and
// point 3 X43 behind point 4, measured along the x axis of the aerodynamic coordinate system of the AERO or AEROS card.
// A chord that is 0.0 makes a triangle; the two are never both 0.0.
//
// The boxes are the panel divided by the division points of the span and the chord: NSPAN (NCHORD) equal divisions when
// it is positive, else the division points of the AEFACT LSPAN (LCHORD), which are fractions of the span (chord) and
// are one more than the boxes. The boxes are numbered from EID on, along the chord first and then span by span (box =
// EID + span index * chord boxes + chord index): the IDs EID to EID + boxes - 1 belong to this card (the
// ReferenceTarget::AeroBox of the splines and the AELIST cards). PID names a PAERO1 and is required even when it lists
// no body. IGID is the interference group: boxes of different groups do not interact.
//
// A blank NSPAN or NCHORD is 0 (the list is used); LSPAN and LCHORD are used only when the number of boxes is 0 (the
// card keeps both when both are given, as the format reference allows).
struct AeroLiftingPanel1Card
{
    base::EntityId id = 0;
    base::EntityId propId = 0;
    base::EntityId positionFrameId = 0;
    std::int32_t spanBoxCount = 0;
    std::int32_t chordBoxCount = 0;
    base::EntityId spanDivisionListId = 0;
    base::EntityId chordDivisionListId = 0;
    base::EntityId interferenceGroupId = 0;
    std::array<double, 3> point1 = {}; // X1, Y1, Z1: the inboard end of the leading edge, in CP
    double rootChord = 0.0;
    std::array<double, 3> point4 = {}; // X4, Y4, Z4: the outboard end of the leading edge, in CP
    double tipChord = 0.0;
};

} // namespace oor::model
