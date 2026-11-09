#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// The ORIENT of a PAERO2: the motion the bodies may have in the aerodynamic coordinate system (the ACSID of the AERO
// card). Z allows motion in the z direction, Y in the y direction, ZY in both.
enum class BodyOrientation : std::uint8_t
{
    Z,
    Y,
    ZY
};

// The words of the format reference, in the order of the enumerators.
inline constexpr std::array<std::string_view, 3> AERO_BODY_PROP2_ORIENTATION_WORDS{"ZDirection", "YDirection",
                                                                                   "PlaneZy"};

// One PAERO2 card: the cross section of the CAERO2 bodies that name it, and the angles at which the interference
// elements receive.
//
// Card layout: PAERO2  PID   ORIENT  WIDTH  AR   LRSB  LRIB  LTH1  LTH2
//                     THI1  THN1    THI2   THN2
// A body is a slender body in an interference tube. WIDTH is the reference half-width of the body and the half-width of
// a tube of constant width, AR the aspect ratio of the tube (height over width); both are required and positive.
// LRSB and LRIB name AEFACT cards with the half-widths at the ends of the slender body elements and of the interference
// elements (one value more than the elements: a rule between cards); a blank or zero field means that
// WIDTH holds everywhere, and the card keeps both as 0. LTH1 and LTH2 name the AEFACT cards of the two arrays of angles
// (in degrees) at which the interference elements receive; each pair THIi, THNi gives the first and the last
// interference element of a body that use the first array, the other elements use the second (the elements of each
// body are counted from one). Blank is 0 for all of them. The format reference draws two pairs: this card
// refuses fields after THN2, since only two arrays of angles exist. A range ends at or after its first element.
//
// The second line is written only when a range is given: the format reference does not ask for it.
struct AeroBodyProp2Card
{
    base::EntityId id = 0;
    BodyOrientation orientation = BodyOrientation::Z;
    double width = 0.0;
    double aspectRatio = 0.0;
    base::EntityId slenderHalfWidthListId = 0;      // AEFACT: half-widths of the slender body elements, 0 when blank
    base::EntityId interferenceHalfWidthListId = 0; // AEFACT: half-widths of the interference elements, 0 when blank
    base::EntityId angleListId1 = 0;                // an AEFACT of angles, 0 when blank
    base::EntityId angleListId2 = 0;
    base::EntityId angleList1FirstElem = 0; // the first interference element of a range, 0 when blank
    base::EntityId angleList1LastElem = 0;  // the last one
    base::EntityId angleList2FirstElem = 0;
    base::EntityId angleList2LastElem = 0;
};

} // namespace oor::model
