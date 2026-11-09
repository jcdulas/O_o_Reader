#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/geometry/GeometryChecks.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace oor::analysis
{

namespace
{

// A grid point at (within the tolerance of) the position of another grid point with a lower ID, in the basic system.
// Opt-in: a deck may have coincident points on purpose (the two sides of a connection), and the check has a price.
//
// The points are put in a grid of cubes of side 4 x tolerance, shifted from the origin by 0.382 of a side. A point and
// a neighbour closer than the tolerance are in the same cube, or in cubes that touch through faces the point is within
// the tolerance of: on each axis one face at most (the cube is wider than twice the tolerance), and none for a point
// farther than that from both (half the points on each axis, for random coordinates; every point of a mesh on round
// coordinates, which the shift puts inside its cubes). So a point looks at its own cube and at no more than seven
// others, none on most meshes. The cubes are keys (packed cell coordinates) in a sorted array: the cube of a point is
// the run of the array it is in; another is found through a table of open addressing, after a filter of bits, small
// enough for the caches, that tells most of the empty ones without a look at the table, whose reads miss the caches
// from a few hundred thousand points. A million points take a fraction of a second. (With cubes of twice the tolerance,
// a point was always within the tolerance of a face on each axis and looked at seven other cubes: 190 ns a point at a
// million.)
class CoincidentNodesCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "geometry.coincident", base::Severity::Warning, "geometry",
        "A grid point coincides with a grid point of a lower ID (within the tolerance of the "
        "validation).",
        true};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const model::NodeTable* grids = model.findTable<model::NodeTable>();
        if (grids == nullptr || grids->rows().aliveCount() < 2)
            return;
        const double tolerance = std::max(context.options().coincidentTolerance, std::numeric_limits<double>::min());
        const BasicPositionCache* cache = nullptr;
        try
        {
            cache = &BasicPositionCache::of(model);
        }
        catch (const base::OorError&)
        {
            return; // the coordinate checks say why the positions cannot be computed
        }
        const std::span<const std::array<double, 3>> positions = cache->positions();

        struct Point
        {
            std::uint64_t cell;
            std::uint32_t row;
        };
        const double side = 4.0 * tolerance;
        const double shift = 0.381966 * side; // no fraction of a round number of sides
        // The cube of a coordinate on its axis, and how far into it the coordinate is.
        const auto cubeOf = [&](double value) { return static_cast<std::int64_t>(std::floor((value + shift) / side)); };
        std::vector<Point> points;
        points.reserve(grids->rows().aliveCount());
        for (std::uint32_t row = 0; row < grids->rows().rowCount(); ++row)
        {
            if (grids->rows().isAlive(row))
            {
                const std::array<double, 3>& xyz = positions[row];
                points.push_back(Point{pack(cubeOf(xyz[0]), cubeOf(xyz[1]), cubeOf(xyz[2])), row});
            }
        }
        std::ranges::sort(points, [](const Point& left, const Point& right)
                          { return left.cell != right.cell ? left.cell < right.cell : left.row < right.row; });

        // The table of cubes: the key, and where its points start in `points` and how many there are. Two slots a
        // point, and a slot found from the hash by a multiplication, not a mask: a table whose size doubled at each
        // power of two (16 MB from 262 144 points to 524 287) made the cost of a point step there.
        const std::size_t capacity = std::max<std::size_t>(16, 2 * points.size());
        // A position in [0, size) from 32 bits of a hash (size below 2^32: the rows are 32-bit).
        const auto reduce = [](std::uint64_t bits, std::size_t size)
        { return static_cast<std::size_t>(((bits & 0xffffffffULL) * size) >> 32U); };
        struct Slot
        {
            std::uint64_t cell = 0;
            std::uint32_t begin = 0;
            std::uint32_t count = 0; // 0: empty
        };
        std::vector<Slot> table(capacity);
        const auto home = [&](std::uint64_t key) { return reduce(mix(key) >> 32U, capacity); };
        const auto next = [&](std::size_t slot) { return slot + 1 == capacity ? 0 : slot + 1; };
        // The filter: a bit set for each cube that has points, at a hash of its key (the other 32 bits of the hash
        // than the slot's). Eight bits a point (a megabyte for a million): about one cube in eight that has no point
        // finds its bit set by another.
        const std::size_t filterBits = 64 * ((8 * points.size() + 63) / 64);
        std::vector<std::uint64_t> filter(filterBits / 64, 0);
        const auto filterBit = [&](std::uint64_t key) { return reduce(mix(key), filterBits); };
        for (std::size_t begin = 0; begin < points.size();)
        {
            std::size_t end = begin;
            while (end < points.size() && points[end].cell == points[begin].cell)
                ++end;
            const std::size_t bit = filterBit(points[begin].cell);
            filter[bit >> 6U] |= std::uint64_t{1} << (bit & 63U);
            std::size_t slot = home(points[begin].cell);
            while (table[slot].count != 0)
                slot = next(slot);
            table[slot] =
                Slot{points[begin].cell, static_cast<std::uint32_t>(begin), static_cast<std::uint32_t>(end - begin)};
            begin = end;
        }

        const double squared = tolerance * tolerance;
        for (std::size_t begin = 0; begin < points.size();)
        {
            std::size_t end = begin;
            while (end < points.size() && points[end].cell == points[begin].cell)
                ++end;
            for (std::size_t at = begin; at < end; ++at)
            {
                const Point& point = points[at];
                const std::array<double, 3>& p = positions[point.row];
                // The neighbouring cube on each axis where the point is within the tolerance of a face of its cube:
                // below or above. `near` has a bit for each such axis.
                std::array<std::int64_t, 3> own{};
                std::array<std::int64_t, 3> other{};
                unsigned near = 0;
                for (std::size_t axis = 0; axis < 3; ++axis)
                {
                    own[axis] = cubeOf(p[axis]);
                    const double local = p[axis] + shift - static_cast<double>(own[axis]) * side;
                    other[axis] = own[axis];
                    if (local <= tolerance)
                        other[axis] = own[axis] - 1;
                    else if (side - local <= tolerance)
                        other[axis] = own[axis] + 1;
                    if (other[axis] != own[axis])
                        near |= 1U << axis;
                }
                base::EntityId best = std::numeric_limits<base::EntityId>::max();
                const base::EntityId mine = grids->idAtRow(point.row);
                // The points of `points` from `first` to `last`: a lower ID than this point's and the best yet, within
                // the tolerance.
                const auto look = [&](std::size_t first, std::size_t last)
                {
                    for (std::size_t index = first; index < last; ++index)
                    {
                        const std::uint32_t row = points[index].row;
                        if (row == point.row)
                            continue;
                        const base::EntityId id = grids->idAtRow(row);
                        if (id >= mine || id >= best)
                            continue;
                        const std::array<double, 3>& q = positions[row];
                        const double dx = p[0] - q[0];
                        const double dy = p[1] - q[1];
                        const double dz = p[2] - q[2];
                        if (dx * dx + dy * dy + dz * dz <= squared)
                            best = id;
                    }
                };
                look(begin, end); // its own cube: the run it is in
                for (unsigned choice = 1; choice < 8; ++choice)
                {
                    if ((choice & ~near) != 0)
                        continue; // a neighbour on an axis where the point is far from both faces
                    const std::uint64_t key =
                        pack((choice & 1U) != 0 ? other[0] : own[0], (choice & 2U) != 0 ? other[1] : own[1],
                             (choice & 4U) != 0 ? other[2] : own[2]);
                    const std::size_t bit = filterBit(key);
                    if ((filter[bit >> 6U] >> (bit & 63U) & 1U) == 0)
                        continue; // no point in that cube
                    std::size_t slot = home(key);
                    while (table[slot].count != 0 && table[slot].cell != key)
                        slot = next(slot);
                    if (table[slot].count != 0)
                        look(table[slot].begin, table[slot].begin + table[slot].count);
                }
                if (best != std::numeric_limits<base::EntityId>::max())
                    sink.reportCard(model::CardLocation{model::typeIndexOf<model::NodeTable>(), point.row}, "xyz",
                                    "coincides with grid point " + std::to_string(best),
                                    "merge the two grid points (equivalence)");
            }
            begin = end;
        }
    }

private:
    // The cell coordinates of a point packed in 63 bits (21 each): a collision of far cubes only adds candidates, which
    // the distance test then refuses.
    static std::uint64_t pack(std::int64_t x, std::int64_t y, std::int64_t z) noexcept
    {
        constexpr std::uint64_t MASK = (1ULL << 21U) - 1U;
        return (static_cast<std::uint64_t>(x) & MASK) | ((static_cast<std::uint64_t>(y) & MASK) << 21U) |
               ((static_cast<std::uint64_t>(z) & MASK) << 42U);
    }
    static std::size_t mix(std::uint64_t key) noexcept
    {
        key ^= key >> 33U;
        key *= 0xff51afd7ed558ccdULL;
        key ^= key >> 33U;
        return static_cast<std::size_t>(key);
    }
};

} // namespace

void addCoincidentNodesCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<CoincidentNodesCheck>());
}

} // namespace oor::analysis
