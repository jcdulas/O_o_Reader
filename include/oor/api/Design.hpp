#pragma once

#include "oor/api/Deck.hpp"
#include "oor/model/design/DesignUpdate.hpp"

namespace oor::api
{
// Read on each operation: card edits cannot leave a cached design model behind.
[[nodiscard]] model::DesignModel designModel(const Deck& deck);
// IDs are explicit, so the solver's independent-variable order never gets confused with the reader's full order.
// All validation happens before the atomic model update. Default: reject field limit violations and save XINIT.
[[nodiscard]] model::DesignUpdate applyDesignPoint(Deck& deck, const std::vector<std::int32_t>& ids,
                                                   const std::vector<double>& values,
                                                   model::DesignLimitMode limits = model::DesignLimitMode::Error,
                                                   bool setInitialValues = true);
} // namespace oor::api
