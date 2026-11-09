#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// The IDs one CAERO card takes: its EID and the following IDs. The boxes of a CAERO1 are numbered from its EID on (EID
// to EID + boxes - 1), and so are the strips of a CAERO4 and a CAERO5, the elements of a CAERO2 and the (x, y) points
// of a CAERO3. A card always owns at least its EID, even when the number of its boxes is not known (the AEFACT that
// gives it is missing): `countKnown` is then false and `boxCount` is 1.
struct AeroBoxRange
{
    base::EntityId panelId = 0; // the EID of the card, its first box
    base::EntityId boxCount = 1;
    CardLocation location;
    bool countKnown = true;

    [[nodiscard]] base::EntityId lastBox() const noexcept { return panelId + boxCount - 1; }
};

// The ranges of every CAERO card of a model, by increasing EID: which card owns an aerodynamic box. The SPLINE and
// AELIST cards name boxes (ReferenceTarget::AeroBox), and a renumbering of the CAERO cards moves them with their owner.
class AeroBoxRanges
{
public:
    explicit AeroBoxRanges(const Model& model);

    [[nodiscard]] const std::vector<AeroBoxRange>& ranges() const noexcept { return m_ranges; }
    // The range that holds the box (the card with the highest EID not above it when ranges overlap), or null when no
    // card owns it.
    [[nodiscard]] const AeroBoxRange* owner(base::EntityId box) const noexcept;

private:
    std::vector<AeroBoxRange> m_ranges;
};

// How many boxes a division makes: `count` equal divisions when it is positive, else one less than the number of
// division points of the AEFACT `list` (0 when that list does not exist or has fewer than two points).
[[nodiscard]] base::EntityId divisionCount(const Model& model, std::int32_t count, base::EntityId list);

} // namespace oor::model
