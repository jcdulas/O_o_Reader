#pragma once

#include "oor/model/cards/optimisation/DesignValue.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// A parameter of a DOPTPRM card and its value.
struct DesignOptimParamsEntry
{
    std::string name;  // upper case
    DesignValue value; // an integer, a real or a word, as the deck wrote it

    [[nodiscard]] bool operator==(const DesignOptimParamsEntry&) const noexcept = default;
};

// One DOPTPRM card: values for the parameters of the design optimisation that override their defaults
// (`DesignOptimParamsCatalogue.hpp`: the kind and the default of each).
//
// Card layout: DOPTPRM  PARAM1  VAL1  PARAM2  VAL2  PARAM3  VAL3  PARAM4  VAL4        fields 1 to 8
//                      PARAM5  VAL5  -etc.-                                          fields 9 to 16, and so on
// Only one DOPTPRM card is allowed (the check `design.doptprm-repeated`). A name the table does not list is
// kept, as a PARAM of an unknown name is; a value of another kind than the table says is kept too and reported by the
// check `design.doptprm-kind`. A pair with both fields blank is skipped.
struct DesignOptimParamsCard
{
    std::vector<DesignOptimParamsEntry> entries; // at least one
};

} // namespace oor::model
