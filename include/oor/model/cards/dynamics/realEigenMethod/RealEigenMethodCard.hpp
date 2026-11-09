#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <optional>

namespace oor::model
{

// METHOD of an EIGR card: the way the eigenvalues are extracted. LAN and AHOU are the modern ones, the others are
// obsolete but still read.
enum class RealEigenMethodMethod : std::uint8_t
{
    Lanczos,              // LAN (the default)
    AutoHouseholder,      // AHOU: HOU or MHOU, chosen by the program
    InversePower,         // INV
    InversePowerEnhanced, // SINV
    Givens,               // GIV
    ModifiedGivens,       // MGIV
    Householder,          // HOU
    ModifiedHouseholder,  // MHOU
    AutoGivens            // AGIV: GIV or MGIV, chosen by the program
};

// NORM of an EIGR card: how the eigenvectors are scaled.
enum class RealEigenMethodNorm : std::uint8_t
{
    Mass, // MASS: unit generalised mass (the default)
    Max,  // MAX: unit value of the largest component of the analysis set
    Point // POINT: unit value of the component G, C
};

// One EIGR card: real eigenvalue extraction, with the method named on the card.
//
// Card layout:
//   EIGR  SID  METHOD  F1  F2  NE  ND
//         NORM G       C
// Fields 7 and 8 of the first line are not used. The second line is optional: without it the vectors are mass
// normalised.
//
// A blank ND stays blank: what it stands for depends on METHOD (for SINV every root in the range, at most 600; for
// INV three times NE; for the Householder and Givens methods all the roots), and a number given asks for that many
// roots, so no number stands for it.
struct RealEigenMethodCard
{
    base::EntityId id = 0;
    RealEigenMethodMethod method = RealEigenMethodMethod::Lanczos;
    std::optional<double> frequency1; // the frequency range, in cycles per unit time
    std::optional<double> frequency2;
    std::int32_t estimatedModeCount = 0;          // the estimate of the number of roots in the range; 0 is blank
    std::optional<std::int32_t> desiredModeCount; // the number of roots desired; empty when blank
    RealEigenMethodNorm normalization = RealEigenMethodNorm::Mass;
    base::EntityId normNode = 0;    // POINT: the grid or scalar point; 0 is blank
    std::int32_t normComponent = 0; // POINT: the component of a grid point (1 to 6); 0 is blank
};

} // namespace oor::model
