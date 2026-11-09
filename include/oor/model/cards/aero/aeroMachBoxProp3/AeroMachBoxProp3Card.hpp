#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace oor::model
{

// A point of the planform that PAERO3 locates (points 5 to 12): its x and y in the aerodynamic coordinate system. A
// blank coordinate has no value (the format reference gives no default), so each is optional.
struct AeroMachBoxProp3Point
{
    std::optional<double> x;
    std::optional<double> y;

    [[nodiscard]] bool operator==(const AeroMachBoxProp3Point&) const noexcept = default;
};

// One PAERO3 card: the property of the CAERO3 Mach box surfaces that name it, with the number of boxes in the flow
// direction and the cranks and control surfaces of the planform.
//
// Card layout: PAERO3  PID  NBOX  NCTRL  (blank)  X5   Y5   X6   Y6
//                     X7   Y7    X8     Y8       X9   Y9   X10  Y10
//                     X11  Y11   X12    Y12
// The eight points 5 to 12 are kept in `points`, the point with the number n at index n - 5 (points 1 to 4 are the
// corners of the CAERO3 card). Points 5 and 6 are the cranks of the leading and the trailing edge, and a crank whose Y
// is not positive does not exist: that is geometry, not an error, so the cranks need no value at all. Points
// 7 and 9 are the ends of the hinge line of the first control surface and points 8 and 10 the points of its inboard and
// outboard edges (usually at the trailing edge); a second control surface takes the outboard edge of the first as its
// inboard edge, so points 9 and 11 are the ends of its hinge line and 10 and 12 the points of its edges. Y7 is not
// below Y8, Y9 not below Y10 and Y11 not below Y12.
//
// NCTRL is 0, 1 or 2 and is required. The continuation lines carry the control surfaces: points 7 to 10 are required
// for NCTRL = 1 and points 7 to 12 for NCTRL = 2, both coordinates of each. a common deck gives all six
// points with NCTRL = 1, so points that NCTRL does not need are read and kept; they have no meaning.
// NBOX is the number of Mach boxes in the flow direction, from 1 to 49. The format is not consistent about that upper
// limit (it also mentions more than 50 boxes); the card keeps the range of the field definition, the stricter one. The
// number of boxes across the span follows from NBOX and the Mach number, not from the card. The card names no other
// card.
struct AeroMachBoxProp3Card
{
    static constexpr std::size_t POINT_COUNT = 8; // the points 5 to 12
    static constexpr std::int32_t MAX_BOX_COUNT = 49;
    static constexpr std::int32_t MAX_CONTROL_COUNT = 2;

    base::EntityId id = 0;
    std::int32_t boxCount = 0;
    std::int32_t controlSurfaceCount = 0;
    std::array<AeroMachBoxProp3Point, POINT_COUNT> points = {}; // index n - 5 is the point n
};

} // namespace oor::model
