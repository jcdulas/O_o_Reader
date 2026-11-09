#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One LOADCYT card: loads of an axisymmetric cyclic model as a function of the azimuth angle, from a table of scale
// factors and a load set (two pairs at most).
//
// Card layout: LOADCYT  SID  TABLEID1  LOADSET1  METHOD1  TABLEID2  LOADSET2  METHOD2
// METHOD (0 or 1) selects how the load is interpolated around the model (0 when blank). The second group is optional:
// it is there when TABLEID2 and LOADSET2 are (both are 0 when it is not).
struct CyclicAzimuthTableLoadCard
{
    base::EntityId id = 0;
    base::EntityId tableId1 = 0;
    base::EntityId loadSetId1 = 0;
    std::int32_t methodName1 = 0;
    base::EntityId tableId2 = 0;
    base::EntityId loadSetId2 = 0;
    std::int32_t methodName2 = 0;

    [[nodiscard]] bool hasSecond() const noexcept { return tableId2 != 0; }
};

} // namespace oor::model
