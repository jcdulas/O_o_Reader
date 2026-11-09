#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// HTYPE: the component of a harmonic a LOADCYH load is on (cosine C, sine S, and the starred forms of a dihedral
// model), or the gravity and rotational loads that generate their own harmonics. Blank applies the load to every type.
enum class HarmonicType : std::uint8_t
{
    All, // blank
    C,
    S,
    CStar,
    SStar,
    GravityLoad,
    RotationalForceLoad
};

// One LOADCYH card: the harmonic coefficients of a static or dynamic load of a cyclic symmetry model.
//
// Card layout: LOADCYH  SID  S  HID  HTYPE  S1  L1  S2  L2
// The load of component HTYPE of harmonic HID is S times (S1 times the load set L1 plus S2 times the load set L2). With
// HTYPE GRAV or RFORCE the HID is ignored and S2 and L2 must be blank. The second pair is optional (l2
// is 0 and s2 0.0 when it is not there). S must not be zero, nor S1 and S2 both.
struct CyclicHarmonicLoadCard
{
    static constexpr std::int32_t NO_HARMONIC = -1; // a blank HID (the harmonic 0 is a harmonic)

    base::EntityId id = 0;
    double scaleFactor = 0.0;
    std::int32_t harmonicId = NO_HARMONIC;
    HarmonicType harmonicType = HarmonicType::All;
    double scaleFactor1 = 0.0;
    base::EntityId loadSetId1 = 0;
    double scaleFactor2 = 0.0;
    base::EntityId loadSetId2 = 0;

    [[nodiscard]] bool hasSecond() const noexcept { return loadSetId2 != 0; }
};

} // namespace oor::model
