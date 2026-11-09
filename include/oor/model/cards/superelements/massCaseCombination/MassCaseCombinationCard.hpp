#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// A term of a MASSSET: a scale factor and the mass increment it applies to (0 is the base mass case).
struct MassCaseCombinationTerm
{
    double scale = 0.0;
    base::EntityId massId = 0;

    [[nodiscard]] bool operator==(const MassCaseCombinationTerm&) const noexcept = default;
};

// One MASSSET card: the linear combination of mass cases that a subcase uses as its mass.
//
// Card layout: MASSSET  ID  S0  S1  ID1  S2  ID2  S3  ID3
//                      S4  ID4  -etc.-
// The mass of a subcase is S0 times the sum of the terms Si times the mass of the case IDi, where the case 0 is the
// base mass case and the others are the incremental mass cases of the Bulk Data sections BEGIN MASSID=n (or BEGIN
// SUPER=m MASSID=n for a part superelement). ID is the number the Case Control MASSSET names; it must be unique among
// the mass combinations and the mass increments (a check between cards), and the card is filed under it
// (IdSpace::PerCardType, category MassCombination). S0 and every Si are real numbers not below zero and default to 0.0;
// the number of the case of a term has no default, so a term has both. The pairs run on from the first line to the
// continuation lines without a break, and a blank pair is padding.
struct MassCaseCombinationCard
{
    base::EntityId id = 0;
    double overallScale = 0.0; // the overall scale factor
    std::vector<MassCaseCombinationTerm> terms;

    [[nodiscard]] bool operator==(const MassCaseCombinationCard&) const noexcept = default;
};

} // namespace oor::model
