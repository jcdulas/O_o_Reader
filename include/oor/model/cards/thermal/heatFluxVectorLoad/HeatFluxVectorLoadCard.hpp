#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/thermal/ThermalRuns.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

// One QVECT card: the thermal vector flux from a distant source into the faces of surface elements.
//
// Card layout: QVECT  SID  Q0  TSOUR  CE  E1 or TID1  E2 or TID2  E3 or TID3  CNTRLND
//                    EID1  EID2  -etc.-
// Q0 (the magnitude) and TSOUR (the source temperature) may be blank, CE is the coordinate system of the direction (-1,
// 0 or a system ID; absent when blank). Each component of the direction is a real (0.0 when blank) or, when the field
// holds an integer, the TABLEDi that gives it as a function of time: `e` has the reals and `tid` the tables, 0 where a
// component is a real. CNTRLND is the control point (0 when blank). The elements are surface elements: single IDs and
// "a THRU b" ranges.
struct HeatFluxVectorLoadCard
{
    base::EntityId id = 0;
    std::optional<double> heatFlux;
    std::optional<double> sourceTemperature;
    std::optional<int> directionFrameId;
    std::array<double, 3> direction = {};
    std::array<base::EntityId, 3> tableId = {};
    base::EntityId controlNode = 0;
    std::vector<ThermalRun> elements;
};

} // namespace oor::model
