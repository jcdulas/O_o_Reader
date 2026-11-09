#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One MOMAX card: a static concentrated moment on a ring of a conical shell model. The card needs an AXIC card in the
// deck, which is a rule between cards, and a LOAD Case Control command that selects its set.
//
// Card layout:
//   MOMAX  SID  RID  HID  S  MR  MP  MZ
//
// SID is a load set number, shared with the other loads (IdSpace::Group). RID is the number of the RINGAX the moment is
// on (a node reference). HID is a harmonic number, zero or more, or a sequence of harmonics written "Sn1Tn2" (
// "S0T10" is the harmonics 0 to 10), which the card keeps as the first harmonic and the last one; a separate card is
// needed for the moment of each harmonic, a sequence being the way to give one moment to many. The moment is
// S times the components (MR, MP, MZ) in the radial, azimuthal and axial directions. S is required (a real the format
// gives no default for); a blank component is 0.0, as for the FORCEAX card the format of which has the same layout.
struct AxisymRingMomentLoadCard
{
    static constexpr std::int32_t SINGLE_HARMONIC = -1; // harmonicEnd of a card that names one harmonic

    base::EntityId id = 0;
    base::EntityId ringId = 0;                  // RID
    std::int32_t harmonicId = 0;                // HID, or the first harmonicId of "Sn1Tn2"
    std::int32_t harmonicEnd = SINGLE_HARMONIC; // the last harmonic of "Sn1Tn2"
    double scaleFactor = 0.0;
    double radialMoment = 0.0;
    double azimuthalMoment = 0.0;
    double axialMoment = 0.0;

    [[nodiscard]] bool isSequence() const noexcept { return harmonicEnd != SINGLE_HARMONIC; }
};

} // namespace oor::model
