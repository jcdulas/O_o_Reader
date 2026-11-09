#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One constant of a DTABLE: a label and a real value, or a label and the number of a TABLEDi that gives the value as a
// function of frequency or time (a VALUi that is an integer points to a table).
struct DesignConstantsEntry
{
    std::string label;  // up to 8 characters; the equations and responses find a constant by it, ignoring the case
    double value = 0.0; // when `table` is 0
    base::EntityId table = 0; // the TABLEDi when the value was written as an integer

    [[nodiscard]] bool operator==(const DesignConstantsEntry&) const noexcept = default;
};

// One DTABLE card: named real constants for the equations of the design relations and responses. A deck may have
// several DTABLE cards.
//
// Card layout: DTABLE  LABL1  VALU1  LABL2  VALU2  LABL3  VALU3  LABL4  VALU4    fields 1 to 8
//                     LABL5  VALU5  ...                                          fields 9 to 16, and so on
// A pair with both fields blank is skipped (the format reference allows blank pairs at the end of a line).
struct DesignConstantsCard
{
    std::vector<DesignConstantsEntry> entries; // at least one
};

} // namespace oor::model
