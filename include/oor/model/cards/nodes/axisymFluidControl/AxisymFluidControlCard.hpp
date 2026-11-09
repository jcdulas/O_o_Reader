#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

// The NOSYM field of an AXIF: whether the nonsymmetric (sine) terms of the series are asked for. YES gives the sine and
// the cosine terms, NO only the cosine terms. The enumerators follow AXISYM_FLUID_SYMMETRY_WORDS.
enum class AxisymFluidControlSymmetry : std::uint8_t
{
    Yes,
    No
};

inline constexpr std::array<std::string_view, 2> AXISYM_FLUID_SYMMETRY_WORDS{"Yes", "No"};

// One item of the list of harmonics of an AXIF, kept as it was written: a single number "N", a range "N1 THRU Ni" or a
// range with a step "N1 THRU Ni STEP NS" (every NS-th harmonic of the range, Ni = i * NS + N1).
struct AxisymFluidControlHarmonics
{
    static constexpr std::int32_t NO_STEP = 0; // a range written without STEP
    static constexpr std::int32_t MAX_HARMONIC = 99;

    std::int32_t first = 0;
    std::int32_t last = 0;       // equal to `first` for a single number
    bool range = false;          // written with THRU
    std::int32_t step = NO_STEP; // the NS of a range written with STEP

    [[nodiscard]] static constexpr AxisymFluidControlHarmonics single(std::int32_t harmonic) noexcept
    {
        return AxisymFluidControlHarmonics{harmonic, harmonic, false, NO_STEP};
    }
    [[nodiscard]] static constexpr AxisymFluidControlHarmonics through(std::int32_t first, std::int32_t last,
                                                                       std::int32_t step = NO_STEP) noexcept
    {
        return AxisymFluidControlHarmonics{first, last, true, step};
    }

    [[nodiscard]] bool operator==(const AxisymFluidControlHarmonics&) const noexcept = default;
};

// One AXIF card: the basic parameters and the existence of an axisymmetric fluid analysis, with the harmonics of the
// solution. Only one AXIF card is allowed in a deck (the table reports a second one when the deck is
// read). Superelements cannot be used with it: a rule between cards.
//
// Card layout:
//   AXIF  CID  G  DRHO  DB  NOSYM  F
//         N1  N2  N3  N4  N5  ...
// and the alternate forms of the continuation lines (a range N1 THRU Ni, a range with a step N1 THRU Ni STEP NS).
//
// CID is the fluid coordinate system, which must be cylindrical or spherical (a rule between cards). G is the
// gravity along the axis (positive gravity is a free fall in the -Z direction of the fluid system). DRHO is
// the default density of the fluid elements (above zero, or blank: it replaces a blank RHO of the FSLIST, BDYLIST and
// CFLUIDi cards) and DB the default bulk modulus (blank or zero with a blank CFLUIDi means an incompressible
// fluid); the format gives no default for either, so a blank one is kept blank. NOSYM is YES or NO and is
// required (the format lists no blank).
//
// F is blank when the harmonics are listed and NONE when there are none (no harmonics, no fluid elements, and no
// continuation line may be present), so the field is the list: the card has F = NONE exactly when its list of
// harmonics is empty, and the reader refuses a NONE with a list and a blank F with none (the format gives that card no
// meaning). The harmonics are numbers from 0 to 99 in increasing order, singly or as ranges (the format: 0 <= Ni <
// 100). A blank field between the numbers of a continuation line is ignored. The card keeps the list as written and
// writes it back in the same items, a range never split between two lines.
struct AxisymFluidControlCard
{
    base::EntityId frameId = 0;
    double gravity = 0.0;
    std::optional<double> defaultDensity; // blank: the RHO of each card stays as it is
    std::optional<double> defaultBulkModulus;
    AxisymFluidControlSymmetry nonsymmetric = AxisymFluidControlSymmetry::Yes;
    std::vector<AxisymFluidControlHarmonics> harmonics; // empty: F = NONE

    [[nodiscard]] bool operator==(const AxisymFluidControlCard&) const noexcept = default;
};

} // namespace oor::model
