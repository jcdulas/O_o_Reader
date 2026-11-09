#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::model
{

// The bearing of a squeeze film damper (the SOLN field): the long-bearing or the short-bearing model. The enumerators
// are numbered in the order of BUSH_PROP_2D_SOLUTION_WORDS.
enum class BushProp2DSolution : std::uint8_t
{
    Long, // LONG (the default)
    Short // SHORT
};

inline constexpr std::array<std::string_view, 2> BUSH_PROP_2D_SOLUTION_WORDS{"Long", "Short"};

// The words that open the second line of a PBUSH2D, in field 9.
inline constexpr std::string_view BUSH_PROP_2D_SQUEEZE_WORD = "SqueezeFilm";
inline constexpr std::string_view BUSH_PROP_2D_CROSS_WORD = "Cross";

// The squeeze film damper form of a PBUSH2D: the second and third lines of the card.
//
// Card layout (fields over the lines, eight to a line):
//   SQUEEZE  BDIA   BLEN   BCLR   SOLN    VISCO   PVAPCO             fields 9 to 15
//   NPORT    PRES1  THETA1 PRES2  THETA2  OFFSET1 OFFSET2            fields 17 to 23
// BDIA (inner journal diameter), BLEN (damper length), BCLR (radial clearance) and VISCO (lubricant viscosity) are
// required and greater than zero. PVAPCO (the vapour pressure of the lubricant) is a required real. SOLN is LONG when
// blank. NPORT, the number of lubrication ports, is 1 or 2 and has no default, so it is required. PRES1 (not negative)
// and THETA1 (0.0 <= THETA1 < 360.0) are required for either number of ports; PRES2 (not negative) and THETA2
// (0.0 <= THETA2 < 360.0) are required for two ports and are blank (nothing) for one: a PRES2 or a THETA2 that a
// one-port card gives is kept, since the format does not forbid it. OFFSET1 and OFFSET2 are 0.0 when blank. The angle
// is measured counterclockwise from the displacement x axis in the XY plane, from y in the YZ plane and from z in the
// XZ plane, the offsets are measured from GB relative to GA.
struct BushProp2DSqueeze
{
    double bearingDiameter = 0.0;
    double bearingLength = 0.0;
    double bearingClearance = 0.0;
    BushProp2DSolution bearingModel = BushProp2DSolution::Long;
    double viscosity = 0.0;
    double vaporPressure = 0.0;
    std::int32_t portCount = 1;
    double pressure1 = 0.0;
    double portAngle1 = 0.0;
    std::optional<double> pressure2;
    std::optional<double> portAngle2;
    double offset1 = 0.0;
    double offset2 = 0.0;

    [[nodiscard]] bool operator==(const BushProp2DSqueeze&) const noexcept = default;
};

// The cross coupling form of a PBUSH2D: the second line of the card.
//
// Card layout: CROSS  K12  K21  B12  B21  M12  M21                       fields 9 to 15
// The terms of the stiffness, damping and mass matrices outside the diagonal, [I] = ((I11, I12), (I21, I22)) with
// I = K, B or M. The format gives them no default and no "required" mark: a blank term is 0.0.
struct BushProp2DCross
{
    double stiffness12 = 0.0;
    double stiffness21 = 0.0;
    double damping12 = 0.0;
    double damping21 = 0.0;
    double mass12 = 0.0;
    double mass21 = 0.0;

    [[nodiscard]] bool operator==(const BushProp2DCross&) const noexcept = default;
};

// One PBUSH2D card (SOL 400 and the rotor dynamics): the linear and nonlinear properties of the two-dimensional bush
// CBUSH2D.
//
// Card layout of the first line: PBUSH2D  PID  K11  K22  B11  B22  M11  M22        fields 1 to 7
// PID is a property ID, unique among the properties (the format reference asks for it and for one PBUSH2D per ID: it is
// a primary property). K11 and K22, the nominal stiffnesses in the directions 1 and 2, are required; B11 and B22
// (damping) and M11 and M22 (acceleration-dependent force) are 0.0 when blank.
//
// The second line may be the squeeze film damper data (SQUEEZE) or the cross coupling terms (CROSS), never
// both: the card has at most one of the two options, which `check` enforces. A card with neither is the
// plain nominal form. The squeeze film option is supported by SOL 400 only (the card does not look at the solution).
struct BushProp2DCard
{
    base::EntityId id = 0;
    double stiffness11 = 0.0;
    double stiffness22 = 0.0;
    double damping11 = 0.0;
    double damping22 = 0.0;
    double mass11 = 0.0;
    double mass22 = 0.0;
    std::optional<BushProp2DSqueeze> squeeze;
    std::optional<BushProp2DCross> crossCoupling;
};

} // namespace oor::model
