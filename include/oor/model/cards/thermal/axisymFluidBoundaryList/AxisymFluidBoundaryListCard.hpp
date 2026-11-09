#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/thermal/axisymFluidFreeSurface/AxisymFluidFreeSurfaceCard.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

// One BDYLIST card: the fluid points (RINGFL) of the boundary between an axisymmetric fluid and a structure.
//
// Card layout: BDYLIST  RHO  IDF1  IDF2  IDF3  IDF4  IDF5  IDF6  IDF7
//                      IDF8 -etc.-
// RHO is the fluid mass density at the boundary (blank: DRHO of the AXIF card; 0.0 generates no boundary matrix terms).
// The points are RINGFL IDs, in the order of travel with the fluid on the right; the first may be the word AXIS
// (AXIS_POINT, of AxisymFluidFreeSurfaceCard.hpp) and the last LAXIS (LAST_AXIS_POINT). The RINGFL card is not typed,
// so the IDs are not references.
struct AxisymFluidBoundaryListCard
{
    std::optional<double> density;
    std::vector<base::EntityId> points;
};

} // namespace oor::model
