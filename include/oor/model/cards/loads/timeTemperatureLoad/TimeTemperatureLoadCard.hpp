#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One TTEMP card (SOL 400, nonlinear transient analysis): a time-dependent temperature distribution of the form
// T(t) = A(T(x)) * F(t), where the spatial distribution A comes from the TEMP, TEMPD, TEMPP1 or TEMPRB cards of the
// same SID and the time function F is a TABLED entry. The grid points that share one time function make up a group
// (GROUP_ID), which a TMPSET lists.
//
// Card layout: TTEMP  SID  GROUP_ID  TID                      fields 1 to 3
//
// SID is a temperature set ID shared with the TEMP cards and the other load cards (IdSpace::Group, category Load): the
// Case Control TEMPERATURE(LOAD) selects it, as it does a TEMP set, so a TTEMP is found by every selection that finds a
// TEMP (and TEMPERATURE(INITIAL) uses its spatial distribution only). GROUP_ID names a TMPSET
// (ReferenceTarget::TemperatureGroup) or is -1, which stands for every grid point in one group and is not a reference.
// TID is a TABLEDi in the dynamic table space (ReferenceTarget::DynamicTable). All three fields are
// required.
struct TimeTemperatureLoadCard
{
    // The value of GROUP_ID that means every grid point of the model.
    static constexpr base::EntityId ALL_GRID_POINTS = -1;

    base::EntityId id = 0;
    base::EntityId groupId = 0; // a TMPSET ID, or ALL_GRID_POINTS
    base::EntityId tableId = 0;

    [[nodiscard]] bool operator==(const TimeTemperatureLoadCard&) const noexcept = default;
};

} // namespace oor::model
