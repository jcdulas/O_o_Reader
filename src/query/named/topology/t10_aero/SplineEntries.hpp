#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/Model.hpp"

#include <optional>
#include <string_view>
#include <utility>
#include <vector>

// The splines of a model as one list, whichever of the eight cards each comes from: what `splinesTable` and
// `splineNodes` both read. Each card is read by its own function in the .cpp; nothing here describes the fields of
// several cards in a table.

namespace oor::query::aero
{

// One spline card. A field the card has no counterpart for stays at zero or empty.
struct SplineEntry
{
    base::EntityId eid = 0;
    std::string_view type;     // the card type: "AeroSurfaceSpline1" ... "AeroUserSpline"
    base::EntityId caero = 0;  // 0: the card names none
    base::EntityId aelist = 0; // 0: the card gives a range of boxes instead (or none: SPLINEX)
    base::EntityId setg = 0;   // 0: the card has no set (SPLINE3, SPLINEX)
    std::optional<std::pair<base::EntityId, base::EntityId>> boxRange; // BOX1 and BOX2, ID1 and ID2, or BOXID twice
    std::optional<double> dz;
    std::string_view method;            // empty: the card has no method
    std::string_view usage;             // FORCE, DISP or BOTH
    std::vector<base::EntityId> points; // SPLINE3: the points G of its independent degrees of freedom
    bool hasPointList = false;          // SPLINE3 gives its structural points itself, with no set
};

// The splines of the model, by EID (cards that share an EID stay in the order of the deck).
[[nodiscard]] std::vector<SplineEntry> readSplines(const model::Model& model);

} // namespace oor::query::aero
