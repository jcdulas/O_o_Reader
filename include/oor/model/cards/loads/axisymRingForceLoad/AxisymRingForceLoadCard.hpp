#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One FORCEAX card: a static concentrated force on a ring of a conical shell model (it needs an AXIC card).
//
// Card layout: FORCEAX  SID  RID  HID  S  FR  FP  FZ
// HID is a harmonic number, or a sequence of harmonics written "Sn1Tn2" ("S0T10" is the harmonics 0 to
// 10). The force is S times the components (FR, FP, FZ) in the radial, azimuthal and axial directions. RID names a
// RINGAX card, which is not modelled yet.
struct AxisymRingForceLoadCard
{
    static constexpr std::int32_t SINGLE_HARMONIC = -1; // harmonicEnd of a card that names one harmonic

    base::EntityId id = 0;
    base::EntityId ringId = 0;                  // RID
    std::int32_t harmonicId = 0;                // HID, or the first harmonicId of "Sn1Tn2"
    std::int32_t harmonicEnd = SINGLE_HARMONIC; // the last harmonic of "Sn1Tn2"
    double scaleFactor = 0.0;
    double radialForce = 0.0;
    double azimuthalForce = 0.0;
    double axialForce = 0.0;

    [[nodiscard]] bool isSequence() const noexcept { return harmonicEnd != SINGLE_HARMONIC; }
};

} // namespace oor::model
