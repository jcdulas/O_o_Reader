#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

// The most factors a THRU form may generate. The format reference sets no limit; a table of a million values is far
// beyond any flutter analysis, and the bound keeps a damaged NF from asking `aeroFlutterFactorsValues` for gigabytes.
inline constexpr std::int32_t MAX_FLUTTER_FACTOR_COUNT = 1'000'000;

// One FLFACT card: a list of the density ratios, Mach numbers, reduced frequencies or velocities of a flutter analysis,
// that a FLUTTER card names by its SID. What the numbers mean is up to the field of the FLUTTER that names the list.
//
// Card layout, first form:    FLFACT  SID  F1  F2  F3  F4  F5  F6  F7
//                                    F8   F9  -etc.-
//             second form:   FLFACT  SID  F1  "THRU"  FNF  NF  FMID
// The first form lists the factors, in the order the analysis runs them; no field is blank between two of them.
// The second form gives NF factors from F1 to FNF, spaced so that the one in the middle is FMID:
// with FMID halfway the spacing is even, with FMID = 2 F1 FNF / (F1 + FNF) the reciprocals are evenly spaced. FMID is
// blank to take the middle. The card keeps the form it was read in (a card read as THRU is written as THRU) and
// `aeroFlutterFactorsValues` gives the factors of either.
//
// A card has the factors, or the four members of the THRU form, never both: `thru` says which, and the members of the
// other form stay at their zero (empty for FMID). FMID is not resolved when blank. The format reference replaces an
// FMID that is not between F1 and FNF by their middle, so the blank is the same rule and stays right when F1 or FNF are
// edited.
struct AeroFlutterFactorsCard
{
    base::EntityId id = 0;
    std::vector<double> factors; // the first form: F1, F2, ... (empty in the THRU form)
    bool isRange = false;        // the second form
    double factor1 = 0.0;
    double factorLast = 0.0;
    std::int32_t factorCount = 0;    // at least 1
    std::optional<double> factorMid; // blank when empty
};

} // namespace oor::model
