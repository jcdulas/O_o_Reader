#pragma once

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <optional>

// The Mach number of a parametric force, a value that AEDW, AEPRESS and AEFORCE share. The helper moves this one kind
// of value: each of the three cards still lays out, reads, writes and checks its own fields.
namespace oor::model
{

// The problem with the MACH of a parametric force, if there is one. The format reference gives a real of zero or more
// other than 1.0 for the three cards: zero is the incompressible limit, and 1.0 is neither subsonic nor supersonic. A
// value above 1.0 is allowed (it needs the supersonic aerodynamic option of the program, which the card cannot know).
[[nodiscard]] inline std::optional<CardProblem> checkForceMach(double mach)
{
    if (const auto problem = checkReal(mach, "machNumber", RealRule::NonNegative))
        return problem;
    if (mach == 1.0)
        return CardProblem{"card.bad-value", "{field} cannot be 1.0", "machNumber"};
    return std::nullopt;
}

} // namespace oor::model
