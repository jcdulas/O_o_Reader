#pragma once

#include <string>
#include <vector>

namespace oor::model
{

// One EXCLUDE card: the Bulk Data entries of a primary module that a secondary (copied) module ignores.
//
// Card layout: EXCLUDE  ENTRY1  ENTRY2  ENTRY3  -etc.-
// The entries are in fields 1, 2, 3, ... on as many lines as it takes. Each ENTRY is the neutral name of a card type
// (SinglePointConstr, PointForceLoad, ...), kept as written. Excluding Node is a fatal error for the solver (grid
// points can be neither ignored nor replaced) and the card refuses it. The card belongs in the Bulk Data section of the
// secondary module: a `BEGIN MODULE` section, which is a part of the deck; the reader keeps the card
// wherever it stands. A card names no ID of the model.
struct ModuleEntryExclusionCard
{
    std::vector<std::string> entries; // ENTRY1, ENTRY2, ...: names of Bulk Data entries, at least one

    [[nodiscard]] bool operator==(const ModuleEntryExclusionCard&) const noexcept = default;
};

} // namespace oor::model
