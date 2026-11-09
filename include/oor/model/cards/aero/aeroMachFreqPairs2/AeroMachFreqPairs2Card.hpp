#pragma once

#include <cstddef>
#include <vector>

namespace oor::model
{

// The most pairs one MKAERO2 card holds: four of a Mach number and a reduced frequency in its eight fields.
inline constexpr std::size_t AERO_MACH_FREQ_PAIRS2_MAX_PAIRS = 4;

// One pair of an MKAERO2 card: the aerodynamic matrices are computed at this Mach number and this reduced frequency.
struct AeroMachFreqPairs2Pair
{
    double mach = 0.0;      // 0.0 or more
    double frequency = 0.0; // above zero

    [[nodiscard]] bool operator==(const AeroMachFreqPairs2Pair&) const noexcept = default;
};

// One MKAERO2 card: a list of Mach number and reduced frequency pairs for the aerodynamic matrices.
//
// Card layout: MKAERO2  M1  K1  M2  K2  M3  K3  M4  K4
// Unlike MKAERO1, which takes every combination of its two lists, the card gives the pairs themselves. It holds one to
// four of them in a single line (no continuation is allowed) and a model may have several cards. The solver
// skips a pair whose two fields are blank in the middle of the line; the card keeps only the pairs that are given, so a
// card written back has them packed from the first field, and a pair with one field blank is refused. The Mach numbers
// are 0.0 or more and the frequencies above zero. That M = 1.0 is not supported except with a CAERO4 is a
// rule between cards, not checked here.
struct AeroMachFreqPairs2Card
{
    std::vector<AeroMachFreqPairs2Pair> pairs;
};

} // namespace oor::model
