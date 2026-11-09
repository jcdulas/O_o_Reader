#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/common/ColumnHelpers.hpp"
#include "oor/query/named/topology/t1_inventory/InventoryQueries.hpp"

namespace oor::query
{

namespace
{

// nodesTable: the grid points, their systems, coordinates and the number of elements that use them.
class NodesTableQuery final : public QueryBase
{
public:
    NodesTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const std::array<std::uint16_t, 1> nodeTypes{model::typeIndexOf<model::NodeTable>()};
        const CardList list = listCards(*run.model, nodeTypes, run.arguments.ids("nodeIds"));
        if (list.size() == 0)
            return emptyTable(info().defaultColumns());
        ListColumns columns(run.model, list, run.context);
        const ListOrigin origin = originOf(list);
        TableBuilder builder;
        builder.column("nodeId", columns.get("id"), ColumnOrigin::Stored, "id");
        builder.column("positionFrameId", columns.get("positionFrameId"), ColumnOrigin::Stored, "positionFrameId");
        builder.column("displacementFrameId", columns.get("displacementFrameId"), ColumnOrigin::Stored,
                       "displacementFrameId");
        const QueryColumn& xyz = columns.get("xyz");
        builder.real("x1", component(xyz, 0));
        builder.real("x2", component(xyz, 1));
        builder.real("x3", component(xyz, 2));
        builder.column("x", columns.get("x"), ColumnOrigin::Computed, "x");
        builder.column("y", columns.get("y"), ColumnOrigin::Computed, "y");
        builder.column("z", columns.get("z"), ColumnOrigin::Computed, "z");
        builder.column("permanentConstraints", columns.get("permanentConstraints"), ColumnOrigin::Stored,
                       "permanentConstraints");
        builder.column("superelementId", columns.get("superelementId"), ColumnOrigin::Stored, "superelementId");
        builder.column("elementCount", columns.get("elementCount"), ColumnOrigin::Computed, "elementCount");
        builder.rows(origin.types, origin.keys, run.link);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "nodesTable";
        info.family = "T1";
        info.description =
            "The grid points: system of position (cp) and of displacement (cd), the coordinates in the cp system (x1 "
            "x2 x3, "
            "read only: edit a position in the basic system with x y z), the coordinates in the basic system (x y z, "
            "editable: converted back to the cp system by apply), permanent constraints (ps), superelement "
            "(superelementId) and "
            "the "
            "number of elements that use the node.";
        info.parameters = {parameter("nodeIds", ParameterKind::IdList, "None", "keep these grid points")};
        info.defaultSort = "nodeId";
        ColumnListBuilder columns;
        columns.int32("nodeId").int32("positionFrameId", true).int32("displacementFrameId", true);
        columns.reals("x1 x2 x3");
        columns.reals("x y z", true);
        columns.int32("permanentConstraints", true).int32("superelementId", true).int64("elementCount");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addNodesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<NodesTableQuery>());
}

} // namespace oor::query
