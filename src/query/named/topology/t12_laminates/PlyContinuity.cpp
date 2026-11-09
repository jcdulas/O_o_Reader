#include "oor/analysis/mass/Laminate.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/LaminateCards.hpp"
#include "oor/query/named/common/NodePositions.hpp"
#include "oor/query/named/common/PropertyZoning.hpp"
#include "oor/query/named/common/ShellEdges.hpp"
#include "oor/query/named/topology/t12_laminates/LaminateQueries.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>
#include <unordered_map>

namespace oor::query
{

namespace
{

constexpr double ANGLE_EPSILON = 1.0e-6; // degrees: two angles that agree to this are the same

// The way two plies are matched: by global ply ID when both laminates are PCOMPG, else by material and angle.
bool samePly(const PlyInfo& left, const PlyInfo& right, bool byGlobalId)
{
    if (byGlobalId)
        return left.globalId == right.globalId;
    double difference = std::fmod(std::fabs(left.angle - right.angle), 360.0);
    difference = std::min(difference, 360.0 - difference);
    return left.material == right.material && difference <= ANGLE_EPSILON;
}

// What the comparison of two stacks found.
struct Match
{
    std::int64_t common = 0;
    std::vector<std::int32_t> dropped; // the plies of the first (from 1) that the second does not have
    std::int64_t longestDrop = 0;
    bool blended = false;
};

// The plies of `thick` that `thin` has, in order: the longest common subsequence, found by dynamic programming and
// traced back from the end so that the plies matched are the earliest possible.
Match compare(const LaminateInfo& thick, const LaminateInfo& thin)
{
    const bool byId = thick.hasGlobalIds && thin.hasGlobalIds;
    const std::size_t n = thick.plies.size();
    const std::size_t m = thin.plies.size();
    std::vector<std::vector<std::int32_t>> table(n + 1, std::vector<std::int32_t>(m + 1, 0));
    for (std::size_t i = n; i-- > 0;)
    {
        for (std::size_t j = m; j-- > 0;)
        {
            table[i][j] = samePly(thick.plies[i], thin.plies[j], byId) ? table[i + 1][j + 1] + 1
                                                                       : std::max(table[i + 1][j], table[i][j + 1]);
        }
    }
    Match match;
    match.common = table[0][0];
    std::vector<std::uint8_t> kept(n, 0);
    for (std::size_t i = 0, j = 0; i < n && j < m;)
    {
        if (samePly(thick.plies[i], thin.plies[j], byId) && table[i][j] == table[i + 1][j + 1] + 1)
        {
            kept[i] = 1;
            ++i;
            ++j;
        }
        else if (table[i + 1][j] >= table[i][j + 1])
            ++i;
        else
            ++j;
    }
    std::int64_t run = 0;
    for (std::size_t i = 0; i < n; ++i)
    {
        if (kept[i] != 0)
        {
            run = 0;
            continue;
        }
        match.dropped.push_back(static_cast<std::int32_t>(i + 1));
        match.longestDrop = std::max(match.longestDrop, ++run);
    }
    match.blended = match.common == static_cast<std::int64_t>(m);
    return match;
}

// plyContinuity: the plies two neighbouring laminate zones have in common and the plies that are dropped.
class PlyContinuityQuery final : public QueryBase
{
public:
    PlyContinuityQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const ShellSet shells(run, run.arguments.ids("elementIds"), ShellNeeds{});
        if (shells.size() == 0)
            return emptyTable(info().defaultColumns());
        // The laminates, by property ID.
        std::map<std::int32_t, LaminateInfo> laminates;
        std::vector<std::int32_t> zoneProperty(shells.size(), 0);
        for (std::size_t index = 0; index < shells.size(); ++index)
        {
            const std::int32_t pid = shells.property(index);
            if (pid == 0 || !shells.isSelected(index))
                continue;
            if (laminates.find(pid) == laminates.end())
            {
                const auto location = model.locate(base::IdSpace::Property, pid);
                std::optional<LaminateInfo> info;
                if (location)
                    info = readLaminate(model, *location);
                if (!info)
                    continue;
                laminates.emplace(pid, std::move(*info));
            }
            zoneProperty[index] = pid;
        }
        if (laminates.size() < 2)
            return emptyTable(info().defaultColumns());
        // The zones of one laminate: shells of one property that touch.
        NodeLists nodes;
        nodes.offsets.push_back(0);
        for (std::size_t index = 0; index < shells.size(); ++index)
        {
            const auto list = shells.nodes(index);
            nodes.nodes.insert(nodes.nodes.end(), list.begin(), list.end());
            nodes.offsets.push_back(static_cast<std::int64_t>(nodes.nodes.size()));
        }
        const std::vector<std::uint32_t> zoneOf = propertyZoneRoots(nodes, zoneProperty);

        // The edge length shared by each pair of zones.
        const ShellEdges edges(run, shells);
        const NodePositions positions(model, run.context.pool());
        std::map<std::pair<std::uint32_t, std::uint32_t>, double> shared;
        for (const ShellEdge& edge : edges.edges())
        {
            if (edge.count != 2)
                continue;
            const auto pair = edges.shellsOf(edge);
            if (zoneProperty[pair[0]] == 0 || zoneProperty[pair[1]] == 0 ||
                zoneProperty[pair[0]] == zoneProperty[pair[1]])
                continue;
            const analysis::EdgeKey key = edges.key(edge);
            std::uint32_t first = zoneOf[pair[0]];
            std::uint32_t second = zoneOf[pair[1]];
            if (first > second)
                std::swap(first, second);
            const double length = positions.distance(key.first, key.second);
            shared[{first, second}] += std::isfinite(length) ? length : 0.0;
        }
        struct Row
        {
            std::int32_t pid1;
            std::int32_t pid2;
            std::uint32_t zone1;
            std::uint32_t zone2;
            double length;
            Match match;
            std::int64_t count1;
            std::int64_t count2;
        };
        std::vector<Row> rows;
        for (const auto& [zones, length] : shared)
        {
            std::int32_t a = zoneProperty[zones.first];
            std::int32_t b = zoneProperty[zones.second];
            const LaminateInfo* infoA = &laminates.at(a);
            const LaminateInfo* infoB = &laminates.at(b);
            const double thickA = infoA->thickness();
            const double thickB = infoB->thickness();
            // pid1 is the thicker laminate (then the one with more plies, then the smaller property ID).
            const bool swap = thickB > thickA || (thickB == thickA && infoB->plies.size() > infoA->plies.size()) ||
                              (thickB == thickA && infoB->plies.size() == infoA->plies.size() && b < a);
            if (swap)
            {
                std::swap(a, b);
                std::swap(infoA, infoB);
            }
            Row row;
            row.pid1 = a;
            row.pid2 = b;
            row.zone1 = swap ? zones.second : zones.first;
            row.zone2 = swap ? zones.first : zones.second;
            row.length = length;
            row.match = compare(*infoA, *infoB);
            row.count1 = static_cast<std::int64_t>(infoA->plies.size());
            row.count2 = static_cast<std::int64_t>(infoB->plies.size());
            rows.push_back(std::move(row));
        }
        std::sort(rows.begin(), rows.end(),
                  [](const Row& left, const Row& right)
                  {
                      if (left.pid1 != right.pid1)
                          return left.pid1 < right.pid1;
                      if (left.pid2 != right.pid2)
                          return left.pid2 < right.pid2;
                      return std::tie(left.zone1, left.zone2) < std::tie(right.zone1, right.zone2);
                  });
        std::vector<std::int32_t> pid1;
        std::vector<std::int32_t> pid2;
        std::vector<double> length;
        std::vector<std::int64_t> count1;
        std::vector<std::int64_t> count2;
        std::vector<std::int64_t> common;
        std::vector<std::int64_t> droppedCount;
        std::vector<std::int64_t> droppedOffsets{0};
        std::vector<std::int32_t> dropped;
        std::vector<std::int64_t> longest;
        std::vector<std::uint8_t> blended;
        for (const Row& row : rows)
        {
            pid1.push_back(row.pid1);
            pid2.push_back(row.pid2);
            length.push_back(row.length);
            count1.push_back(row.count1);
            count2.push_back(row.count2);
            common.push_back(row.match.common);
            droppedCount.push_back(static_cast<std::int64_t>(row.match.dropped.size()));
            dropped.insert(dropped.end(), row.match.dropped.begin(), row.match.dropped.end());
            droppedOffsets.push_back(static_cast<std::int64_t>(dropped.size()));
            longest.push_back(row.match.longestDrop);
            blended.push_back(row.match.blended ? 1 : 0);
        }
        TableBuilder builder;
        builder.int32("propId1", std::move(pid1));
        builder.int32("propId2", std::move(pid2));
        builder.real("sharedEdgeLength", std::move(length));
        builder.int64("plyCount1", std::move(count1));
        builder.int64("plyCount2", std::move(count2));
        builder.int64("commonPlyCount", std::move(common));
        builder.int64("droppedPlyCount", std::move(droppedCount));
        builder.intList("droppedPlies", std::move(droppedOffsets), std::move(dropped));
        builder.int64("maxConsecutiveDrops", std::move(longest));
        builder.flag("isBlended", blended);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "plyContinuity";
        info.family = "T12";
        info.description =
            "One row for each pair of neighbouring property zones that are laminates (zones of one property whose "
            "shells touch; "
            "the zones share at least one edge): propId1 is the thicker laminate (then the one with more plies, then "
            "the "
            "smaller ID), propId2 the other, the length of the edges they share, the number of plies of each, the "
            "plies "
            "they have in "
            "common (the longest common subsequence of the two stacks), the plies of propId1 that propId2 does not "
            "have "
            "(dropped, "
            "by their number in the stack of propId1) and the longest run of consecutive dropped plies, and isBlended "
            "when "
            "the plies of propId2 are found, in order, among those of propId1. Plies are matched by global ply ID when "
            "both "
            "laminates are PCOMPG, else by material and angle, in stacking order. With elementIds only those shells "
            "are "
            "looked at.";
        info.parameters = {parameter("elementIds", ParameterKind::IdList, "None", "only these shells are looked at")};
        info.defaultSort = "propId1, propId2";
        ColumnListBuilder columns;
        columns.int32("propId1").int32("propId2").real("sharedEdgeLength").int64("plyCount1").int64("plyCount2");
        columns.int64("commonPlyCount").int64("droppedPlyCount").intList("droppedPlies").int64("maxConsecutiveDrops");
        columns.flag("isBlended");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addPlyContinuityQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<PlyContinuityQuery>());
}

} // namespace oor::query
