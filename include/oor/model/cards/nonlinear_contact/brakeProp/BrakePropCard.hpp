#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>

namespace oor::model
{

// One BRKPROP card (SOL 400): the property of a brake system for the brake squeal analysis, named by a BRKSYS.
//
// Card layout: BRKPROP  ID  (blank)  PARAM1  VALUE1  PARAM2  VALUE2  PARAM3  VALUE3        fields 1 to 8
//                          PARAM4  VALUE4  -etc.-                                          fields 9 to 16, and so on
// The card is a list of NAME and VALUE pairs that starts in field 3 (field 2 is blank) and goes on from the first
// field of each continuation line. The format names three parameters, and a deck may give them in any
// order (the card keeps the values, not the order): AVSTIF (the penalty stiffness per unit area between
// the two surfaces: real, required when TAVSTIF is 0), TAVSTIF (a TABL3Di that gives it as a function of temperature or
// position: integer, 0 when blank) and FRICT (the friction coefficient: real, 0.0 when blank takes it from the contact
// table). A parameter is given once.
//
// Choices of the card: the format gives no range, so AVSTIF must be greater than zero (a penalty stiffness) and FRICT
// may not be negative, as for the physical quantity they are. A parameter name that the format does not list is refused
// (the card stays text in tolerant mode).
struct BrakePropCard
{
    base::EntityId id = 0;                   // the number that a BRKSYS names in BProp_ID
    std::optional<double> areaStiffness;     // AVSTIF: blank when TAVSTIF gives the stiffness
    base::EntityId areaStiffnessTableId = 0; // TAVSTIF: the TABL3Di of the stiffness, 0 when blank
    double friction = 0.0;                   // FRICT: 0.0 means the friction comes from the CONTACT TABLE option

    [[nodiscard]] bool operator==(const BrakePropCard&) const noexcept = default;
};

} // namespace oor::model
