#include "oor/query/named/common/ComponentNodes.hpp"

#include "oor/analysis/components/ComponentCache.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/query/named/TableBuilder.hpp"

#include <algorithm>
#include <vector>

namespace oor::query
{

std::shared_ptr<const QueryTable> componentNodesTable(const model::Model& model,
                                                      const analysis::ComponentModel& components,
                                                      const ColumnSet& empty, base::ThreadPool* pool)
{
    const auto entities = analysis::ComponentCache::of(model).entities(model, pool);
    std::vector<base::EntityId> nodes;
    if (const model::NodeTable* grids = model.findTable<model::NodeTable>())
    {
        for (std::uint32_t row = 0; row < grids->rows().rowCount(); ++row)
        {
            if (grids->rows().isAlive(row))
                nodes.push_back(grids->idAtRow(row));
        }
    }
    for (std::size_t index = 0; index < entities->size(); ++index)
    {
        for (const base::EntityId node : entities->nodesOf(index))
        {
            if (node != 0)
                nodes.push_back(node);
        }
    }
    std::sort(nodes.begin(), nodes.end());
    nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
    if (nodes.empty())
        return emptyTable(empty);
    std::vector<base::EntityId> used; // the points some connection uses
    for (std::size_t connection = 0; connection < components.connections().size(); ++connection)
    {
        const auto touched = components.connectionNodes(connection);
        used.insert(used.end(), touched.begin(), touched.end());
    }
    std::sort(used.begin(), used.end());
    used.erase(std::unique(used.begin(), used.end()), used.end());
    std::vector<std::int64_t> component(nodes.size());
    std::vector<std::uint8_t> componentValid(nodes.size());
    std::vector<std::uint8_t> connectionOnly(nodes.size());
    for (std::size_t index = 0; index < nodes.size(); ++index)
    {
        component[index] = components.componentOfNode(nodes[index]);
        componentValid[index] = component[index] != 0 ? 1 : 0;
        connectionOnly[index] =
            component[index] == 0 && std::binary_search(used.begin(), used.end(), nodes[index]) ? 1 : 0;
    }
    TableBuilder builder;
    builder.int32("nodeId", std::vector<std::int32_t>(nodes.begin(), nodes.end()));
    builder.int64("componentId", std::move(component), std::move(componentValid));
    builder.flag("isConnectionOnly", connectionOnly);
    return builder.finish();
}

void addComponentNodeColumns(ColumnListBuilder& columns)
{
    columns.int32("nodeId").int64("componentId").flag("isConnectionOnly");
}

} // namespace oor::query
