#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One AELISTC card: a list of character strings of at most eight characters that other aeroelastic data name by the SID
// (the format reference says nothing more of what the strings stand for).
//
// Card layout: AELISTC  SID  C1  C2  C3  C4  C5  C6  C7
//                      C8   -etc.-
// The strings run from field 2 to the last field used, with no blank between two of them; the first line
// holds seven, the rest are on the continuation lines and the format reference draws no limit. At least one is given. A
// string is kept as written and, to read back from every format, has no blank, comma, dollar sign or quote. The card
// names no other card and no card names it by a typed reference.
struct AeroLabelListCard
{
    base::EntityId id = 0;
    std::vector<std::string> names; // the strings C1, C2, ...

    [[nodiscard]] bool operator==(const AeroLabelListCard&) const noexcept = default;
};

} // namespace oor::model
