#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/common/ColumnHelpers.hpp"
#include "oor/query/named/topology/t5_mesh_checks/MeshCheckQueries.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <utility>
#include <vector>

namespace oor::query
{

namespace
{

class Sets
{
public:
    explicit Sets(std::size_t count) : m_parent(count) { std::iota(m_parent.begin(), m_parent.end(), 0U); }
    std::uint32_t find(std::uint32_t item)
    {
        while (m_parent[item] != item)
        {
            m_parent[item] = m_parent[m_parent[item]];
            item = m_parent[item];
        }
        return item;
    }
    void join(std::uint32_t first, std::uint32_t second)
    {
        const std::uint32_t left = find(first);
        const std::uint32_t right = find(second);
        if (left != right)
            m_parent[std::max(left, right)] = std::min(left, right);
    }

private:
    std::vector<std::uint32_t> m_parent;
};

// The key of a cube of the grid: its three indexes packed in 63 bits (21 each). Far cubes may share a key; the
// candidates are checked by distance, so a shared key only costs comparisons. The points sorted by it follow the mesh
// (z, then y, then x): a mesh numbered along its rows is sorted almost as it is, and the points of a cube and of its
// neighbours are near in memory (a hash of the indexes put them anywhere: 187 ns a point at a million).
std::uint64_t cellKey(std::int64_t x, std::int64_t y, std::int64_t z) noexcept
{
    constexpr std::uint64_t MASK = (1ULL << 21U) - 1U;
    return (static_cast<std::uint64_t>(x) & MASK) | ((static_cast<std::uint64_t>(y) & MASK) << 21U) |
           ((static_cast<std::uint64_t>(z) & MASK) << 42U);
}

// The bits of a key mixed, for the slot of its cube in the table and its bit in the filter.
std::uint64_t mix(std::uint64_t key) noexcept
{
    key ^= key >> 33U;
    key *= 0xff51afd7ed558ccdULL;
    key ^= key >> 33U;
    return key;
}

// A position in [0, size) from 32 bits (size below 2^32: the points are counted in 32 bits).
std::size_t reduce(std::uint64_t bits, std::size_t size) noexcept
{
    return static_cast<std::size_t>(((bits & 0xffffffffULL) * size) >> 32U);
}

// The cubes that hold points: where the points of each start in the sorted list and how many there are, in a flat
// table with open addressing (a lookup is one probe into one array; a std::unordered_map of the cells took several
// cache misses a lookup: 569 ns a point at a million points, 337 with this table). Two slots a cube, the slot found
// from the key by a multiplication, not a mask: a table whose size doubled at each power of two made the cost of a
// point step there.
class CellRanges
{
public:
    explicit CellRanges(std::size_t cells) : m_slots(std::max<std::size_t>(16, 2 * cells)) {}

    void add(std::uint64_t key, std::uint32_t begin, std::uint32_t count)
    {
        std::size_t slot = slotOf(key);
        while (m_slots[slot].count != 0)
            slot = next(slot);
        m_slots[slot] = Slot{key, begin, count};
    }

    // Where the points of the cube start and how many there are: count 0 for a cube with no point.
    [[nodiscard]] std::pair<std::uint32_t, std::uint32_t> find(std::uint64_t key) const noexcept
    {
        std::size_t slot = slotOf(key);
        while (m_slots[slot].count != 0 && m_slots[slot].key != key)
            slot = next(slot);
        return {m_slots[slot].begin, m_slots[slot].count};
    }

private:
    struct Slot
    {
        std::uint64_t key = 0;
        std::uint32_t begin = 0;
        std::uint32_t count = 0; // 0: empty
    };

    [[nodiscard]] std::size_t slotOf(std::uint64_t key) const noexcept { return reduce(mix(key), m_slots.size()); }
    [[nodiscard]] std::size_t next(std::size_t slot) const noexcept
    {
        return slot + 1 == m_slots.size() ? 0 : slot + 1;
    }

    std::vector<Slot> m_slots;
};

// Whether a cube may hold points: a bit set for each cube that has, at other bits of the mixed key than its slot (the
// high 32). Eight bits a point (a megabyte for a million, which the caches keep): about one empty cube in eight finds
// its bit set by another, and the others are told empty without a look at the table, whose reads miss the caches.
class CellFilter
{
public:
    explicit CellFilter(std::size_t points) : m_bits(64 * ((8 * points + 63) / 64)), m_words(m_bits / 64, 0) {}

    void add(std::uint64_t key) noexcept
    {
        const std::size_t bit = bitOf(key);
        m_words[bit >> 6U] |= std::uint64_t{1} << (bit & 63U);
    }
    [[nodiscard]] bool mayHold(std::uint64_t key) const noexcept
    {
        const std::size_t bit = bitOf(key);
        return (m_words[bit >> 6U] >> (bit & 63U) & 1U) != 0;
    }

private:
    [[nodiscard]] std::size_t bitOf(std::uint64_t key) const noexcept { return reduce(mix(key) >> 32U, m_bits); }

    std::size_t m_bits;
    std::vector<std::uint64_t> m_words;
};

// coincidentNodes: the grid points closer than a tolerance, in groups.
class CoincidentNodesQuery final : public QueryBase
{
public:
    CoincidentNodesQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        if (!run.arguments.has("tolerance"))
            throw QueryError(
                "coincidentNodes: tolerance is required (the model has no units: give the distance below which "
                "two grid points coincide)");
        const double tolerance = run.arguments.realOr("tolerance", 0.0);
        if (!(tolerance > 0.0) || !std::isfinite(tolerance))
            throw QueryError("coincidentNodes: tolerance is a distance above zero");
        const std::array<std::uint16_t, 1> nodeTypes{model::typeIndexOf<model::NodeTable>()};
        const CardList list = listCards(*run.model, nodeTypes, run.arguments.ids("nodeIds"));
        const std::size_t count = list.size();
        if (count < 2)
            return emptyTable(info().defaultColumns());
        ListColumns columns(run.model, list, run.context);
        const std::vector<double> x = columns.reals("x");
        const std::vector<double> y = columns.reals("y");
        const std::vector<double> z = columns.reals("z");

        // The grid points by cube, found by sorting (cube key, point). The cubes are four times the tolerance wide and
        // shifted from the origin by 0.382 of a side: two points within the tolerance are in one cube, or in cubes that
        // touch through faces both are within the tolerance of, on each axis one face at most (the cube is wider than
        // twice the tolerance), and none for a point farther than that from both: half the points on each axis for
        // random coordinates, every point of a mesh on round coordinates (the shift puts them inside their cubes). A
        // point looks at its own cube (its run of the sorted list) and at no more than seven others, none on most
        // meshes; each finds the points within the tolerance of it, the pair from its lower index. (With cubes of the
        // tolerance, a point looked at 27: 320 ns a point at a million.)
        const double side = 4.0 * tolerance;
        const double shift = 0.381966 * side; // no fraction of a round number of sides
        const auto cubeOf = [&](double value) { return static_cast<std::int64_t>(std::floor((value + shift) / side)); };
        std::vector<std::array<std::int64_t, 3>> cube(count);
        std::vector<std::pair<std::uint64_t, std::uint32_t>> cells(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            cube[index] = {cubeOf(x[index]), cubeOf(y[index]), cubeOf(z[index])};
            cells[index] = {cellKey(cube[index][0], cube[index][1], cube[index][2]), static_cast<std::uint32_t>(index)};
        }
        std::sort(cells.begin(), cells.end());
        CellRanges ranges(count);
        CellFilter filter(count);
        std::vector<std::uint32_t> runBegin(count);
        std::vector<std::uint32_t> runEnd(count);
        for (std::size_t at = 0; at < count;)
        {
            std::size_t end = at;
            while (end < count && cells[end].first == cells[at].first)
                ++end;
            ranges.add(cells[at].first, static_cast<std::uint32_t>(at), static_cast<std::uint32_t>(end - at));
            filter.add(cells[at].first);
            std::fill(runBegin.begin() + static_cast<std::ptrdiff_t>(at),
                      runBegin.begin() + static_cast<std::ptrdiff_t>(end), static_cast<std::uint32_t>(at));
            std::fill(runEnd.begin() + static_cast<std::ptrdiff_t>(at),
                      runEnd.begin() + static_cast<std::ptrdiff_t>(end), static_cast<std::uint32_t>(end));
            at = end;
        }
        // The pairs of points within the tolerance, found chunk by chunk of the sorted list (each chunk fills a list of
        // its own, so that the pairs and the union that follows are the same for any number of threads).
        constexpr std::size_t CHUNK = 16384;
        const double squared = tolerance * tolerance;
        std::vector<std::vector<std::pair<std::uint32_t, std::uint32_t>>> pairs((count + CHUNK - 1) / CHUNK);
        run.context.parallel(count, CHUNK,
                             [&](base::IndexRange range)
                             {
                                 auto& found = pairs[range.begin / CHUNK];
                                 for (std::size_t at = range.begin; at < range.end; ++at)
                                 {
                                     const std::uint32_t index = cells[at].second;
                                     const std::array<double, 3> p{x[index], y[index], z[index]};
                                     const std::array<std::int64_t, 3>& own = cube[index];
                                     // The neighbouring cube on each axis where the point is within the tolerance of a
                                     // face of its own: below or above; `near` has a bit for each such axis.
                                     std::array<std::int64_t, 3> other = own;
                                     unsigned near = 0;
                                     for (std::size_t axis = 0; axis < 3; ++axis)
                                     {
                                         const double local = p[axis] + shift - static_cast<double>(own[axis]) * side;
                                         if (local <= tolerance)
                                             other[axis] = own[axis] - 1;
                                         else if (side - local <= tolerance)
                                             other[axis] = own[axis] + 1;
                                         if (other[axis] != own[axis])
                                             near |= 1U << axis;
                                     }
                                     const auto look = [&](std::size_t first, std::size_t last)
                                     {
                                         for (std::size_t place = first; place < last; ++place)
                                         {
                                             const std::uint32_t candidate = cells[place].second;
                                             if (candidate <= index)
                                                 continue;
                                             const double ex = x[candidate] - p[0];
                                             const double ey = y[candidate] - p[1];
                                             const double ez = z[candidate] - p[2];
                                             if (ex * ex + ey * ey + ez * ez <= squared)
                                                 found.emplace_back(index, candidate);
                                         }
                                     };
                                     look(runBegin[at], runEnd[at]); // its own cube
                                     for (unsigned choice = 1; choice < 8; ++choice)
                                     {
                                         if ((choice & ~near) != 0)
                                             continue; // a neighbour on an axis where the point is far from both faces
                                         const std::uint64_t key = cellKey((choice & 1U) != 0 ? other[0] : own[0],
                                                                           (choice & 2U) != 0 ? other[1] : own[1],
                                                                           (choice & 4U) != 0 ? other[2] : own[2]);
                                         if (!filter.mayHold(key))
                                             continue;
                                         const auto [first, length] = ranges.find(key);
                                         look(first, static_cast<std::size_t>(first) + length);
                                     }
                                 }
                             });
        Sets sets(count);
        for (const auto& found : pairs)
        {
            for (const auto& pair : found)
                sets.join(pair.first, pair.second);
        }
        // The groups of two or more, numbered by their smallest node ID (the list is in order of ID).
        std::vector<std::uint32_t> sizes(count, 0);
        for (std::size_t index = 0; index < count; ++index)
            ++sizes[sets.find(static_cast<std::uint32_t>(index))];
        std::vector<std::int64_t> groupOfRoot(count, 0);
        std::int64_t groups = 0;
        std::vector<std::uint32_t> first(count, 0);
        for (std::size_t index = 0; index < count; ++index)
        {
            const std::uint32_t root = sets.find(static_cast<std::uint32_t>(index));
            if (sizes[root] < 2 || groupOfRoot[root] != 0)
                continue;
            groupOfRoot[root] = ++groups;
            first[root] = static_cast<std::uint32_t>(index);
        }
        if (groups == 0)
            return emptyTable(info().defaultColumns());
        std::vector<std::uint32_t> members;
        for (std::size_t index = 0; index < count; ++index)
        {
            if (sizes[sets.find(static_cast<std::uint32_t>(index))] >= 2)
                members.push_back(static_cast<std::uint32_t>(index));
        }
        std::stable_sort(members.begin(), members.end(), [&](std::uint32_t left, std::uint32_t right)
                         { return groupOfRoot[sets.find(left)] < groupOfRoot[sets.find(right)]; });
        const std::vector<std::int32_t> cp = asInt32(columns.get("positionFrameId"));
        const std::vector<std::int32_t> cd = asInt32(columns.get("displacementFrameId"));
        std::vector<std::int64_t> groupId;
        std::vector<std::int32_t> nid;
        std::array<std::vector<double>, 3> position;
        std::vector<double> distance;
        std::vector<std::int32_t> cps;
        std::vector<std::int32_t> cds;
        for (const std::uint32_t member : members)
        {
            const std::uint32_t root = sets.find(member);
            const std::uint32_t head = first[root];
            groupId.push_back(groupOfRoot[root]);
            nid.push_back(list.ids[member]);
            position[0].push_back(x[member]);
            position[1].push_back(y[member]);
            position[2].push_back(z[member]);
            distance.push_back(std::sqrt((x[member] - x[head]) * (x[member] - x[head]) +
                                         (y[member] - y[head]) * (y[member] - y[head]) +
                                         (z[member] - z[head]) * (z[member] - z[head])));
            cps.push_back(cp[member]);
            cds.push_back(cd[member]);
        }
        TableBuilder builder;
        builder.int64("groupId", std::move(groupId));
        builder.int32("nodeId", std::move(nid));
        builder.real("x", std::move(position[0]));
        builder.real("y", std::move(position[1]));
        builder.real("z", std::move(position[2]));
        builder.real("distance", std::move(distance));
        builder.int32("positionFrameId", std::move(cps));
        builder.int32("displacementFrameId", std::move(cds));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "coincidentNodes";
        info.family = "T5";
        info.description =
            "The grid points closer than `tolerance` (required: the model has no units) in the basic system. Points "
            "are in "
            "one group when a chain of points each within the tolerance of the next joins them. Groups of two or more "
            "only, "
            "numbered from 1 by their smallest node ID; distance is to the first node (the smallest ID) of the group.";
        info.parameters = {parameter("tolerance", ParameterKind::Real, "(required)",
                                     "the distance below which two grid points coincide"),
                           parameter("nodeIds", ParameterKind::IdList, "None", "look only at these grid points")};
        info.defaultSort = "groupId, nodeId";
        ColumnListBuilder columns;
        columns.int64("groupId")
            .int32("nodeId")
            .reals("x y z distance")
            .int32("positionFrameId")
            .int32("displacementFrameId");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addCoincidentNodesQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<CoincidentNodesQuery>());
}

} // namespace oor::query
