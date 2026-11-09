#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ContactArguments.hpp"
#include "oor/query/named/topology/t6_connections/ConnectionQueries.hpp"

#include <algorithm>
#include <map>

namespace oor::query
{

namespace
{

struct Pair
{
    std::int64_t connections = 0;
    std::int64_t fasteners = 0;
    std::int64_t rigids = 0;
    std::int64_t mpcs = 0;
    std::int64_t lines = 0;
    std::int64_t contacts = 0;
    std::vector<std::int32_t> elements;
};

// interfaces: the pairs of components that connections join.
class InterfacesQuery final : public QueryBase
{
public:
    InterfacesQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const auto components =
            analysis::ComponentModel::of(*run.model, contactOptionsOf(run.arguments, "interfaces"), run.context.pool());
        std::map<std::pair<std::int32_t, std::int32_t>, Pair> pairs;
        for (std::size_t index = 0; index < components->connections().size(); ++index)
        {
            const analysis::Connection& connection = components->connections()[index];
            std::vector<std::int32_t> touched;
            for (const base::EntityId node : components->connectionNodes(index))
            {
                const std::int32_t component = components->componentOfNode(node);
                if (component != 0)
                    touched.push_back(component);
            }
            std::sort(touched.begin(), touched.end());
            touched.erase(std::unique(touched.begin(), touched.end()), touched.end());
            for (std::size_t first = 0; first < touched.size(); ++first)
            {
                for (std::size_t second = first + 1; second < touched.size(); ++second)
                {
                    Pair& pair = pairs[{touched[first], touched[second]}];
                    ++pair.connections;
                    switch (connection.kind)
                    {
                    case analysis::ConnectionKind::Fastener:
                        ++pair.fasteners;
                        break;
                    case analysis::ConnectionKind::Rigid:
                        ++pair.rigids;
                        break;
                    case analysis::ConnectionKind::Mpc:
                        ++pair.mpcs;
                        break;
                    case analysis::ConnectionKind::Line:
                        ++pair.lines;
                        break;
                    case analysis::ConnectionKind::Mass:
                        break;
                    case analysis::ConnectionKind::Contact:
                        ++pair.contacts;
                        break;
                    }
                    // A contact connection is listed by the ID of its table; a pairing without table has none.
                    const bool hasId = connection.kind != analysis::ConnectionKind::Mpc &&
                                       (connection.kind != analysis::ConnectionKind::Contact || connection.id != 0);
                    if (hasId)
                        pair.elements.push_back(connection.id);
                }
            }
        }
        std::vector<std::int64_t> first;
        std::vector<std::int64_t> second;
        std::vector<std::int64_t> connections;
        std::vector<std::int64_t> fasteners;
        std::vector<std::int64_t> rigids;
        std::vector<std::int64_t> mpcs;
        std::vector<std::int64_t> lines;
        std::vector<std::int64_t> contacts;
        std::vector<std::int64_t> offsets{0};
        std::vector<std::int32_t> elements;
        for (auto& [key, pair] : pairs)
        {
            first.push_back(key.first);
            second.push_back(key.second);
            connections.push_back(pair.connections);
            fasteners.push_back(pair.fasteners);
            rigids.push_back(pair.rigids);
            mpcs.push_back(pair.mpcs);
            lines.push_back(pair.lines);
            contacts.push_back(pair.contacts);
            std::sort(pair.elements.begin(), pair.elements.end());
            pair.elements.erase(std::unique(pair.elements.begin(), pair.elements.end()), pair.elements.end());
            elements.insert(elements.end(), pair.elements.begin(), pair.elements.end());
            offsets.push_back(static_cast<std::int64_t>(elements.size()));
        }
        TableBuilder builder;
        builder.int64("component1", std::move(first));
        builder.int64("component2", std::move(second));
        builder.int64("connectionCount", std::move(connections));
        builder.int64("fastenerCount", std::move(fasteners));
        builder.int64("rigidCount", std::move(rigids));
        builder.int64("mpcCount", std::move(mpcs));
        builder.int64("lineCount", std::move(lines));
        builder.int64("contactCount", std::move(contacts));
        builder.intList("connections", std::move(offsets), std::move(elements));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "interfaces";
        info.family = "T6";
        info.description =
            "The pairs of components joined by at least one connection (component1 < component2): the number of "
            "connections, of fasteners (springs, dampers, bushes, gaps, welds, general elements), rigid elements, MPC "
            "equations and 1D elements that bridge the two, and the element IDs of the connections (an MPC equation "
            "has none). "
            "A connection that touches three components is in the three pairs. A connection that only touches one "
            "component "
            "(a rigid region inside a plate) makes no interface, but its nodes are boundary nodes of the component "
            "(componentBoundaries). With contact=\"connection\", a pair of contact bodies that meets is a connection "
            "(contactCount; the ID of its table is among the connections); with the default contact=\"merge\" the "
            "bodies are one component and contactCount is 0.";
        info.parameters = contactParameters();
        info.defaultSort = "component1, component2";
        ColumnListBuilder columns;
        columns.int64("component1")
            .int64("component2")
            .int64("connectionCount")
            .int64("fastenerCount")
            .int64("rigidCount");
        columns.int64("mpcCount").int64("lineCount").int64("contactCount").intList("connections");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addInterfacesQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<InterfacesQuery>());
}

} // namespace oor::query
