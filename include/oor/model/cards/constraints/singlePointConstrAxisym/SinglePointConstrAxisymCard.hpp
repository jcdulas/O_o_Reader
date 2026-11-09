#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One SPCAX card: a single point constraint or an enforced displacement of a conical shell coordinate.
//
// Card layout: SPCAX  SID  RID  HID  C  D
// RID is a RINGAX point and HID a harmonic number (the format reference says HID > 0; harmonic 0 is accepted). C holds
// the digits 1 to 6 of the constrained components as one number; D is the enforced displacement (required). SID is
// selected by the SPC Case Control command.
struct SinglePointConstrAxisymCard
{
    base::EntityId id = 0;
    base::EntityId ringId = 0;
    std::int32_t harmonic = 0; // HID
    std::int32_t components = 0;
    double enforced = 0.0; // D
};

} // namespace oor::model
