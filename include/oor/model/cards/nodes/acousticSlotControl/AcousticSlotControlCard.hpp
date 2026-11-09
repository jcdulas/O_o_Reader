#pragma once

#include <cstdint>
#include <optional>

namespace oor::model
{

// One AXSLOT card: the harmonic index and the default values of the acoustic cavity (slot) analysis, which the GRIDS,
// SLBDY, CAXIFi and CSLOTi cards leave blank (the format asks for a value here, in the RHOD, BD and MD fields, when one
// of them does). Only one AXSLOT card is allowed in a deck; the table reports a second one when the deck
// is read.
//
// Card layout:
//   AXSLOT  RHOD  BD  N  WD  MD
//
// RHOD is the default fluid density (above zero), BD the default bulk modulus (zero or more: 0.0 is an incompressible
// fluid), N the harmonic index (zero or more), WD the default slot width and MD the default number of slots
// (zero or more: no matrices are made for zero slots). Every field may be blank and the format gives no
// default, so a blank one is kept blank (an empty optional, a NaN or -1 in the column). The format prints the sign of
// WD badly (a greater-than with its bar apart): it is read as zero or more, like its other width, the W of the GRIDS
// card, has.
struct AcousticSlotControlCard
{
    std::optional<double> defaultDensity;
    std::optional<double> defaultBulkModulus;
    std::optional<std::int32_t> harmonic;
    std::optional<double> defaultSlotWidth;
    std::optional<std::int32_t> defaultSlotCount;
};

} // namespace oor::model
