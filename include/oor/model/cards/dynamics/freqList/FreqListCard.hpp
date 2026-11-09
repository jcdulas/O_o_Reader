#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One FREQ card: a set of excitation frequencies listed one by one.
//
// Card layout: FREQ  SID  F1  F2  F3  F4  F5  F6  F7
//                   F8   F9  F10 -etc.-
// The frequencies are in cycles per unit time, in the order they are written (the solution sorts them and drops the
// duplicates; the card does not).
struct FreqListCard
{
    base::EntityId id = 0;
    std::vector<double> frequencies;
};

} // namespace oor::model
