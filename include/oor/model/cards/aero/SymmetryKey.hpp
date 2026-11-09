#pragma once

#include "oor/model/core/CardProblem.hpp"

#include <cstdint>
#include <optional>

namespace oor::model
{

// The symmetry key of one plane of the aerodynamic coordinate system (SYMXZ and SYMXY of AERO and AEROS): -1, 0 (no
// symmetry, the default) or +1. The two planes do not use the signs alike. Across the x-z plane +1 is symmetry and -1
// antisymmetry; across the x-y plane -1 is symmetry (the way to model a ground effect) and +1 antisymmetry. The card
// keeps the number as written; the analysis reads it with the sign of its plane.
[[nodiscard]] std::optional<CardProblem> checkSymmetryKey(std::int32_t key, const char* field);

} // namespace oor::model
