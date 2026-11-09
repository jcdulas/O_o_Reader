#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// A degree of freedom of a SUPAX: a ring, a harmonic, and the digits of the conical shell degrees of freedom of the
// ring at that harmonic as one number.
struct AxisymConeSupportDof
{
    base::EntityId ring = 0;
    base::EntityId harmonic = 0;
    std::int32_t components = 0;

    [[nodiscard]] bool operator==(const AxisymConeSupportDof&) const noexcept = default;
};

// One SUPAX card: the fictitious supports of a conical shell free body, the determinate reactions of the analysis.
//
// Card layout: SUPAX  RID1  HID1  C1  RID2  HID2  C2
// The format reference draws two triples to a line and says up to 12 degrees of freedom may be on one card; the card
// keeps the triples in a list and writes them two to a line, the next ones on the continuation lines (fields 9 and 12,
// 17 and 20,
// ...). RIDi is the number of a ring (a RINGAX card), HIDi a harmonic number (the format reference asks for a positive
// integer, but the first harmonic is 0 and the cards that name harmonics elsewhere, MPCAX and SPCAX, take 0 or more, so
// this card does too) and Ci any unique combination of the digits 1 to 6. The model does not look the ring up (RINGAX
// is not typed). The card is allowed only when the deck has an AXIC card, and its degrees of freedom may not
// be on an MPCAX, SPCAX or OMITAX card. It has no ID.
struct AxisymConeSupportCard
{
    static constexpr std::size_t MAX_DOFS = 12;

    std::vector<AxisymConeSupportDof> dofs;

    [[nodiscard]] bool operator==(const AxisymConeSupportCard&) const noexcept = default;
};

} // namespace oor::model
