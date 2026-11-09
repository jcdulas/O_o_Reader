#pragma once

#include "oor/analysis/validation/engine/ValidationSupport.hpp"

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

namespace oor::analysis
{

// The analysis subcases of the Case Control that select an analysis: SUBCASE sections, or the global section alone when
// the deck has no SUBCASE (the combinations SUBCOM and SYMCOM, SYM and REPCASE select nothing of their own).
[[nodiscard]] std::vector<const model::Subcase*> solvingSubcases(const model::CaseControl& caseControl);

// Reports, at the SOL statement, that the solution is one of `solutions` and that no analysis subcase selects any of
// the Case Control commands `commands` (their neutral names, own or inherited): the eigenvalue method for a normal
// modes solution, the time steps for a transient one. `what` says what the command selects, for the message.
void reportMissingSelection(const ValidationContext& context, FindingSink& sink,
                            std::initializer_list<std::int64_t> solutions,
                            std::initializer_list<std::string_view> commands, const std::string& what);

} // namespace oor::analysis
