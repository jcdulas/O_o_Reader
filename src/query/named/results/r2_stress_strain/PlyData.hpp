#pragma once

#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/providers/quantities/CompositeQuantities.hpp"

#include <vector>

namespace oor::query
{

// The plies of the laminates of a call: their stresses from the stress tables, the failure index from the chain
// (stored, else recomputed), and what the model says of the plies. Each flat has the fields of `plyStressFields()` and
// `failureIndex`, `strengthRatio`, `bondIndex`, `fiberIndex`, `matrixIndex` (the theories of failure modes),
// `plyId`, `matId`, `theta`, `thickness` (NaN where unknown), `indexQuadratic`, `indexLinear`, the texts
// `failureMode` and `failureTheory`, and the field `failureProvider`: the provider of the failure index of each row
// in `providers` (-1: none). Not `whole`: only what a block of `readBlock` reads (no `matId`, `theta`, `thickness`,
// `indexQuadratic`, `indexLinear`, no text). The tables are read and the rows completed on the threads of the call.
struct PlyData
{
    std::vector<ElementFlat> flats;
    std::vector<ProviderInfo> providers;
    std::vector<ProviderAttempt> attempts;
};

[[nodiscard]] PlyData buildPlyData(const ResultQueryContext& context, const ElementRequest& request, bool whole = true);

// The block of rows of one flat of the plies, for `compositePlyTable`.
// The block takes the columns of `flat` (left empty).
[[nodiscard]] RowBlock plyBlock(const ResultQueryContext& context, ElementFlat&& flat,
                                const std::vector<std::int16_t>& providers);

} // namespace oor::query
