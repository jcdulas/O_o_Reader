#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>

namespace oor::model
{

// One PHBDY card: the property of a CHBDYP surface element.
//
// Card layout: PHBDY  PID  AF  D1  D2
// AF is the area (POINT), the effective width (LINE, ELCYL), the wall thickness (an inner TUBE); D1 and D2 are the
// diameters of ELCYL, TUBE and FTUBE surfaces. All three may be blank: absent values are `std::nullopt`. A blank D2 is
// D1 (format reference), stored resolved and always written once D1 is there.
struct HeatSurfacePropCard
{
    base::EntityId id = 0;
    std::optional<double> area;
    std::optional<double> diameter1;
    std::optional<double> diameter2;
};

} // namespace oor::model
