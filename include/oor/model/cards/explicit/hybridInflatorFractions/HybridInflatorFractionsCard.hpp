#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace oor::model
{

// TYPE (field 2): what the fractions are, mass fractions or molar fractions. A blank TYPE is MASS. Molar fractions need
// the universal gas constant, PARAM,UGASC (a rule of the model, not of the card).
enum class GasFractionBasis : std::uint8_t
{
    Mass,
    Molar
};
inline constexpr std::array<std::string_view, 2> GAS_FRACTION_BASIS_WORDS{"Mass", "MolarFraction"};

// The fractions of the gases at one time: the data that follow one TIME keyword of the card.
struct GasFractionStep
{
    double time = 0.0;             // not negative
    std::vector<double> fractions; // one per gas, in the order of the gases, not negative, one of them above zero

    [[nodiscard]] bool operator==(const GasFractionStep&) const noexcept = default;
};

// One INFLFRC card (SOL 700): the fractions of the gases that a hybrid inflator (INFLHB, which names it in its FRAC
// field) blows in, as a function of time. The solver interpolates linearly between the times.
//
// Card layout: the first line holds the header, the data follow from the first continuation line:
//   INFLFRC  FRACID  TYPE                                                           fields 1 and 2
//   TIME  TIME1  FRAC1  FRAC2  FRAC3  etc.                                          from field 9
//   TIME  TIME2  FRAC1  FRAC2  FRAC3  etc.
// The data of a time begin with the word TIME in the first field of a line and go on over as many lines as needed
// (eight fields to a line); the next time begins at the start of the next line (a common deck shows it: nine
// fractions take two lines). A missing fraction is 0.0, so the times may have a different number of fractions. The
// fractions of the times are in the order of the gases of the inflator and are scaled to add up to 1.0 by the solver;
// at least one of them must be above zero. The card keeps each time with its fractions, in the order read,
// and writes them back as it read them.
struct HybridInflatorFractionsCard
{
    base::EntityId id = 0; // unique number of the gas fractions
    GasFractionBasis type = GasFractionBasis::Mass;
    std::vector<GasFractionStep> steps; // at least one
};

} // namespace oor::model
