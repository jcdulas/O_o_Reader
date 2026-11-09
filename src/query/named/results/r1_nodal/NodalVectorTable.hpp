#pragma once

#include "oor/query/named/QueryInfo.hpp"
#include "oor/query/named/results/ResultQuery.hpp"

#include <memory>

namespace oor::query
{

// The catalogue entry of the queries that list a nodal vector table (displacements, velocities, accelerations, SPC and
// MPC forces): `subcase`, `[step]`, `nodeId`, `t1 t2 t3 r1 r2 r3`, `magnitude`, `frame`.
[[nodiscard]] QueryInfo nodalVectorInfo(const std::string& name, const std::string& description);

// The rows of every table of `kind` that the context selects, as the query of that kind reports them. `magnitude` is
// the norm of the three translations.
[[nodiscard]] std::shared_ptr<const QueryTable> nodalVectorTable(const ResultQueryContext& context,
                                                                 const QueryInfo& info, results::ResultKind kind);

} // namespace oor::query
