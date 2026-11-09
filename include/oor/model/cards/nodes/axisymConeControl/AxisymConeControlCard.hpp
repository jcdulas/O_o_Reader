#pragma once

#include <cstdint>

namespace oor::model
{

// One AXIC card: it says the problem is an axisymmetric conical shell problem (RINGAX, CCONEAX, POINTAX and the load
// and constraint cards of the same family are then allowed, and most other cards are not: the format reference lists
// the ones that stay, a rule between cards).
//
// Card layout:
//   AXIC  H
//
// H is the highest harmonic of the problem, from 0 up to 997. Only one AXIC card is allowed in a deck;
// the table reports a second one when the deck is read, as the other cards of which one is allowed do.
struct AxisymConeControlCard
{
    static constexpr std::int32_t MAX_HARMONIC = 997;

    std::int32_t highestHarmonic = 0;
};

} // namespace oor::model
