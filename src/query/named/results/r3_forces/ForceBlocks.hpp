#pragma once

#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/providers/quantities/ElementQuantities.hpp"

#include <memory>
#include <string>
#include <vector>

namespace oor::query
{

// Which of the columns besides the keys and the values a force table has.
struct ForceColumns
{
    bool location = false; // `location`
    bool station = false;  // `station` (x/L)
    bool node = false;     // `nodeId`
    bool frame = false;    // `frame`
    bool tension = false;  // `tension` = max(axial, 0)
};

// The block of rows of one flat of a force chain: `subcase`, the step, `elementId`, `type`, `propId`, the columns
// `options` asks for and the value fields `values`.
[[nodiscard]] RowBlock forceBlock(const ResultQueryContext& context, const ElementFlat& flat,
                                  const std::vector<std::string>& values, const ForceColumns& options,
                                  std::int16_t provider);

// Makes the table of a force query: the blocks of every flat of `resolution`, sorted. `info` is the catalogue entry.
[[nodiscard]] std::shared_ptr<const QueryTable> forceTable(const ResultQueryContext& context, const QueryInfo& info,
                                                           const ElementResolution& resolution,
                                                           const std::vector<std::string>& values,
                                                           const ForceColumns& options,
                                                           const std::vector<std::string>& sortKeys);

} // namespace oor::query
