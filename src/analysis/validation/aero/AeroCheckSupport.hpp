#pragma once

#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace oor::analysis
{

// The defining card of an exact aerodynamic control label.
struct ControlLabelOwner
{
    std::string cardName; // the card type: AeroControlSurface, AeroRigidTrimVariable or AeroController
    base::EntityId id = 0;
    model::CardLocation where;
};

// The defining trim-variable cards by exact label; the first definition is retained for a repeated label.
[[nodiscard]] std::unordered_map<std::string, ControlLabelOwner> controlLabels(const model::Model& model);

// The exact names defined by AeroComponent and AeroComponentUnion cards.
[[nodiscard]] std::unordered_set<std::string> componentNames(const model::Model& model);

// How many numbers the AEFACT `list` holds; nothing when no AEFACT has the ID.
[[nodiscard]] std::optional<std::size_t> aefactSize(const model::Model& model, base::EntityId list);

// How many elements a division makes: `count` equal divisions when it is positive, else one less than the points of the
// AEFACT `list` (0 for a list of fewer than two points); nothing when the list does not exist (`ref.unresolved` says
// so).
[[nodiscard]] std::optional<std::int64_t> divisionElements(const model::Model& model, std::int32_t count,
                                                           base::EntityId list);

} // namespace oor::analysis
