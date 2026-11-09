#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::model
{

// The VARTYP words: the variabilities that follow are alpha variabilities (the default) or delta variabilities. They
// are related through the order n of the matrix that is randomized, alpha = delta * sqrt(2 / (n + 1)).
enum class MonteCarloParamsVariabilityType : std::uint8_t
{
    Alpha,
    Delta
};

inline constexpr std::array<std::string_view, 2> MONTE_CARLO_TYPE_WORDS{"AlphaVariation", "DeltaVariation"};

// The three variabilities of the STRVAR line (structural mass, stiffness and damping) or of the FLUVAR line (fluid
// mass, stiffness and damping). Each is a real that is zero or more and is 0.0 when blank.
struct MonteCarloParamsVariabilities
{
    double mass = 0.0;      // MSVAR or MFVAR
    double stiffness = 0.0; // KSVAR or KFVAR
    double damping = 0.0;   // BSVAR or BFVAR

    [[nodiscard]] bool operator==(const MonteCarloParamsVariabilities&) const noexcept = default;
};

// One MONCARL card: the parameters of a Monte-Carlo simulation by the non-parametric variability method. The Case
// Control MONCARL selects it by SID.
//
// Card layout: MONCARL  SID  VARTYP
//                      STRVAR  MSVAR  KSVAR  BSVAR
//                      FLUVAR  MFVAR  KFVAR  BFVAR
// In this library's numbering (field 1 is SID): VARTYP is field 2 and each line after the head has eight fields from
// field 9 on; a line starts with its keyword, STRVAR or FLUVAR, followed by the three variabilities. The layout has the
// STRVAR line first and the FLUVAR line second; a card with one of them or neither is read
// (every variability defaults to 0.0), the lines in either order, and a keyword that repeats is refused. A line that is
// present is kept even when its three values are zero: the keyword is the line. VARTYP is blank for ALPHAVAR, which is
// kept resolved and written as a blank. The format limits the values by the order of the matrix, which
// the card does not know: only a negative value is refused here.
struct MonteCarloParamsCard
{
    base::EntityId id = 0; // SID
    MonteCarloParamsVariabilityType type = MonteCarloParamsVariabilityType::Alpha;
    std::optional<MonteCarloParamsVariabilities> structural; // the STRVAR line
    std::optional<MonteCarloParamsVariabilities> fluid;      // the FLUVAR line

    [[nodiscard]] bool operator==(const MonteCarloParamsCard&) const noexcept = default;
};

} // namespace oor::model
