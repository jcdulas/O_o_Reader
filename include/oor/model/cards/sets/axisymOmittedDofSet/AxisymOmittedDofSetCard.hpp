#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace oor::model
{

// A degree of freedom of an OMITAX: a ring, a harmonic, and the digits of the conical shell degrees of freedom of the
// ring at that harmonic as one number (in ascending order).
struct AxisymOmittedDofSetDof
{
    base::EntityId ring = 0;
    base::EntityId harmonic = 0;
    std::int32_t components = 0;

    [[nodiscard]] bool operator==(const AxisymOmittedDofSetDof&) const noexcept = default;
};

// One OMITAX card: degrees of freedom of a conical shell model that are excluded (the o-set) from the analysis set (the
// a-set). The card needs an AXIC card in the deck, which is a rule between cards. SOL 400 does not support it.
//
// Card layout: OMITAX  RID1  HID1  C1  RID2  HID2  C2
// The format draws two triples to a line and says up to 12 degrees of freedom may be on one card; the card keeps the
// triples in a list and writes them two to a line, the next ones on the continuation lines (fields 9 and 12, 17 and 20,
// ...), as the SUPAX card of the same layout does. RIDi is the number of a ring (a RINGAX card: a node reference), HIDi
// a harmonic number (zero or more) and Ci any unique combination of the digits 1 to 6. Degrees of freedom that are on
// an OMITAX card may not be on an MPCAX, SUPAX or SPCAX card: a rule between cards. The card has no ID.
struct AxisymOmittedDofSetCard
{
    static constexpr std::size_t MAX_DOFS = 12;

    std::vector<AxisymOmittedDofSetDof> dofs;

    [[nodiscard]] bool operator==(const AxisymOmittedDofSetCard&) const noexcept = default;
};

} // namespace oor::model
