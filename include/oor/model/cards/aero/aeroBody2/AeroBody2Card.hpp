#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// One CAERO2 card: an aerodynamic body (a slender body surrounded by an interference tube) for the Doublet-Lattice
// method, given by its leading point and its length.
//
// Card layout: CAERO2  EID  PID  CP  NSB  NINT  LSB  LINT  IGID
//                     X1   Y1   Z1  X12
// Point 1 (X1, Y1, Z1) is the nose of the body, in the coordinate system CP (0 is the basic system); the body runs X12
// behind it along the x axis of the aerodynamic coordinate system of the AERO or AEROS card. PID names a PAERO2, which
// gives the half-widths. IGID is the interference group: elements of different groups do not interact. The format
// reference allows zero for IGID (CAERO1 wants a positive one) and gives it no default, so the field is required but
// may be 0.
//
// The body is cut in two ways, each with its own pair of fields. The slender body elements are NSB equal divisions
// when NSB is positive, else the division points of the AEFACT LSB; the interference elements are NINT equal
// divisions, else the points of the AEFACT LINT (a list has one point more than the elements it cuts). A body needs
// both: NSB or LSB, and NINT or LINT (the format asks for one interference element at least). Like the CAERO1
// lists, LSB and LINT are used only when their number is zero, and the card keeps both when a deck gives both.
//
// The beams and connection points are numbered sequentially from EID on, so the card owns NSB + NINT IDs
// when both are numbers, and one less than the points of each list otherwise (the ReferenceTarget::AeroBox of a
// spline). The rules that need other cards are not checked here: a body needs at least two slender body elements
// (which a list gives through its AEFACT), and the AEFACT of the half-widths in the PAERO2 has one value
// more than the elements it describes.
//
// X12 is the length of the body and is positive. A blank X1, Y1 or Z1 is 0.0, as for CAERO1.
struct AeroBody2Card
{
    base::EntityId id = 0;
    base::EntityId propId = 0;
    base::EntityId positionFrameId = 0;
    std::int32_t slenderElemCount = 0;
    std::int32_t interferenceElemCount = 0;
    base::EntityId slenderListId = 0;
    base::EntityId interferenceListId = 0;
    base::EntityId interferenceGroupId = 0;
    std::array<double, 3> point1 = {}; // X1, Y1, Z1: the nose of the body, in CP
    double length = 0.0;
};

} // namespace oor::model
