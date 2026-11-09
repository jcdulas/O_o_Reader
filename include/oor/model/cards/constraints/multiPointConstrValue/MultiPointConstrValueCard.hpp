#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// One MPCD card: the load selectable right hand side Ym of one or two equations of non-homogeneous multipoint
// constraints (the MPCY cards). The value of an equation replaces the YM of its MPCY.
//
// Card layout: MPCD  SID  GM1  CM1  YM1  GM2  CM2  YM2
// An equation is named by its dependent degree of freedom: the point GMi and the component CMi (one of 1 to 6 for a
// grid point; blank, 0 or 1 for a scalar point; the card keeps 0 for a blank). The second equation is optional: its GM2
// is 0 when it is absent (then CM2 and YM2 are 0 too). The format reference gives no default to YMi; a blank value is
// kept as 0.0, as the SPCD it resembles does. SID is selected by the LOAD Case Control command in the static solutions,
// so it is a load set ID like the SID of an SPCD and the card is in the Load category; the MPC command
// selects the MPCY.
struct MultiPointConstrValueCard
{
    base::EntityId id = 0;
    std::array<base::EntityId, 2> points{0, 0};
    std::array<std::int32_t, 2> components{0, 0};
    std::array<double, 2> values{0.0, 0.0};

    [[nodiscard]] bool hasSecondEquation() const noexcept { return points[1] != 0; }
};

} // namespace oor::model
