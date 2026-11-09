#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::model
{

// The enumerators of this file are numbered in the order of the array of words that follows each, so the position of a
// word is the value of its enumerator (`keywordText` and `parseKeyword` of explicit/ExplicitSupport.hpp).

// The formulation of the shell (FORM).
enum class ShellPropNodalForm : std::uint8_t
{
    Hughes,   // HUGHES: Hughes-Liu
    Blt,      // BLT: Belytschko-Lin-Tsay
    Keyhoff,  // KEYHOFF: Key-Hoff
    C0Tria,   // C0-TRIA: the C0 triangle
    Membrane, // MEMB: a membrane, with no bending
    Dummy     // DUMMY: a dummy element, for fluid-structure coupling only
};

inline constexpr std::array<std::string_view, 6> SHELL_PROP_NODAL_FORM_WORDS{
    "HughesLiu", "BelytschkoLinTsay", "KeyHoff", "C0Triangle", "Membrane", "Dummy"};

// The quadrature through the thickness (QUAD).
enum class ShellPropNodalQuadrature : std::uint8_t
{
    Gauss,  // GAUSS (the default)
    Lobatto // LOBATTO
};

inline constexpr std::array<std::string_view, 2> SHELL_PROP_NODAL_QUADRATURE_WORDS{"Gauss", "LobattoPoints"};

// The reference surface of the shell (REF).
enum class ShellPropNodalReference : std::uint8_t
{
    Top,    // TOP
    Middle, // MID (the default)
    Bottom  // BOT
};

inline constexpr std::array<std::string_view, 3> SHELL_PROP_NODAL_REFERENCE_WORDS{"Top", "MidSurface", "Bottom"};

// Whether the spin correction is applied (SPINCOR).
enum class ShellPropNodalSpinCorrection : std::uint8_t
{
    No, // NO (the default)
    Yes // YES
};

inline constexpr std::array<std::string_view, 2> SHELL_PROP_NODAL_SPIN_CORRECTION_WORDS{"No", "Yes"};

// The method of the transverse shear (TRANSHR).
enum class ShellPropNodalTransverseShear : std::uint8_t
{
    Linear,     // LINEAR
    Constant,   // CONSTANT
    Approximate // CONAPX: approximated constant shear
};

inline constexpr std::array<std::string_view, 3> SHELL_PROP_NODAL_TRANSVERSE_SHEAR_WORDS{"Linear", "Constant",
                                                                                         "ConstantApproximation"};

// The shear-lock avoidance (SHRLCK).
enum class ShellPropNodalShearLock : std::uint8_t
{
    Avoid,  // AVOID
    NoAvoid // NOAVOID
};

inline constexpr std::array<std::string_view, 2> SHELL_PROP_NODAL_SHEAR_LOCK_WORDS{"AvoidShearLocking",
                                                                                   "NoAvoidShearLocking"};

// One PSHELL1 card (SOL 700 only): the property of a shell with a thickness at each grid point, a formulation and the
// options of the explicit shell elements.
//
// Card layout (the fields are numbered across the lines, eight to a line):
//   PSHELL1  PID  MID  FORM  QUAD  NUMB  SHFACT  REF  SPINCOR                      fields 1 to 8
//            T1   T2   T3    T4    TRANSHR  SHRLCK                                  9 to 14
// PID and MID are required. FORM is blank when the deck leaves it out: the format reference says the formulation then
// depends on the element that names the property (KEYHOFF for a CQUAD4, C0-TRIA for a CTRIA3), which the card cannot
// know, so the card keeps the blank (`std::nullopt`) and a later step resolves it for the element. The format spells
// the triangle C0-TRIA in the table of words and CO-TRIA elsewhere (a letter O); both are read and C0-TRIA is written.
// NUMB is the number of integration points through the thickness, 1 to 5; its default depends on FORM (1 for a
// membrane, 3 for the others) and is stored resolved. SHFACT is 0.83333 when blank. T1 to T4 are the thicknesses at the
// grid points, 0.0 when blank: with T1 blank or zero the thickness comes from the element. TRANSHR and
// SHRLCK are blank (no value kept) when the deck leaves them out: their defaults depend on FORM and on the element
// (BLT has NOAVOID, KEYHOFF has LINEAR and AVOID, HUGHES neither), and the format marks the other combinations
// as unavailable, which the card does not refuse.
struct ShellPropNodalCard
{
    static constexpr std::int32_t DEFAULT_INTEGRATION_POINT_COUNT = 3;
    static constexpr std::int32_t MEMBRANE_INTEGRATION_POINT_COUNT = 1;
    static constexpr std::int32_t MAX_INTEGRATION_POINT_COUNT = 5;
    static constexpr double DEFAULT_SHEAR_FACTOR = 0.83333;

    base::EntityId id = 0;    // unique among the properties
    base::EntityId matId = 0; // the material
    std::optional<ShellPropNodalForm> formulation;
    ShellPropNodalQuadrature quadrature = ShellPropNodalQuadrature::Gauss;
    std::int32_t integrationPointCount = DEFAULT_INTEGRATION_POINT_COUNT;
    double shearFactor = DEFAULT_SHEAR_FACTOR;
    ShellPropNodalReference referenceSurface = ShellPropNodalReference::Middle;
    ShellPropNodalSpinCorrection spinCorrection = ShellPropNodalSpinCorrection::No;
    double thickness1 = 0.0;
    double thickness2 = 0.0;
    double thickness3 = 0.0;
    double thickness4 = 0.0;
    std::optional<ShellPropNodalTransverseShear> transverseShear;
    std::optional<ShellPropNodalShearLock> shearLocking;
};

// The NUMB that a blank field means for a formulation (3, and 1 for a membrane).
[[nodiscard]] constexpr std::int32_t shellPropNodalDefaultPointCount(
    const std::optional<ShellPropNodalForm>& form) noexcept
{
    return form && *form == ShellPropNodalForm::Membrane ? ShellPropNodalCard::MEMBRANE_INTEGRATION_POINT_COUNT
                                                         : ShellPropNodalCard::DEFAULT_INTEGRATION_POINT_COUNT;
}

} // namespace oor::model
