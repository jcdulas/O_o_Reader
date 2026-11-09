#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// The kind of Eulerian material of a PEULER or PEULER1 (the TYPE field): which solver the elements use and whether the
// material has shear strength. The enumerators are numbered in the order of EULER_SOLVER_WORDS.
enum class EulerSolverType : std::uint8_t
{
    Hydro,                // HYDRO (the default): hydrodynamic, no shear strength, plus void
    FirstOrder,           // 1STORDER: single material, first order Riemann solver
    SecondOrder,          // 2NDORDER: single material, second order Riemann solver
    Strength,             // STRENGTH: structural material with shear strength, plus void
    MultiMaterialHydro,   // MMHYDRO: several hydrodynamic materials
    MultiMaterialStrength // MMSTREN: several materials with shear strength
};

inline constexpr std::array<std::string_view, 6> EULER_SOLVER_WORDS{
    "Hydrodynamic", "FirstOrder", "SecondOrder", "Strength", "MultiMaterialHydrodynamic", "MultiMaterialStrength"};

} // namespace oor::model
