#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One NLFREQ card: the forcing frequencies of a nonlinear harmonic response, listed one by one.
//
// Card layout: NLFREQ  ID  F1  F2  F3  F4  F5  F6  F7
//                         F8  F9  -etc.-
// The frequencies are in cycles per unit time, in any order (the order is the order of the analysis, which carries the
// solution of one frequency over to the next). They follow from field 2 over as many continuation lines as needed.
struct NonlinearHarmonicFreqListCard
{
    base::EntityId id = 0; // the NLFREQ field of an NLHARM names it
    std::vector<double> frequencies;
};

} // namespace oor::model
