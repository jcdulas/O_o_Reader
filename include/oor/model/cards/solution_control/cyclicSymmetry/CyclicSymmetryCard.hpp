#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// The STYPE words of the format reference: the symmetry type. ROT is rotational symmetry, DIH dihedral symmetry and AXI
// the special case of ROT that models axisymmetric structures.
enum class CyclicSymmetryType : std::uint8_t
{
    Rotational,
    Dihedral,
    Axisymmetric
};

inline constexpr std::array<std::string_view, 3> CYCLIC_SYMMETRY_TYPE_WORDS{"Rotational", "Dihedral", "Axisymmetric"};

// One CYSYM card: the parameters of a cyclic symmetry analysis.
//
// Card layout: CYSYM  NSEG  STYPE
// In this library's numbering NSEG is field 1 and STYPE field 2. NSEG is an integer greater than 0, and STYPE has no
// default on the format, so it is required. With STYPE = AXI every grid point must lie on side 1, side 2 or the axis
// and the plate elements may not have midside grid points: rules between this card and the model, which the card does
// not check. The card names no other card.
struct CyclicSymmetryCard
{
    std::int32_t segments = 0; // NSEG
    CyclicSymmetryType type = CyclicSymmetryType::Rotational;

    [[nodiscard]] bool operator==(const CyclicSymmetryCard&) const noexcept = default;
};

} // namespace oor::model
