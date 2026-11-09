#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstddef>
#include <vector>

namespace oor::model
{

// A value of a DDVAL, or a range of them "first THRU last BY increment". A single value has `last` equal to `first` and
// an increment of 0 (a range never has 0: `isRange`).
struct DesignDiscreteValuesRun
{
    double first = 0.0;
    double last = 0.0;
    double increment = 0.0;

    [[nodiscard]] bool isRange() const noexcept { return increment != 0.0; }
    [[nodiscard]] bool operator==(const DesignDiscreteValuesRun&) const noexcept = default;
};

// One DDVAL card: the discrete values a design variable may take (DESVAR DDVAL), a topometry variable (TOMVAR DSVID),
// the rotor speeds of a Campbell diagram (CAMPBLL DDVALID) or a relative spin rate (RSPINR SPTID).
//
// Card layout: DDVAL  ID  DVAL1  DVAL2  DVAL3  DVAL4  DVAL5  DVAL6  DVAL7          fields 1 to 8
//             DDVAL  ID  DVAL1  THRU  DVAL2  BY  INC                              the alternate format
//                    DVAL8  DVAL9  DVAL10  ...                                     continuation lines: eight to a line
//                    DVAL8  THRU  DVAL9  BY  INC                                   in either format, in any order
// The values and ranges are kept as written, in order: the values of a sequence need not be ascending and
// the card is written back as it was read. A range goes from `first` to `last` by `increment`, which has the sign of
// the way it goes (positive when first is below last, negative when above), and `last` is always one of the
// values even when the range is not evenly divisible by the increment: `designDiscreteValuesValues` gives
// them. The format draws no range without BY, so a THRU with no BY is refused; the values it expands keep the order of
// the card. A range stays on one line when it is written (it takes five fields; the format asks the rest of the line to
// be blank). The ID is unique among the DDVAL cards.
struct DesignDiscreteValuesCard
{
    base::EntityId id = 0;
    std::vector<DesignDiscreteValuesRun> values;

    [[nodiscard]] bool operator==(const DesignDiscreteValuesCard&) const noexcept = default;
};

// The most values a range may stand for: a DDVAL is a short list of discrete values, and a range that would give more
// than this is a mistake of the increment, which `check` refuses (it would also be a list nobody can build).
inline constexpr std::size_t MAX_DISCRETE_RANGE_VALUES = 1000000;

// The values of a card as one list, the ranges expanded: first, first + increment, ..., and always `last` (a value of
// the sequence that comes within a rounding error of `last` is `last`). The order is the order of the card (not
// sorted).
[[nodiscard]] std::vector<double> designDiscreteValuesValues(const DesignDiscreteValuesCard& card);

} // namespace oor::model
