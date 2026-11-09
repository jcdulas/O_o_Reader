#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/NodePositions.hpp"
#include "oor/query/named/common/SetSelection.hpp"
#include "oor/query/named/common/ShellEdges.hpp"
#include "oor/query/named/topology/t3_boundaries/BoundaryQueries.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <numeric>
#include <string>
#include <tuple>

namespace oor::query
{

namespace
{

// One free edge: its nodes (smaller ID first, as in the edge table) and the shell that carries it (the first layer of
// its stack). The free edges of a call are in the order of the edge table (by their nodes).
struct FreeEdge
{
    std::int32_t n1;
    std::int32_t n2;
    std::uint32_t owner;
};

// One free edge in the walk of a loop: the edge (its index among the free edges) and the node the walk enters it by.
struct Step
{
    std::uint32_t edge;
    std::int32_t from;
    std::int32_t to;
};

struct Loop
{
    std::vector<Step> steps;
    bool closed = false;
};

// The rows of the table, made by one call of chainLoops or more (one per group).
struct Rows
{
    std::vector<std::string> group;
    std::vector<std::int64_t> loopId;
    std::vector<std::int32_t> position;
    std::vector<std::int32_t> from;
    std::vector<std::int32_t> to;
    std::vector<std::int32_t> eid;
    std::vector<std::int32_t> pid;
    std::vector<double> length;
    std::vector<std::uint8_t> closed;

    void append(Rows&& other)
    {
        const auto move = [](auto& into, auto& from)
        { into.insert(into.end(), std::make_move_iterator(from.begin()), std::make_move_iterator(from.end())); };
        move(group, other.group);
        move(loopId, other.loopId);
        move(position, other.position);
        move(from, other.from);
        move(to, other.to);
        move(eid, other.eid);
        move(pid, other.pid);
        move(length, other.length);
        move(closed, other.closed);
    }
};

// The stacks of the selected shells: shells on the same corner grid points (a skin and its doubler) are the layers of
// one place, which carries an edge once, as `panels` counts them. Each shell gets the number of its stack.
std::vector<std::uint32_t> stacksOf(const ShellSet& shells)
{
    struct Entry
    {
        std::array<std::int32_t, 4> corners;
        std::uint32_t index;
    };
    std::vector<Entry> entries;
    entries.reserve(shells.size());
    for (std::size_t index = 0; index < shells.size(); ++index)
    {
        if (!shells.isSelected(index))
            continue;
        Entry entry{{0, 0, 0, 0}, static_cast<std::uint32_t>(index)};
        const std::vector<std::int32_t> corners = shells.corners(index);
        for (std::size_t at = 0; at < corners.size() && at < entry.corners.size(); ++at)
            entry.corners[at] = corners[at];
        std::sort(entry.corners.begin(), entry.corners.end());
        entries.push_back(entry);
    }
    std::sort(entries.begin(), entries.end(), [](const Entry& left, const Entry& right)
              { return std::tie(left.corners, left.index) < std::tie(right.corners, right.index); });
    std::vector<std::uint32_t> stack(shells.size(), std::numeric_limits<std::uint32_t>::max());
    std::uint32_t number = 0;
    for (std::size_t at = 0; at < entries.size(); ++at)
    {
        if (at > 0 && entries[at].corners != entries[at - 1].corners)
            ++number;
        stack[entries[at].index] = number;
    }
    return stack;
}

// Chains the free edges of one boundary (in the order of the edge table) into ordered loops, numbered from 1 by their
// smallest edge, and appends their rows. `group` names the group of the rows (empty: no group).
void chainLoops(const std::vector<FreeEdge>& edges, const ShellSet& shells, const NodePositions& positions,
                const std::string& group, Rows& rows)
{
    const std::size_t count = edges.size();
    if (count == 0)
        return;
    // The edges at each node: (node, edge) sorted, so that the edges of a node are together.
    std::vector<std::pair<std::int32_t, std::uint32_t>> incident;
    incident.reserve(2 * count);
    for (std::size_t edge = 0; edge < count; ++edge)
    {
        incident.emplace_back(edges[edge].n1, static_cast<std::uint32_t>(edge));
        incident.emplace_back(edges[edge].n2, static_cast<std::uint32_t>(edge));
    }
    std::sort(incident.begin(), incident.end());
    const auto edgesAt = [&](std::int32_t node)
    {
        const auto first = std::lower_bound(incident.begin(), incident.end(), std::make_pair(node, std::uint32_t{0}));
        auto last = first;
        while (last != incident.end() && last->first == node)
            ++last;
        return std::pair{first, last};
    };
    const auto otherEnd = [&](std::size_t edge, std::int32_t node)
    { return edges[edge].n1 == node ? edges[edge].n2 : edges[edge].n1; };

    std::vector<std::uint8_t> visited(count, 0);
    std::vector<Loop> loops;
    // The next unvisited edge at a node: the one that leads to the smallest node.
    const auto nextEdge = [&](std::int32_t node) -> std::int64_t
    {
        const auto [first, last] = edgesAt(node);
        std::int64_t best = -1;
        std::int32_t bestNode = std::numeric_limits<std::int32_t>::max();
        for (auto at = first; at != last; ++at)
        {
            if (visited[at->second] != 0)
                continue;
            const std::int32_t other = otherEnd(at->second, node);
            if (other < bestNode)
            {
                bestNode = other;
                best = at->second;
            }
        }
        return best;
    };
    const auto walk = [&](std::int32_t start, std::uint32_t first)
    {
        Loop loop;
        std::int32_t current = start;
        std::int64_t edge = first;
        while (edge >= 0)
        {
            const auto index = static_cast<std::size_t>(edge);
            visited[index] = 1;
            const std::int32_t other = otherEnd(index, current);
            loop.steps.push_back(Step{static_cast<std::uint32_t>(edge), current, other});
            current = other;
            // A chain ends where the boundary ends or branches (a node that is not on exactly two edges).
            const auto [firstAt, lastAt] = edgesAt(current);
            edge = lastAt - firstAt == 2 ? nextEdge(current) : -1;
        }
        loop.closed = loop.steps.size() > 1 && current == start;
        loops.push_back(std::move(loop));
    };
    // Chains start at the nodes where the boundary ends or branches (not two edges); what is left is closed loops.
    std::vector<std::int32_t> nodes;
    for (const auto& entry : incident)
    {
        if (nodes.empty() || nodes.back() != entry.first)
            nodes.push_back(entry.first);
    }
    for (const std::int32_t node : nodes)
    {
        const auto [first, last] = edgesAt(node);
        if (last - first == 2)
            continue;
        for (std::int64_t edge = nextEdge(node); edge >= 0; edge = nextEdge(node))
            walk(node, static_cast<std::uint32_t>(edge));
    }
    for (std::size_t edge = 0; edge < count; ++edge)
    {
        if (visited[edge] == 0)
            walk(edges[edge].n1, static_cast<std::uint32_t>(edge));
    }
    // The loops are numbered by the smallest edge they hold (the edges are sorted by their nodes).
    std::vector<std::uint32_t> smallest(loops.size());
    for (std::size_t index = 0; index < loops.size(); ++index)
    {
        std::uint32_t low = std::numeric_limits<std::uint32_t>::max();
        for (const Step& step : loops[index].steps)
            low = std::min(low, step.edge);
        smallest[index] = low;
    }
    std::vector<std::size_t> order(loops.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) { return smallest[a] < smallest[b]; });

    for (std::size_t number = 0; number < order.size(); ++number)
    {
        const Loop& loop = loops[order[number]];
        for (std::size_t at = 0; at < loop.steps.size(); ++at)
        {
            const Step& step = loop.steps[at];
            const std::uint32_t owner = edges[step.edge].owner;
            rows.group.push_back(group);
            rows.loopId.push_back(static_cast<std::int64_t>(number + 1));
            rows.position.push_back(static_cast<std::int32_t>(at + 1));
            rows.from.push_back(step.from);
            rows.to.push_back(step.to);
            rows.eid.push_back(shells.id(owner));
            rows.pid.push_back(shells.property(owner));
            rows.length.push_back(positions.distance(step.from, step.to));
            rows.closed.push_back(loop.closed ? 1 : 0);
        }
    }
}

// The free edges of the selected shells: the edges that one stack carries, with its first layer.
std::vector<FreeEdge> freeEdgesOf(const ShellEdges& edges, const std::vector<std::uint32_t>& stack)
{
    std::vector<FreeEdge> result;
    for (const ShellEdge& edge : edges.edges())
    {
        const auto onEdge = edges.shellsOf(edge);
        const bool oneStack = std::all_of(onEdge.begin(), onEdge.end(),
                                          [&](std::uint32_t shell) { return stack[shell] == stack[onEdge[0]]; });
        if (!oneStack)
            continue;
        const analysis::EdgeKey key = edges.key(edge);
        result.push_back(FreeEdge{key.first, key.second, onEdge[0]});
    }
    return result;
}

// The free edges of each group in one pass over the edges of all of them: the edges of each shell, then per group the
// edges that one of its stacks carries.
void groupLoops(const QueryRun& run, const std::vector<NamedIdSet>& groups,
                const std::vector<std::vector<std::int32_t>>& members, const ShellSet& shells, const ShellEdges& edges,
                const std::vector<std::uint32_t>& stack, Rows& rows)
{
    // The edges of each shell, as positions in edges.edges() (which are in the order of the edge table).
    const std::span<const ShellEdge> all = edges.edges();
    std::vector<std::uint32_t> offsets(shells.size() + 1, 0);
    for (const ShellEdge& edge : all)
        for (const std::uint32_t shell : edges.shellsOf(edge))
            ++offsets[shell + 1];
    std::partial_sum(offsets.begin(), offsets.end(), offsets.begin());
    std::vector<std::uint32_t> edgesOfShell(offsets.back());
    {
        std::vector<std::uint32_t> next(offsets.begin(), offsets.end() - 1);
        for (std::size_t position = 0; position < all.size(); ++position)
            for (const std::uint32_t shell : edges.shellsOf(all[position]))
                edgesOfShell[next[shell]++] = static_cast<std::uint32_t>(position);
    }
    const std::span<const std::int32_t> ids = shells.list().ids;
    const NodePositions positions(*run.model, run.context.pool());

    constexpr std::size_t GROUPS_PER_CHUNK = 64;
    const std::size_t chunks = (groups.size() + GROUPS_PER_CHUNK - 1) / GROUPS_PER_CHUNK;
    std::vector<Rows> parts(chunks);
    run.context.parallel(groups.size(), GROUPS_PER_CHUNK,
                         [&](base::IndexRange range)
                         {
                             Rows& part = parts[range.begin / GROUPS_PER_CHUNK];
                             struct Entry
                             {
                                 std::uint32_t edge;
                                 std::uint32_t stack;
                                 std::uint32_t shell;
                             };
                             std::vector<Entry> entries;
                             std::vector<FreeEdge> free;
                             for (std::size_t number = range.begin; number < range.end; ++number)
                             {
                                 entries.clear();
                                 free.clear();
                                 for (const std::int32_t id : members[number])
                                 {
                                     const auto found = std::lower_bound(ids.begin(), ids.end(), id);
                                     if (found == ids.end() || *found != id)
                                         continue; // not a shell
                                     const auto shell = static_cast<std::uint32_t>(found - ids.begin());
                                     for (std::uint32_t at = offsets[shell]; at < offsets[shell + 1]; ++at)
                                         entries.push_back(Entry{edgesOfShell[at], stack[shell], shell});
                                 }
                                 std::sort(entries.begin(), entries.end(),
                                           [](const Entry& left, const Entry& right)
                                           {
                                               return std::tie(left.edge, left.stack, left.shell) <
                                                      std::tie(right.edge, right.stack, right.shell);
                                           });
                                 for (std::size_t at = 0; at < entries.size();)
                                 {
                                     std::size_t end = at + 1;
                                     bool oneStack = true;
                                     while (end < entries.size() && entries[end].edge == entries[at].edge)
                                     {
                                         oneStack = oneStack && entries[end].stack == entries[at].stack;
                                         ++end;
                                     }
                                     if (oneStack)
                                     {
                                         const analysis::EdgeKey key = edges.key(all[entries[at].edge]);
                                         free.push_back(FreeEdge{key.first, key.second, entries[at].shell});
                                     }
                                     at = end;
                                 }
                                 chainLoops(free, shells, positions, groups[number].name, part);
                             }
                         });
    for (Rows& part : parts)
        rows.append(std::move(part));
}

// freeEdges: the shell edges carried by one stack of shells, chained into ordered loops, for one selection of shells
// or for each group of several.
class FreeEdgesQuery final : public QueryBase
{
public:
    FreeEdgesQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        Rows rows;
        bool grouped = false;
        if (run.arguments.has("groups"))
        {
            if (run.arguments.has("elementIds"))
                throw QueryError("freeEdges: give elementIds or groups, not both");
            grouped = true;
            const std::vector<NamedIdSet> groups = run.arguments.idSets("groups");
            std::vector<std::vector<std::int32_t>> members;
            members.reserve(groups.size());
            std::vector<std::int64_t> everyone;
            for (const NamedIdSet& group : groups)
            {
                members.push_back(idSetMembers(*run.model, group, "freeEdges"));
                everyone.insert(everyone.end(), members.back().begin(), members.back().end());
            }
            const ShellSet shells(run, everyone, ShellNeeds{});
            const ShellEdges edges(run, shells);
            groupLoops(run, groups, members, shells, edges, stacksOf(shells), rows);
        }
        else
        {
            const ShellSet shells(run, run.arguments.ids("elementIds"), ShellNeeds{});
            const ShellEdges edges(run, shells);
            const std::vector<FreeEdge> free = freeEdgesOf(edges, stacksOf(shells));
            if (!free.empty())
                chainLoops(free, shells, NodePositions(*run.model, run.context.pool()), std::string(), rows);
        }
        if (rows.loopId.empty())
            return emptyTable(info().defaultColumns());

        std::vector<std::uint8_t> pidValid(rows.pid.size());
        for (std::size_t index = 0; index < rows.pid.size(); ++index)
            pidValid[index] = rows.pid[index] != 0 ? 1 : 0;
        TableBuilder builder;
        builder.text("group", rows.group, std::vector<std::uint8_t>(rows.group.size(), grouped ? 1 : 0));
        builder.int64("loopId", std::move(rows.loopId));
        builder.int32("position", std::move(rows.position));
        builder.int32("n1", std::move(rows.from));
        builder.int32("n2", std::move(rows.to));
        builder.int32("elementId", std::move(rows.eid));
        builder.int32("propId", std::move(rows.pid), std::move(pidValid));
        builder.real("length", std::move(rows.length));
        builder.flag("loopIsClosed", rows.closed);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "freeEdges";
        info.family = "T3";
        info.description =
            "The shell edges carried by one stack of shells, chained into ordered loops: loopId and position (both "
            "from 1) give the order, n1 -> n2 follows the walk along the loop, elementId and propId are the element "
            "that "
            "carries the edge. Shells on the same corner grid points (a skin and its doubler) are the layers of one "
            "stack, which counts once as in `panels`: the edge of a stack alone is free, and elementId is its first "
            "layer "
            "(the smallest element ID). A loop is closed when the walk comes back to its first node; an open chain "
            "starts and ends where the boundary ends or branches. Loops are numbered by their smallest edge (by node "
            "IDs). With elementIds the edges are those of that selection of elements (its boundary), the other "
            "elements being treated as absent. With groups (a dict {name: element IDs} or a list of SET1 IDs, named "
            "by their ID) the loops of the boundary of each group in one call, in the order of the groups, `group` "
            "naming it and loopId counted from 1 in each group: the rows of a group are those of a call with "
            "elementIds=its elements (`group` is null without groups); the IDs that are not shells are ignored.";
        info.parameters = {
            parameter("elementIds", ParameterKind::IdList, "None", "the elements whose boundary to find"),
            parameter("groups", ParameterKind::IdSets, "None",
                      "{name: element IDs} or SET1 IDs: the boundary of each group")};
        info.defaultSort = "group, loopId, position";
        ColumnListBuilder columns;
        columns.text("group")
            .int64("loopId")
            .int32("position")
            .int32("n1")
            .int32("n2")
            .int32("elementId")
            .int32("propId")
            .real("length")
            .flag("loopIsClosed");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addFreeEdgesQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<FreeEdgesQuery>());
}

} // namespace oor::query
