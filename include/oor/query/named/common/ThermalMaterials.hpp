#pragma once

#include "oor/model/core/Model.hpp"
#include "oor/query/named/common/CardFacts.hpp"

#include <optional>

namespace oor::query
{

// The heat transfer material (a MAT4 or MAT5) of the conduction element at `element`: the material of its property, or
// its own for a CONROD. Nothing when the element has no thermal material.
[[nodiscard]] std::optional<model::CardLocation> thermalMaterialOfElement(CardFacts& facts, const model::Model& model,
                                                                          model::CardLocation element);

// The multiplier HGEN of the thermal material of the element: the QVOL of a heat load is multiplied by it. 1.0 (the
// default of the cards) when the element has no thermal material.
[[nodiscard]] double heatGenerationMultiplier(CardFacts& facts, const model::Model& model, model::CardLocation element);

// The conductivity K of a MAT4 (nothing for a MAT5, whose conductivity is a matrix in the material system, or a MAT4
// with a blank K).
[[nodiscard]] std::optional<double> isotropicConductivity(const model::Model& model, model::CardLocation material);

// True when a MATT4 of the same MID gives the conductivity of the MAT4 a table of the temperature.
[[nodiscard]] bool conductivityDependsOnTemperature(const model::Model& model, base::EntityId mid);

} // namespace oor::query
