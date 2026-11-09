#pragma once

#include "oor/model/cards/optimisation/DesignValue.hpp"
#include "oor/model/core/Model.hpp"

#include <optional>
#include <string_view>

namespace oor::model
{

// The value an optimisation parameter has in a model.
struct DesignOptimParamsValue
{
    DesignValue value;     // the value of the card, or the default of the catalogue
    bool fromDeck = false; // false when no DOPTPRM gives it and `value` is the default
};

// The value of the parameter `name` (its neutral name, MaxDesignCycles for DESMAX): the one the last DOPTPRM that
// names it gives, else the default of the catalogue (`DesignOptimParamsCatalogue.hpp`: the one of sizing and shape
// optimisation where the format reference gives two). Nullopt for a name the catalogue does not list that no card
// gives, and for a listed one that has no default (the program computes it).
[[nodiscard]] std::optional<DesignOptimParamsValue> designOptimParamsValue(const Model& model, std::string_view name);

// The same as a number, for an integer or a real value: the DELX that a DESVAR with no DELXV takes, the DESMAX that
// bounds the cycles.
[[nodiscard]] std::optional<double> designOptimParamsNumber(const Model& model, std::string_view name);

} // namespace oor::model
