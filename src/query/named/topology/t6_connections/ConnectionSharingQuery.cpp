#include "oor/analysis/components/ComponentCache.hpp"
#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/analysis/connections/ConnectionSharing.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ContactArguments.hpp"
#include "oor/query/named/topology/t6_connections/ConnectionQueries.hpp"

#include <algorithm>
#include <optional>
#include <string>

namespace oor::query
{

namespace
{

// connectionSharing: per grid point, the connections that touch it.
class ConnectionSharingQuery final : public QueryBase
{
public:
    ConnectionSharingQuery() : QueryBase(makeInfo()) {}

    // Without nodeIds, the table is made once for a content of the model, the contact options and minCount: a call
    // again returns it as it is (the sharing of the connections was made again at each call, as the solver asks for
    // it at each of its steps).
    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const analysis::ContactOptions contact = contactOptionsOf(run.arguments, "connectionSharing");
        const std::int64_t minimum = run.arguments.integerOr("minCount", 1);
        std::optional<std::vector<std::int64_t>> wanted = run.arguments.ids("nodeIds");
        if (wanted)
            return build(run, contact, minimum, std::move(wanted));
        const model::Model& model = *run.model;
        const std::string key = "connectionSharing|" + contactKey(contact) + "|" + std::to_string(minimum);
        return std::static_pointer_cast<const QueryTable>(analysis::ComponentCache::of(model).memo(
            model, key,
            [&] { return std::static_pointer_cast<const void>(build(run, contact, minimum, std::nullopt)); }));
    }

private:
    [[nodiscard]] std::shared_ptr<const QueryTable> build(const QueryRun& run, const analysis::ContactOptions& contact,
                                                          std::int64_t minimum,
                                                          std::optional<std::vector<std::int64_t>> wanted) const
    {
        const model::Model& model = *run.model;
        const auto components = analysis::ComponentModel::of(model, contact, run.context.pool());
        const analysis::ConnectionSharing sharing = analysis::ConnectionSharing::build(*components);
        if (wanted)
            std::sort(wanted->begin(), wanted->end());

        std::vector<std::int32_t> nid;
        std::vector<std::int64_t> count;
        std::vector<std::int64_t> offsets{0};
        std::vector<std::string> kinds;
        std::vector<std::string> cards;
        std::vector<std::int32_t> ids;
        std::vector<std::int32_t> equations;
        for (const base::EntityId node : sharing.nodes())
        {
            if (wanted && !std::binary_search(wanted->begin(), wanted->end(), static_cast<std::int64_t>(node)))
                continue;
            const auto touching = sharing.connectionsAt(node);
            if (static_cast<std::int64_t>(touching.size()) < minimum)
                continue;
            nid.push_back(node);
            count.push_back(static_cast<std::int64_t>(touching.size()));
            for (const std::uint32_t connection : touching)
            {
                const analysis::Connection& item = components->connections()[connection];
                kinds.emplace_back(analysis::connectionClassName(sharing.classOf(connection)));
                cards.emplace_back(model.findTable(item.card.table)->info().name);
                ids.push_back(item.id);
                equations.push_back(sharing.equationOf(connection));
            }
            offsets.push_back(static_cast<std::int64_t>(ids.size()));
        }
        if (nid.empty())
            return emptyTable(info().defaultColumns());
        TableBuilder builder;
        builder.int32("nodeId", std::move(nid));
        builder.int64("connectionCount", std::move(count));
        builder.textList("kinds", offsets, kinds);
        builder.textList("cards", offsets, cards);
        builder.intList("ids", offsets, std::move(ids));
        builder.intList("equations", std::move(offsets), std::move(equations));
        return builder.finish();
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "connectionSharing";
        info.family = "T6";
        info.description =
            "Per grid point, the connections that touch it: rigid elements (kind rigid), RBE3 (rigidInterpolation), "
            "MPC equations (mpc), CFAST and CWELD through the grid points of the shells they pierce and their "
            "own GA and GB (fastener), and the other connection elements: springs, dampers, bushes, gaps, "
            "a 1D element or a mass between two components (connector), and with contact=\"connection\" the pairs of "
            "contact bodies that meet, on their node pairs (contact: the card is the table, the ID its ID). kinds, "
            "cards, ids and equations are aligned: "
            "ids gives the element ID, or the set ID of an MPC equation, and equations the number of an MPC equation "
            "among the MPC cards of its set (as multiPointConstrTable numbers them for that set; 0 for any other "
            "connection). "
            "minCount keeps the grid points touched by at least that many connections (2: the grid points that "
            "several connections share).";
        info.parameters = {parameter("minCount", ParameterKind::Integer, "1", "the fewest connections a node has"),
                           parameter("nodeIds", ParameterKind::IdList, "None", "keep these nodes")};
        const std::vector<ParameterInfo> contact = contactParameters();
        info.parameters.insert(info.parameters.end(), contact.begin(), contact.end());
        info.defaultSort = "nodeId";
        ColumnListBuilder columns;
        columns.int32("nodeId").int64("connectionCount").textList("kinds").textList("cards").intList("ids");
        columns.intList("equations");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addConnectionSharingQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ConnectionSharingQuery>());
}

} // namespace oor::query
