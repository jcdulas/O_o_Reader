#pragma once

#include "oor/model/cards/explicit/explicitParam/ExplicitParamCard.hpp"
#include "oor/model/core/Model.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

// The values a DYPARAM setting has in a model.
struct ExplicitParamSetting
{
    // One value for each value slot of the entry (more for a list), in the order of the fields from field 2: the value
    // the card gives, or the default of the format reference where the card leaves a blank. A blank value is a slot
    // with no default (the format states none, or computes it from other data).
    std::vector<ExplicitParamValue> values;
    bool fromDeck =
        false; // false when no DYPARAM card has the name and the values are the defaults of the format reference
};

// The setting of the neutral name: the last DYPARAM card of that name, which is the one the solver would end
// up with, else the defaults of the entry. For the names that come in several settings (CONTACT, SMP, MATRMERG,
// MATRMRG1) the last card is whichever came last, whatever its option or entity: read their cards from the table to
// tell them apart. Nullopt for a name the catalogue does not know that no card gives, and for a known name that has no
// default at all.
[[nodiscard]] std::optional<ExplicitParamSetting> explicitParamSetting(const Model& model, std::string_view name);

} // namespace oor::model
