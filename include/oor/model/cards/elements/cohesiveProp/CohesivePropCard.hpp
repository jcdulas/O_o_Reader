#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// Where the stress and strain of a cohesive element are given (the OUTPUT field of a PCOHE). The enumerators are
// numbered in the order of COHESIVE_PROP_OUTPUT_WORDS.
enum class CohesivePropOutput : std::uint8_t
{
    Grid, // GRID (the default): at the corner grid points
    Gauss // GAUSS: at the Gauss points
};

inline constexpr std::array<std::string_view, 2> COHESIVE_PROP_OUTPUT_WORDS{"Nodes", "GaussPoints"};

// The values of the INT field: the integration scheme of the element.
inline constexpr std::int32_t COHESIVE_PROP_INTEGRATION_GAUSS = 0;
// Newton-Cotes / Lobatto, for very stiff interfaces.
inline constexpr std::int32_t COHESIVE_PROP_INTEGRATION_LOBATTO = 1;

// One PCOHE card (SOL 400): the property of the interface cohesive zone elements (CIFQUAD, CIFHEX, CIFPENT and
// CIFQDX), which simulate the onset and growth of a delamination.
//
// Card layout: PCOHE  PID  MID  INT  T  OUTPUT  SECANT      fields 1 to 6
// PID is a property ID, in the space of all the properties (the format reference asks for an ID that no other property
// has, and one PCOHE per ID). MID names an MCOHE, the cohesive law, which is a material. INT is 0 (Gauss integration,
// when blank) or 1 (Newton-Cotes / Lobatto). T is the thickness, greater than zero and 1.0 when blank; it applies to
// the CIFQUAD only. OUTPUT is GRID (when blank) or GAUSS; the format reference says that INT = 1 sets OUTPUT
// to GRID, which the solver does: the card keeps the word it was given, as the other cards keep a value the solver
// overrides. SECANT is 0 (a secant stiffness matrix, when blank) or 1 (a tangent one).
struct CohesivePropCard
{
    base::EntityId id = 0;
    base::EntityId matId = 0;                                   // an MCOHE
    std::int32_t integration = COHESIVE_PROP_INTEGRATION_GAUSS; // INT: 0 or 1
    double thickness = 1.0;                                     // T, greater than zero
    CohesivePropOutput output = CohesivePropOutput::Grid;       // OUTPUT
    std::int32_t tangentStiffness = 0;                          // SECANT: 0 secant matrix, 1 tangent matrix
};

} // namespace oor::model
