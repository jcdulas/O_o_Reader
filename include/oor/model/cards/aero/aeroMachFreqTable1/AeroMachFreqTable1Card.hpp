#pragma once

#include <cstddef>
#include <vector>

namespace oor::model
{

// The most Mach numbers, and the most reduced frequencies, one MKAERO1 card holds: a line of eight fields each.
inline constexpr std::size_t AERO_MACH_FREQ_TABLE1_MAX_VALUES = 8;

// One MKAERO1 card: a table of Mach numbers and reduced frequencies at which the aerodynamic matrices are computed.
//
// Card layout: MKAERO1  M1  M2  M3  M4  M5  M6  M7  M8
//                      K1  K2  K3  K4  K5  K6  K7  K8
// The card gives one to eight Mach numbers (fields 1 to 8) and one to eight reduced frequencies (fields 9 to 16, the
// continuation line, which is required), and the matrices are computed for every pair of a Mach number and a frequency.
// A blank field ends a list, so a Mach number of 0.0 has to be written (a value after a blank would be lost: it is
// refused). A model may have several cards; together they give all the pairs of each card. The Mach numbers are 0.0 or
// more and the frequencies above zero. That 1.0 is not supported except with a CAERO4 is a rule between
// cards, not checked here; so is the bound the number of boxes of a chord puts on the largest frequency.
struct AeroMachFreqTable1Card
{
    std::vector<double> machs;       // M1 to M8
    std::vector<double> frequencies; // K1 to K8
};

} // namespace oor::model
