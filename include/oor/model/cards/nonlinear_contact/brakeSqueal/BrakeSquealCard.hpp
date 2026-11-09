#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/BrakeKeywords.hpp"

#include <optional>

namespace oor::model
{

// One BSQUEAL card (SOL 400): the data of a brake squeal analysis of one wheel, or of another rotating object in
// contact with friction between sets of bodies (unsymmetric friction stiffness matrices).
//
// Card layout: BSQUEAL  ID  OMETH  AVSTIF  (blank)  (blank)  BSONLY                  fields 1 to 6 of the first line
//                      RX  RY  RZ  X  Y  Z                                          fields 9 to 14 of the second line
// BSONLY is the seventh field of the layout of the format, field 6 in this library's numbering, and the second line is
// a line of its own. ID is the number that a Case Control BSQUEAL selects (it is the
// ID of a BRKSYS card too: `contact.brake-squeal-id-repeated`). OMETH is the load factor (or time) of the analysis
// (real, default 0.0; the format gives the range of a static analysis, 0.0 < OMETH < 1.0, which depends on the SUBCASE
// and is not checked here); AVSTIF the penalty stiffness per unit area between the two surfaces (real, required);
// BSONLY is YES (default) or NO. RX, RY and RZ are the direction cosines of the axis of rotation in the basic system
// and X, Y and Z a point of the axis; the format takes a real or a blank with no default, so each may stay blank and
// the card keeps a blank one blank.
//
// The format gives no range, so AVSTIF must be greater than zero (it is a stiffness); OMETH, the cosines and the
// coordinates must be finite.
struct BrakeSquealCard
{
    base::EntityId id = 0;
    double loadFactor = 0.0;
    double areaStiffness = 0.0;
    BrakeSquealOnly squealOnly = BrakeSquealOnly::Yes;
    std::optional<double> axisX; // X direction cosine of the axis of rotation
    std::optional<double> axisY;
    std::optional<double> axisZ;
    std::optional<double> xCoord; // X coordinate of a point of the axis of rotation
    std::optional<double> yCoord;
    std::optional<double> zCoord;

    [[nodiscard]] bool operator==(const BrakeSquealCard&) const noexcept = default;
};

} // namespace oor::model
