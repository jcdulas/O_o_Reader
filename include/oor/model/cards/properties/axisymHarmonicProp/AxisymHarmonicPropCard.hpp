#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One PAXSYMH card: the properties of the linear axisymmetric harmonic elements CQUADX and CTRIAX.
//
// Card layout:  PAXSYMH  PID  MID  CID  NHARM  INT        fields 1 to 5
// PID is a property ID (the elements name it). MID is a MAT1 or a MAT9 and is required. CID is the element
// coordinate system, 0 (the basic system) when blank: the Y axis of the system is the axis of symmetry and its X axis
// the radial direction. NHARM is the harmonic index, an integer of 0 or more that is 1 when blank (the
// gyroscopic matrix exists for the element only with NHARM = 1). The format is not consistent about CID and
// NHARM (positive in places, 0 or more in others); the card takes 0 or more. INT is the integration scheme, 0, 1,
// 2 or 3: 0 means 2 (no mid-side nodes) or 3 (mid-side nodes), and 1 gives a singular stiffness and is meant
// for experiments only, which the card accepts.
//
// The property is a primary one: its ID is in the property space, so the PID of a CQUADX or a CTRIAX finds it.
struct AxisymHarmonicPropCard
{
    static constexpr std::int32_t DEFAULT_HARMONIC_INDEX = 1;
    static constexpr std::int32_t MAX_INTEGRATION = 3;

    base::EntityId id = 0;
    base::EntityId matId = 0;
    base::EntityId frameId = 0; // 0: the basic system
    std::int32_t harmonicIndex = DEFAULT_HARMONIC_INDEX;
    std::int32_t integration = 0; // the field INT
};

} // namespace oor::model
