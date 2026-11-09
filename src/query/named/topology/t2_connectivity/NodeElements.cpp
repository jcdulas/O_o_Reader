#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/Topology.hpp"
#include "oor/query/named/topology/t2_connectivity/ConnectivityQueries.hpp"

#include <algorithm>

namespace oor::query
{

namespace
{

// nodeElements: one row for each node of each element.
class NodeElementsQuery final : public QueryBase
{
public:
    NodeElementsQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const TopologyList topology = TopologyList::build(*run.model, run.context, run.arguments.ids("elementIds"));
        const auto nodeFilter = sortedIds(run.arguments.ids("nodeIds"));
        struct Row
        {
            std::int32_t node;
            std::uint32_t element; // position in the list
            std::int32_t position; // from 0
        };
        std::vector<Row> rows;
        rows.reserve(topology.nodes.nodes.size());
        for (std::size_t index = 0; index < topology.list.size(); ++index)
        {
            std::int32_t position = 0;
            for (const std::int32_t node : topology.nodes.of(index))
            {
                if (!nodeFilter ||
                    std::binary_search(nodeFilter->begin(), nodeFilter->end(), static_cast<std::int64_t>(node)))
                    rows.push_back(Row{node, static_cast<std::uint32_t>(index), position});
                ++position;
            }
        }
        const CardList& list = topology.list;
        std::sort(rows.begin(), rows.end(),
                  [&](const Row& left, const Row& right)
                  {
                      if (left.node != right.node)
                          return left.node < right.node;
                      if (list.ids[left.element] != list.ids[right.element])
                          return list.ids[left.element] < list.ids[right.element];
                      return left.position < right.position;
                  });
        std::vector<std::int32_t> nid(rows.size());
        std::vector<std::int32_t> eid(rows.size());
        std::vector<std::string> type(rows.size());
        std::vector<std::int32_t> local(rows.size());
        std::vector<std::uint8_t> corner(rows.size());
        for (std::size_t index = 0; index < rows.size(); ++index)
        {
            const Row& row = rows[index];
            nid[index] = row.node;
            eid[index] = list.ids[row.element];
            type[index] = std::string(typeName(list.types[row.element]));
            local[index] = row.position + 1;
            corner[index] = isCornerPosition(list.types[row.element], static_cast<std::size_t>(row.position)) ? 1 : 0;
        }
        TableBuilder builder;
        builder.int32("nodeId", std::move(nid));
        builder.int32("elementId", std::move(eid));
        builder.dictionary("type", type);
        builder.int32("localIndex", std::move(local));
        builder.flag("isCorner", corner);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "nodeElements";
        info.family = "T2";
        info.description =
            "One row for each node of each element: the element, its type, the position of the node in the node list "
            "of the "
            "card (from 1) and whether it is a corner node (not a mid-side node). The orientation node of a bar or a "
            "bush is "
            "not one of the nodes. Sorted by node, then element.";
        info.parameters = {parameter("nodeIds", ParameterKind::IdList, "None", "keep these nodes"),
                           parameter("elementIds", ParameterKind::IdList, "None", "keep these elements")};
        info.defaultSort = "nodeId, elementId";
        ColumnListBuilder columns;
        columns.int32("nodeId").int32("elementId").dictionary("type").int32("localIndex").flag("isCorner");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addNodeElementsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<NodeElementsQuery>());
}

} // namespace oor::query
