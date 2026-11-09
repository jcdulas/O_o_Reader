#pragma once

#include "oor/analysis/validation/engine/ValidationCheck.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/model/core/CardTypeInfo.hpp"

#include <functional>
#include <string_view>

namespace oor::analysis
{

// Whether a card is left out of the comparison of the IDs (a card of the category whose ID is not of the kind
// compared).
using SkipCard = std::function<bool(const model::Model&, model::CardLocation)>;

// The cards of a category whose ID a card of another type has too, where the format reference wants the IDs of the category
// different (a BSID names a BSURF or a BCPROP, and so on). Two cards of one type are the reader's business
// (`ref.duplicate-id`). The card of the type that comes later in the registry is reported; `what` names the IDs in the
// message ("contact body").
void reportIdsSharedByTypes(const ValidationContext& context, FindingSink& sink, model::CardCategory category,
                            std::string_view what, const SkipCard& skip = {});

} // namespace oor::analysis
