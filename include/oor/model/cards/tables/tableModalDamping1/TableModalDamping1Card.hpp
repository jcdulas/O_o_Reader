#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// The units of the damping values of a TABDMP1 card.
enum class DampingType : std::uint8_t
{
    G,    // G: structural damping coefficient (the default)
    Crit, // CRIT: fraction of critical damping
    Q     // Q: quality factor
};

// One TABDMP1 card: modal damping as a function of natural frequency (selected by SDAMPING in Case Control).
//
// Card layout:
//   TABDMP1  TID  TYPE
//            f1   g1    f2   g2    f3   g3    -etc.-   ENDT
// The points follow each other from field 9, two fields each, over as many continuation lines as needed, and the word
// ENDT ends the table. f is ascending or descending; a repeated f makes a jump (the mean of the two values at it). A
// pair that has SKIP in a field is not kept.
struct TableModalDamping1Card
{
    base::EntityId id = 0;
    DampingType kind = DampingType::G;
    std::vector<double> x; // the natural frequencies
    std::vector<double> y; // the damping values
};

} // namespace oor::model
