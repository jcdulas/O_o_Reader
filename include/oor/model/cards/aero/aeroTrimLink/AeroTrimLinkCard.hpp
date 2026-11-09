#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/aero/LabeledValues.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One AELINK card: a linear relation that fixes one trim variable (the dependent one) as a combination of others, so
// that the trim does not solve for it.
//
// Card layout: AELINK  ID     LABLD  LABL1  C1  LABL2  C2  LABL3  C3
//                     LABL4  C4     -etc.-
// The relation is: dependent variable + C1 x LABL1 + C2 x LABL2 + ... = 0. The pairs (LABLi, Ci) start in field 3 and
// run to the last field used, two fields to a pair, four pairs to a continuation line; at least one is given. A blank
// pair (padding at the end of a line) is skipped. LABLD and the LABLi are the labels of an AESURF, AESTAT or AEPARM,
// kept as written and matched without regard to the case by a check between cards; a variable is the
// dependent one of a single AELINK among the cards that apply to one subcase (the cards of its ID and those of ID 0:
// a check between cards too). The card does not refuse a repeated LABLi, which only adds its
// coefficients.
//
// ID selects the trim: a positive ID is the set of the TRIM (or TRIM2) card that the Case Control TRIM names, and the
// relation holds for that subcase only; 0, or the word ALWAYS, makes it hold for every subcase.
// Several AELINK cards may share an ID (a deck may give one relation per card, all with the same ID), so the
// space is a group and nothing is visited: the ID names a trim set, but the Case Control decides which set a subcase
// takes and a check between cards may compare the two. The two spellings of "every subcase" are kept apart so that the
// text reads back as it was written: `always` is true only when the word was written, and then `id` is 0.
struct AeroTrimLinkCard
{
    base::EntityId id = 0;           // 0: every subcase
    bool always = false;             // written as the word ALWAYS instead of 0
    std::string dependentLabel;      // the dependent variable, up to eight characters
    std::vector<LabeledValue> terms; // the independent variables and their coefficients

    [[nodiscard]] bool operator==(const AeroTrimLinkCard&) const noexcept = default;
};

} // namespace oor::model
