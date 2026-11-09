#pragma once

#include "oor/model/control/case_control/CaseControl.hpp"

#include <cstddef>
#include <vector>

namespace oor::analysis
{

// One subcase a combination subcase adds, with its coefficient.
struct CombinationTerm
{
    std::size_t subcaseIndex = 0; // the position of the subcase in the list given to SubcaseCombination::termsOf
    double coefficient = 0.0;
};

// What a combination subcase (SUBCOM, SYMCOM) adds up, as the solver reads its coefficients.
//
// SUBCOM with SUBSEQ: the plain subcases (SUBCASE) before it, in order. Rn applies to the most
// recently appearing subcase, R(n-1) to the one before, and so on: a list shorter than the subcases is aligned on the
// last ones, and the earlier ones take 0 (public v10112: SUBCOM 40 with SUBSEQ = 5320. combines SUBCASE 20, the last
// one, and the SUBCOM 30 between them is not counted). Leading coefficients beyond the first subcase have no subcase
// and are ignored.
// SUBCOM with SUBSEQ1 = S0, S1, SUB1, S2, SUB2, ...: the subcases named, each with S0 times its own factor, in the
// order of the list (format reference SUBSEQ1; a SUBSEQ1 takes precedence over a SUBSEQ, which the format reference
// forbids with it). A named subcase that is not a plain subcase before the SUBCOM is left out. SYMCOM with SYMSEQ: the
// SYM subcases before it, aligned on the last ones as for SUBSEQ (format reference SYMSEQ: the coefficients of the
// previously occurring SYM subcases); every coefficient is 1.0 when there is no SYMSEQ. A SUBCOM with neither list
// gives its subcases the coefficient 0.
class SubcaseCombination
{
public:
    // The terms of `subcases[index]` (the analysis subcases in file order); empty for a section that is not a
    // combination.
    [[nodiscard]] static std::vector<CombinationTerm> termsOf(const std::vector<const model::Subcase*>& subcases,
                                                              std::size_t index);
};

} // namespace oor::analysis
