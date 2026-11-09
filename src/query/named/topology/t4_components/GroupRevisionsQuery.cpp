#include "oor/analysis/components/RevisionStamp.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ContactArguments.hpp"
#include "oor/query/named/common/UnitGroups.hpp"
#include "oor/query/named/topology/t4_components/ComponentQueries.hpp"

#include <algorithm>
#include <cstring>

namespace oor::query
{

namespace
{

std::int64_t asInt64(std::uint64_t stamp)
{
    std::int64_t value = 0;
    std::memcpy(&value, &stamp, sizeof(value));
    return value;
}

// groupRevisions: what each group of the caller holds and its revision stamp.
class GroupRevisionsQuery final : public QueryBase
{
public:
    GroupRevisionsQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const std::string detail = run.arguments.textOr("detail", "groups");
        if (detail != "groups" && detail != "nodes" && detail != "elements")
            throw QueryError("groupRevisions: detail is \"groups\", \"nodes\" or \"elements\", not \"" + detail + "\"");
        const UnitGroups groups(model, run.arguments.groups("groups"), "groupRevisions",
                                contactOptionsOf(run.arguments, "groupRevisions"), run.context.pool());
        const std::size_t count = groups.groups().unitCount();
        if (count == 0)
            return emptyTable(info().columnsFor(detail));
        if (detail == "nodes")
            return nodesForm(groups);
        if (detail == "elements")
            return elementsForm(model, groups);
        std::vector<std::vector<model::CardLocation>> members(count);
        for (std::size_t index = 0; index < count; ++index)
            members[index] = groups.groups().memberCards(static_cast<std::int32_t>(index + 1));
        const std::vector<std::uint64_t> stamps =
            analysis::RevisionStamp::computeFor(model, members, run.context.pool());
        std::vector<std::string> name;
        std::vector<std::int64_t> elements;
        std::vector<std::int64_t> connections;
        std::vector<std::int64_t> nodes;
        std::vector<std::int64_t> revision;
        for (std::size_t index = 0; index < count; ++index)
        {
            const auto unit = static_cast<std::int32_t>(index + 1);
            name.push_back(groups.nameOf(unit));
            elements.push_back(static_cast<std::int64_t>(groups.groups().elementCount(unit)));
            connections.push_back(static_cast<std::int64_t>(groups.groups().connectionCount(unit)));
            nodes.push_back(static_cast<std::int64_t>(groups.groups().group(unit).nodes.size()));
            revision.push_back(asInt64(stamps[index]));
        }
        TableBuilder builder;
        builder.text("group", name);
        builder.int64("elementCount", std::move(elements));
        builder.int64("connectionCount", std::move(connections));
        builder.int64("nodeCount", std::move(nodes));
        builder.int64("revision", std::move(revision));
        return builder.finish();
    }

private:
    // detail="nodes": the grid points of each group.
    static std::shared_ptr<const QueryTable> nodesForm(const UnitGroups& groups)
    {
        std::vector<std::string> name;
        std::vector<std::int32_t> nid;
        for (std::size_t index = 0; index < groups.groups().unitCount(); ++index)
        {
            const auto unit = static_cast<std::int32_t>(index + 1);
            for (const base::EntityId node : groups.groups().group(unit).nodes)
            {
                name.push_back(groups.nameOf(unit));
                nid.push_back(node);
            }
        }
        TableBuilder builder;
        builder.text("group", name);
        builder.int32("nodeId", std::move(nid));
        return builder.finish();
    }

    // detail="elements": the member elements of each group, the listed ones and those it absorbs, by element ID.
    static std::shared_ptr<const QueryTable> elementsForm(const model::Model& model, const UnitGroups& groups)
    {
        std::vector<std::string> name;
        std::vector<std::int32_t> eid;
        std::vector<std::string> type;
        for (std::size_t index = 0; index < groups.groups().unitCount(); ++index)
        {
            const auto unit = static_cast<std::int32_t>(index + 1);
            const analysis::ResolvedGroup& group = groups.groups().group(unit);
            std::vector<std::uint32_t> members(group.listed.begin(), group.listed.end());
            members.insert(members.end(), group.absorbed.begin(), group.absorbed.end());
            std::vector<std::pair<base::EntityId, std::string>> elements;
            for (const std::uint32_t member : members)
            {
                const analysis::ModelEntity& entity = groups.entities().entity(member);
                if (!entity.isEquation) // an MPC equation is no element
                    elements.emplace_back(entity.id, model.findTable(entity.card.table)->info().name);
            }
            std::sort(elements.begin(), elements.end());
            for (const auto& [id, card] : elements)
            {
                name.push_back(groups.nameOf(unit));
                eid.push_back(static_cast<std::int32_t>(id));
                type.push_back(card);
            }
        }
        TableBuilder builder;
        builder.text("group", name);
        builder.int32("elementId", std::move(eid));
        builder.dictionary("type", type);
        return builder.finish();
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "groupRevisions";
        info.family = "T4";
        info.description =
            "The groups the caller gives, condensed as units: {name: {\"components\": IDs}} "
            "merges components, {name: {\"elements\": IDs}} is a zone of elements (connections included). A group "
            "holds its elements, every other element, rigid element or MPC equation whose grid points are all in it "
            "(a connection between two of its components, a mass a zone encloses) and the clusters of connections "
            "that touch only it (connectionClusters), with their connection-only grid points. elementCount counts "
            "the elements that are not connections, connectionCount the connections and MPC equations it holds, "
            "nodeCount its grid points. revision is a content hash of its cards and of what they depend on, as a "
            "signed int64, for caching a reduction of the group: equal content gives an equal stamp, also across "
            "sessions and after a write and a read. An "
            "ID that names nothing raises QueryError. detail=\"nodes\": the grid points of each group (group, nodeId); "
            "detail=\"elements\": its member elements, those given or of the components given and those it absorbs "
            "(group, elementId, type, by element ID; an MPC equation it absorbs is no element).";
        info.detailParameter = "detail";
        info.parameters = {
            parameter("groups", ParameterKind::Groups, "(required)",
                      "{name: {\"components\": IDs}} or {name: {\"elements\": IDs}}"),
            parameter("detail", ParameterKind::Text, "\"groups\"", "\"groups\", \"nodes\" or \"elements\"")};
        const std::vector<ParameterInfo> contact = contactParameters();
        info.parameters.insert(info.parameters.end(), contact.begin(), contact.end());
        info.defaultSort = "the order of the groups given";
        ColumnListBuilder columns;
        columns.text("group").int64("elementCount").int64("connectionCount").int64("nodeCount").int64("revision");
        info.columnSets.push_back(columns.build("groups"));
        ColumnListBuilder nodes;
        nodes.text("group").int32("nodeId");
        info.columnSets.push_back(nodes.build("nodes"));
        ColumnListBuilder elements;
        elements.text("group").int32("elementId").dictionary("type");
        info.columnSets.push_back(elements.build("elements"));
        return info;
    }
};

} // namespace

void addGroupRevisionsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<GroupRevisionsQuery>());
}

} // namespace oor::query
