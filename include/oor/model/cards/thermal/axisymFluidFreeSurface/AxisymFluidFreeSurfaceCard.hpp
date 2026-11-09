#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

// A point of the lists of the axisymmetric fluid (FSLIST, BDYLIST): the ID of a RINGFL, or one of the two words that
// say the list meets the polar axis: AXIS (at its first point) and LAXIS (at its last one).
inline constexpr base::EntityId AXIS_POINT = -1;
inline constexpr base::EntityId LAST_AXIS_POINT = -2;

// One FSLIST card: the fluid points (RINGFL) of a free surface boundary of an axisymmetric fluid model.
//
// Card layout: FSLIST  RHO  IDF1  IDF2  IDF3  IDF4  IDF5  IDF6  IDF7
//                     IDF8 -etc.-
// RHO is the mass density at the surface (blank: DRHO of the AXIF card). The points are RINGFL IDs, in the order of
// travel with the fluid on the right; the first may be the word AXIS (stored as AXIS_POINT) and the last LAXIS
// (LAST_AXIS_POINT). The RINGFL card is not typed, so the IDs are not references.
struct AxisymFluidFreeSurfaceCard
{
    std::optional<double> density;
    std::vector<base::EntityId> points;
};

} // namespace oor::model
