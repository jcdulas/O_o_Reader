#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// The pattern of the holes of a perforated shell (the TOPOLOGY field): on a square grid, a triangular grid or a
// hexagonal one. The enumerators are numbered in the order of PERFORATED_SHELL_PROP_TOPOLOGY_WORDS.
enum class PerforatedShellPropTopology : std::uint8_t
{
    Square, // SQUARE
    Tria,   // TRIA
    Hexa    // HEXA
};

inline constexpr std::array<std::string_view, 3> PERFORATED_SHELL_PROP_TOPOLOGY_WORDS{"Square", "Triangular",
                                                                                      "Hexagonal"};

// One PSHLPF card: the property of a perforated shell of a trim component (TRMC) made of a porous elastic material.
// The shell elements of the trim component name it by PID. The card is for the Bulk Data of a trim component
// (`BEGIN TRMC=trmid`) only; the library does not look at where it stands.
//
// Card layout: PSHLPF  PID  MID  T  SPACEG  RADIUS  TOPOLGY  FRHO  FVIS      fields 1 to 8
//                     HOMG                                                  field 9 (the second line)
// The format spells the pattern TOPOLGY in the layout and TOPOLOGY in the description of the fields; the card calls it
// TOPOLOGY. MID is a MAT1. T (the thickness), SPACEG (the distance between the holes), RADIUS (the radius of a hole),
// FRHO (the density of the fluid) and FVIS (its viscosity) are real numbers greater than zero. The format gives none of
// these fields nor TOPOLOGY a default, so all of them are required. HOMG is the homogenisation flag, 0 or 1; it is 1
// when blank, and then the stiffness and the density vary with the porosity, while 0 uses the stiffness as
// it is given.
struct PerforatedShellPropCard
{
    base::EntityId id = 0; // unique among the properties
    base::EntityId matId = 0;
    double thickness = 0.0;
    double holeSpacing = 0.0;
    double holeRadius = 0.0;
    PerforatedShellPropTopology topology = PerforatedShellPropTopology::Square;
    double fluidDensity = 0.0;
    double fluidViscosity = 0.0;
    std::int32_t homogenization = 1;

    [[nodiscard]] bool operator==(const PerforatedShellPropCard&) const noexcept = default;
};

} // namespace oor::model
