#pragma once

#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/query/named/QueryInfo.hpp"
#include "oor/query/table/QueryTable.hpp"

#include <memory>

namespace oor::query
{

// Every grid point of the model and every point an element or an MPC equation uses, with its component (null for a
// point no component holds) and isConnectionOnly (a point that only connections use; a CFAST or a CWELD uses the
// grid points of the shells it pierces and its GA and GB, not GS): the table of connectedComponents(detail="nodes")
// and of connectionNodes. `empty` gives the columns of a model with no point.
[[nodiscard]] std::shared_ptr<const QueryTable> componentNodesTable(const model::Model& model,
                                                                    const analysis::ComponentModel& components,
                                                                    const ColumnSet& empty, base::ThreadPool* pool);

// The columns of that table.
void addComponentNodeColumns(ColumnListBuilder& columns);

} // namespace oor::query
