#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ColumnHelpers.hpp"
#include "oor/query/named/common/Topology.hpp"
#include "oor/query/named/topology/t5_mesh_checks/MeshCheckQueries.hpp"

#include <algorithm>
#include <numeric>

namespace oor::query
{

namespace
{

// The corner nodes of an element in the order of the card.
std::vector<std::int32_t> cornersOf(std::uint16_t type, std::span<const std::int32_t> nodes)
{
    std::vector<std::int32_t> corners;
    for (std::size_t position = 0; position < nodes.size(); ++position)
    {
        if (isCornerPosition(type, position))
            corners.push_back(nodes[position]);
    }
    return corners;
}

// True when `other` goes round the same corners the same way as `reference`: a rotation of it for a shell, the same
// sequence for anything else (a line, a solid).
bool sameWay(const std::vector<std::int32_t>& reference, const std::vector<std::int32_t>& other, bool ring)
{
    if (reference.size() != other.size())
        return false;
    if (!ring)
        return reference == other;
    const std::size_t count = reference.size();
    for (std::size_t shift = 0; shift < count; ++shift)
    {
        bool equal = true;
        for (std::size_t position = 0; position < count && equal; ++position)
            equal = reference[position] == other[(position + shift) % count];
        if (equal)
            return true;
    }
    return false;
}

// duplicateElements: elements on the same corner nodes.
class DuplicateElementsQuery final : public QueryBase
{
public:
    DuplicateElementsQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const bool sameType = run.arguments.boolOr("sameType", true);
        const model::Model& model = *run.model;
        std::vector<std::uint16_t> types;
        for (const std::uint16_t type : topologicalTypes())
        {
            // A mass, a rigid element or a fastener sits on its nodes: elements on the same node are not duplicates.
            if (analysis::dimensionOf(analysis::ElementTopology::shapeOf(type)) >= 1)
                types.push_back(type);
        }
        const CardList list = listCards(model, types, run.arguments.ids("elementIds"));
        const NodeLists nodes = nodesOfCards(model, list, run.context);
        const std::size_t count = list.size();

        // The sorted distinct corners of each element, and a hash of them (and of the type when it counts).
        std::vector<std::vector<std::int32_t>> corners(count);
        std::vector<std::vector<std::int32_t>> ordered(count);
        std::vector<std::uint64_t> hashes(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            corners[index] = cornersOf(list.types[index], nodes.of(index));
            ordered[index] = corners[index];
            std::sort(ordered[index].begin(), ordered[index].end());
            ordered[index].erase(std::unique(ordered[index].begin(), ordered[index].end()), ordered[index].end());
            std::uint64_t hash = sameType ? 0x9E3779B97F4A7C15ULL * (list.types[index] + 1U) : 0xCBF29CE484222325ULL;
            for (const std::int32_t node : ordered[index])
                hash = (hash ^ static_cast<std::uint64_t>(static_cast<std::uint32_t>(node))) * 0x100000001B3ULL;
            hashes[index] = hash;
        }
        std::vector<std::uint32_t> order;
        for (std::size_t index = 0; index < count; ++index)
        {
            if (ordered[index].size() >= 2)
                order.push_back(static_cast<std::uint32_t>(index));
        }
        std::sort(order.begin(), order.end(), [&](std::uint32_t left, std::uint32_t right)
                  { return hashes[left] != hashes[right] ? hashes[left] < hashes[right] : left < right; });
        // Groups: elements with the same hash that really have the same corners (and type).
        std::vector<std::vector<std::uint32_t>> groups;
        for (std::size_t at = 0; at < order.size();)
        {
            std::size_t end = at;
            while (end < order.size() && hashes[order[end]] == hashes[order[at]])
                ++end;
            std::vector<std::vector<std::uint32_t>> inRun;
            for (std::size_t item = at; item < end; ++item)
            {
                const std::uint32_t element = order[item];
                bool placed = false;
                for (auto& group : inRun)
                {
                    const std::uint32_t head = group.front();
                    if (ordered[head] == ordered[element] && (!sameType || list.types[head] == list.types[element]))
                    {
                        group.push_back(element);
                        placed = true;
                        break;
                    }
                }
                if (!placed)
                    inRun.push_back({element});
            }
            for (auto& group : inRun)
            {
                if (group.size() >= 2)
                    groups.push_back(std::move(group));
            }
            at = end;
        }
        if (groups.empty())
            return emptyTable(info().defaultColumns());
        // Each group in order of element ID (the list is), the groups by their first element.
        for (auto& group : groups)
            std::sort(group.begin(), group.end());
        std::sort(groups.begin(), groups.end(),
                  [](const std::vector<std::uint32_t>& left, const std::vector<std::uint32_t>& right)
                  { return left.front() < right.front(); });

        ListColumns columns(run.model, list, run.context);
        const std::vector<std::int32_t> pids = asInt32(columns.get("propId"));
        std::vector<std::int64_t> groupId;
        std::vector<std::int32_t> eid;
        std::vector<std::string> type;
        std::vector<std::int32_t> pid;
        std::vector<std::uint8_t> pidValid;
        std::vector<std::int64_t> offsets{0};
        std::vector<std::int32_t> nodeValues;
        std::vector<std::uint8_t> orientation;
        for (std::size_t number = 0; number < groups.size(); ++number)
        {
            const std::uint32_t head = groups[number].front();
            const bool ring = analysis::dimensionOf(analysis::ElementTopology::shapeOf(list.types[head])) == 2;
            for (const std::uint32_t element : groups[number])
            {
                groupId.push_back(static_cast<std::int64_t>(number + 1));
                eid.push_back(list.ids[element]);
                type.emplace_back(typeName(list.types[element]));
                pid.push_back(pids[element]);
                pidValid.push_back(pids[element] != 0 ? 1 : 0);
                for (const std::int32_t node : nodes.of(element))
                    nodeValues.push_back(node);
                offsets.push_back(static_cast<std::int64_t>(nodeValues.size()));
                orientation.push_back(element == head || sameWay(corners[head], corners[element], ring) ? 1 : 0);
            }
        }
        TableBuilder builder;
        builder.int64("groupId", std::move(groupId));
        builder.int32("elementId", std::move(eid));
        builder.dictionary("type", type);
        builder.int32("propId", std::move(pid), std::move(pidValid));
        builder.intList("nodes", std::move(offsets), std::move(nodeValues));
        builder.flag("sameOrientation", orientation);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "duplicateElements";
        info.family = "T5";
        info.description =
            "Groups of lines, shells and solids on the same corner nodes (masses, rigid elements and fasteners sit on "
            "their "
            "nodes and are not looked at). With sameType (the default) the elements of a group are of one card type. "
            "sameOrientation is whether the element goes round the corners the way the first element of the group "
            "does (a "
            "rotation of its corners for a shell, the same sequence for a line or a solid). Groups are numbered from 1 "
            "by "
            "their smallest element ID.";
        info.parameters = {
            parameter("sameType", ParameterKind::Bool, "True", "the elements of a group have one card type"),
            parameter("elementIds", ParameterKind::IdList, "None", "look only at these elements")};
        info.defaultSort = "groupId, elementId";
        ColumnListBuilder columns;
        columns.int64("groupId")
            .int32("elementId")
            .dictionary("type")
            .int32("propId")
            .intList("nodes")
            .flag("sameOrientation");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addDuplicateElementsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DuplicateElementsQuery>());
}

} // namespace oor::query
