#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ComponentNodes.hpp"
#include "oor/query/named/common/ContactArguments.hpp"
#include "oor/query/named/topology/t6_connections/ConnectionQueries.hpp"

namespace oor::query
{

namespace
{

// connectionNodes: the component of every grid point, and the points only connections use (units).
class ConnectionNodesQuery final : public QueryBase
{
public:
    ConnectionNodesQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const auto components =
            analysis::ComponentModel::of(model, contactOptionsOf(run.arguments, "connectionNodes"), run.context.pool());
        return componentNodesTable(model, *components, info().defaultColumns(), run.context.pool());
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "connectionNodes";
        info.family = "T6";
        info.description =
            "The connection system point by point (units): every grid point of the model and "
            "every point an element or an MPC equation uses, with componentId, the component that holds it (null for "
            "a point no component holds), and isConnectionOnly, true for a point that only connections use (a CFAST "
            "or a CWELD uses the grid points of the shells it pierces and its GA and GB, never GS; a mass on such a "
            "point is a connection). The same table as connectedComponents(detail=\"nodes\").";
        info.parameters = contactParameters();
        info.defaultSort = "nodeId";
        ColumnListBuilder columns;
        addComponentNodeColumns(columns);
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addConnectionNodesQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ConnectionNodesQuery>());
}

} // namespace oor::query
