#pragma once

#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <vector>

namespace oor::query
{

// The order of the rows of the exported table of one card type (`model.grid.table()`, `model.nodes.table()`,
// `model.pshell.table()`): its live cards sorted by ID, cards that share an ID in the order of their keys, as
// ModelSource orders them. The row-index columns (`nodesRows`, `nodeARow`, `propIdRow`, ...) and the node arrays
// (`xyzGlobal()`, `cd_rotations()`) index these rows, whatever the order of the cards in the
// deck: the storage row of a card is its reading order and is never exported.
class ExportedRowOrder
{
public:
    // The storage rows of the live cards of `type`, in the order of the rows of its exported table.
    [[nodiscard]] static std::vector<std::uint32_t> storageRows(const model::Model& model, std::uint16_t type);

    // For each storage row of `type` (dead ones included), its row in the exported table, or -1 for a dead row.
    [[nodiscard]] static std::vector<std::int32_t> tableRows(const model::Model& model, std::uint16_t type);
};

} // namespace oor::query
