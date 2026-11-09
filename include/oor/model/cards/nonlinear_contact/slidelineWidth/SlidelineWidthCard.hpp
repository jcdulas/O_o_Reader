#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One entry of the width list of a BWIDTH: a single width (by == 0.0), or "first THRU last BY by", a range of widths
// whose values step from `first` to `last` by `by` (1.0 or -1.0 when BY is left out: stored resolved).
struct WidthRun
{
    double first = 0.0;
    double last = 0.0;
    double increment = 0.0; // 0.0 for a single width

    [[nodiscard]] bool isRange() const noexcept { return increment != 0.0; }
    [[nodiscard]] bool operator==(const WidthRun&) const noexcept = default;
};

// One BWIDTH card: the width (3-D slideline) or thickness (2-D) of each line segment of the BLSEG of the same ID.
//
// Card layout: BWIDTH  ID  W1  W2  W3  ...   or, with ranges,   BWIDTH  ID  W1  THRU  W2  BY  INC  ...
// over as many continuation lines as needed. With fewer widths than segments the last width goes on; with no BWIDTH the
// width is 1.0. A BWIDTH of a BLSEG that has one grid point gives the area of that point in W1.
struct SlidelineWidthCard
{
    base::EntityId segmentsId = 0; // the ID of the BLSEG
    std::vector<WidthRun> widths;
};

} // namespace oor::model
