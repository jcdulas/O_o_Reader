#pragma once

#include "oor/analysis/validation/engine/ValidationSupport.hpp"

#include <initializer_list>
#include <string>
#include <string_view>

// What the checks of compatibility share: which card types an element may have as its property, and a property as its
// material. The rules are lists of card names, taken from the rules of each element and property card.

namespace oor::analysis
{

// Reports each element of the card types `elements` whose property exists and is not one of the card types `allowed`.
// `needs` are the card types the message names as the right ones ("needs a PSHELL, PCOMP or PCOMPG"); all of `allowed`
// when it is empty.
void reportWrongPropertyType(const ValidationContext& context, FindingSink& sink,
                             std::initializer_list<std::string_view> elements,
                             std::initializer_list<std::string_view> allowed,
                             std::initializer_list<std::string_view> needs = {});

// Reports each material of the property card types `properties` whose card type is in `refused` ("MAT8" for a solid).
void reportRefusedMaterial(const ValidationContext& context, FindingSink& sink,
                           std::initializer_list<std::string_view> properties,
                           std::initializer_list<std::string_view> refused,
                           std::initializer_list<std::string_view> needs);

// True for the material field of a property that a heat conduction analysis reads: the PSHELL membrane material MID1
// (MID2 to MID4 are the bending, transverse shear and coupling materials, which conduction does not use), the plies of
// a laminate, and the single material of a solid, rod, tube, bar or beam property.
[[nodiscard]] bool isConductionMaterialField(std::string_view property, std::string_view field) noexcept;

// Reports each material of the property card types `properties` whose card type is not in `allowed`.
void reportUnlistedMaterial(const ValidationContext& context, FindingSink& sink,
                            std::initializer_list<std::string_view> properties,
                            std::initializer_list<std::string_view> allowed,
                            std::initializer_list<std::string_view> needs);

} // namespace oor::analysis
