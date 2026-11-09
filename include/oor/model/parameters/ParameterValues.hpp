#pragma once

#include "oor/model/cards/parameters/parameter/ParameterCard.hpp"
#include "oor/model/cards/solution_control/modelParams/ModelParamsCard.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::model
{

// The value a parameter has in a model.
struct ParameterValue
{
    ParameterCard card;    // the PARAM card, or the format reference default as a card
    bool fromDeck = false; // false when no PARAM card gives it and `card` is the default of the catalogue
};

// The value of the parameter `name` (its neutral name, exactly: "WeightToMass"): the last PARAM card of that name,
// which is the one the solver uses, or the format reference default of the catalogue when the model has none. Nullopt
// for a name the catalogue does not know and a known one that has no default (the program computes it).
[[nodiscard]] std::optional<ParameterValue> parameterValue(const Model& model, std::string_view name);

// The same as a number, when the parameter is a real or an integer: the last card of that name that holds a number (a
// word or a blank value is ignored, as the solver would refuse it), else the default.
[[nodiscard]] std::optional<double> realParameter(const Model& model, std::string_view name);

// The same as an integer, for the last card that holds an integer (a real is not an integer parameter value), else the
// default.
[[nodiscard]] std::optional<std::int32_t> integerParameter(const Model& model, std::string_view name);

// The model parameter `name` of the MDLPRM cards (its neutral name, exactly: "OffsetDefinition"): the last card that
// gives it, or nullopt when none does (the default of the format is then the caller's).
[[nodiscard]] std::optional<ModelParameter> modelParameter(const Model& model, std::string_view name);

} // namespace oor::model
