#include "oor/analysis/geometry/PointLocator.hpp"

#include "../connections/fasteners/SurfaceProjection.hpp"
#include "SolidShape.hpp"
#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/base/threading/ParallelFor.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/core/CardRegistry.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace oor::analysis
{

using base::Vec3;

namespace
{

// A projection inside a face, or natural coordinates inside a solid, to rounding.
constexpr double INSIDE_TOLERANCE = 1e-9;
// The fraction of the median element size that is the default distance.
constexpr double DEFAULT_TOLERANCE_FRACTION = 0.05;
// The most cells one element may cover: the cell grows with the largest element so that it does not fill the index.
constexpr double ELEMENT_SPAN_IN_CELLS = 16.0;
constexpr std::size_t CHUNK_POINTS = 4096;

// One element that can hold a point, ready to be asked: a shell as a shell element of the fastener piercing search, a
// solid with its grid point positions.
struct Candidate
{
    model::CardLocation location;
    base::EntityId eid = 0;
    bool isShell = false;
    detail::ShellElement shell;
    detail::SolidElement solid;
    std::array<base::EntityId, detail::MAX_SOLID_NODES> slotIds{};
    Vec3 low;
    Vec3 high;
};

// What a point found in one candidate.
struct Hit
{
    double distance = 0.0;
    Vec3 natural;
    std::vector<base::EntityId> nodes;
    std::vector<double> weights;
};

std::optional<detail::SolidShapeKind> solidKind(ElementShape shape) noexcept
{
    switch (shape)
    {
    case ElementShape::Tetrahedron:
        return detail::SolidShapeKind::Tetrahedron;
    case ElementShape::Pentahedron:
        return detail::SolidShapeKind::Pentahedron;
    case ElementShape::Hexahedron:
        return detail::SolidShapeKind::Hexahedron;
    case ElementShape::Pyramid:
        return detail::SolidShapeKind::Pyramid;
    default:
        return std::nullopt;
    }
}

class Locator
{
public:
    Locator(const model::Model& model, const std::optional<std::vector<model::CardLocation>>& elements,
            base::ThreadPool* pool)
        : m_model(model)
    {
        try
        {
            m_positions = BasicPositionCache::of(model, pool).positions();
        }
        catch (const base::OorError&)
        {
            m_positions = {}; // a broken coordinate system: no grid point is placed, so no element is either
        }
        if (elements)
        {
            for (const model::CardLocation location : *elements)
                add(location);
        }
        else
        {
            const model::CardRegistry& registry = model::CardRegistry::instance();
            for (std::size_t type = 1; type < registry.size(); ++type)
            {
                const auto typeIndex = static_cast<std::uint16_t>(type);
                const int dimension = dimensionOf(ElementTopology::shapeOf(typeIndex));
                const model::CardTable* table = model.findTable(typeIndex);
                if (table == nullptr || (dimension != 2 && dimension != 3))
                    continue;
                for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
                {
                    if (table->rows().isAlive(row))
                        add(model::CardLocation{typeIndex, row});
                }
            }
        }
    }

    [[nodiscard]] double medianSize() const
    {
        if (m_candidates.empty())
            return 0.0;
        std::vector<double> sizes;
        sizes.reserve(m_candidates.size());
        for (const Candidate& candidate : m_candidates)
            sizes.push_back(sizeOf(candidate));
        const auto middle = sizes.begin() + static_cast<std::ptrdiff_t>(sizes.size() / 2);
        std::nth_element(sizes.begin(), middle, sizes.end());
        return *middle;
    }

    // The cells of the index: each element in every cell its box, widened by the tolerance, touches.
    void index(double tolerance)
    {
        m_tolerance = tolerance;
        if (m_candidates.empty())
            return;
        double largest = 0.0;
        m_origin = m_candidates.front().low;
        for (const Candidate& candidate : m_candidates)
        {
            largest = std::max(largest, sizeOf(candidate));
            for (std::size_t axis = 0; axis < 3; ++axis)
                m_origin[axis] = std::min(m_origin[axis], candidate.low[axis] - tolerance);
        }
        m_cell = std::max({2.0 * medianSize(), largest / ELEMENT_SPAN_IN_CELLS, 2.0 * tolerance});
        if (!(m_cell > 0.0))
            m_cell = 1.0;
        for (std::uint32_t index = 0; index < m_candidates.size(); ++index)
        {
            const Candidate& candidate = m_candidates[index];
            std::array<std::int64_t, 3> first{};
            std::array<std::int64_t, 3> last{};
            for (std::size_t axis = 0; axis < 3; ++axis)
            {
                first[axis] = cellOf(candidate.low[axis] - tolerance, axis);
                last[axis] = cellOf(candidate.high[axis] + tolerance, axis);
            }
            for (std::int64_t x = first[0]; x <= last[0]; ++x)
                for (std::int64_t y = first[1]; y <= last[1]; ++y)
                    for (std::int64_t z = first[2]; z <= last[2]; ++z)
                        m_cells.push_back(Cell{x, y, z, index});
        }
        std::sort(m_cells.begin(), m_cells.end());
    }

    [[nodiscard]] LocatedPoint locate(const Vec3& point) const
    {
        LocatedPoint result;
        result.natural = {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(),
                          std::numeric_limits<double>::quiet_NaN()};
        if (m_candidates.empty() || !std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z))
            return result;
        const Cell key{cellOf(point.x, 0), cellOf(point.y, 1), cellOf(point.z, 2), 0};
        const Candidate* best = nullptr;
        Hit bestHit;
        for (auto at = std::lower_bound(m_cells.begin(), m_cells.end(), key);
             at != m_cells.end() && at->x == key.x && at->y == key.y && at->z == key.z; ++at)
        {
            const Candidate& candidate = m_candidates[at->candidate];
            std::optional<Hit> hit = candidate.isShell ? onShell(candidate, point) : inSolid(candidate, point);
            if (!hit)
                continue;
            const double tie = 1e-12 * std::max(1.0, bestHit.distance);
            const bool nearer = best == nullptr || hit->distance < bestHit.distance - tie ||
                                (hit->distance <= bestHit.distance + tie && candidate.eid < best->eid);
            if (nearer)
            {
                best = &candidate;
                bestHit = std::move(*hit);
            }
        }
        if (best == nullptr)
            return result;
        result.element = best->location;
        result.eid = best->eid;
        result.natural = bestHit.natural;
        result.distance = bestHit.distance;
        result.nodes = std::move(bestHit.nodes);
        result.weights = std::move(bestHit.weights);
        return result;
    }

private:
    struct Cell
    {
        std::int64_t x = 0;
        std::int64_t y = 0;
        std::int64_t z = 0;
        std::uint32_t candidate = 0;

        [[nodiscard]] bool operator<(const Cell& other) const noexcept
        {
            if (x != other.x)
                return x < other.x;
            if (y != other.y)
                return y < other.y;
            if (z != other.z)
                return z < other.z;
            return candidate < other.candidate;
        }
    };

    [[nodiscard]] std::optional<Vec3> positionOf(base::EntityId nid) const
    {
        if (nid == 0)
            return std::nullopt;
        const auto location = m_model.locate(base::IdSpace::Node, nid);
        if (!location || location->table != model::typeIndexOf<model::NodeTable>() ||
            location->row >= m_positions.size())
            return std::nullopt;
        const std::array<double, 3>& value = m_positions[location->row];
        return Vec3{value[0], value[1], value[2]};
    }

    static double sizeOf(const Candidate& candidate) noexcept
    {
        return std::max({candidate.high.x - candidate.low.x, candidate.high.y - candidate.low.y,
                         candidate.high.z - candidate.low.z});
    }

    [[nodiscard]] std::int64_t cellOf(double value, std::size_t axis) const noexcept
    {
        return static_cast<std::int64_t>(std::floor((value - m_origin[axis]) / m_cell));
    }

    void add(model::CardLocation location)
    {
        const model::CardTable* table = m_model.findTable(location.table);
        if (table == nullptr || !table->rows().isAlive(location.row))
            return;
        const ElementShape shape = ElementTopology::shapeOf(location.table);
        Candidate candidate;
        candidate.location = location;
        candidate.eid = table->idAt(location.row);
        bool placed = false;
        if (dimensionOf(shape) == 2)
            placed = makeShell(*table, location, candidate);
        else if (const auto kind = solidKind(shape))
            placed = makeSolid(*table, location, *kind, candidate);
        if (placed)
            m_candidates.push_back(std::move(candidate));
    }

    // A shell as the shell element of its face: its corners and the mid-side points it has.
    bool makeShell(const model::CardTable& table, model::CardLocation location, Candidate& candidate) const
    {
        std::vector<std::array<base::EntityId, 4>> faces;
        std::vector<std::array<base::EntityId, 4>> midSides;
        ElementTopology::orientedFacesOf(table, location.table, location.row, faces);
        ElementTopology::orientedFaceMidSidesOf(table, location.table, location.row, midSides);
        if (faces.empty())
            return false;
        const std::array<base::EntityId, 4>& corners = faces.front();
        const std::size_t cornerCount = corners[3] == 0 ? 3 : 4;
        bool quadratic = false;
        for (std::size_t edge = 0; edge < cornerCount && !midSides.empty(); ++edge)
            quadratic = quadratic || midSides.front()[edge] != 0;
        detail::ShellElement& shell = candidate.shell;
        shell.isVirtual = true;
        shell.kind = cornerCount == 3 ? (quadratic ? detail::ShellShapeKind::Tria6 : detail::ShellShapeKind::Tria3)
                                      : (quadratic ? detail::ShellShapeKind::Quad8 : detail::ShellShapeKind::Quad4);
        std::vector<Vec3> placed;
        for (std::size_t slot = 0; slot < shell.slotCount(); ++slot)
        {
            const base::EntityId nid = slot < cornerCount ? corners[slot] : midSides.front()[slot - cornerCount];
            const auto position = positionOf(nid);
            if (!position)
            {
                if (slot < cornerCount)
                    return false; // a corner with no position: the element has no face
                continue;
            }
            shell.nodes[slot] = nid;
            shell.points[slot] = *position;
            shell.presentNodes = static_cast<std::uint8_t>(shell.presentNodes | (1U << slot));
            placed.push_back(*position);
        }
        candidate.isShell = true;
        boxOf(placed, candidate);
        return true;
    }

    // A solid with the positions of its corners and of the mid-side points it has.
    bool makeSolid(const model::CardTable& table, model::CardLocation location, detail::SolidShapeKind kind,
                   Candidate& candidate) const
    {
        std::vector<base::EntityId> slots;
        ElementTopology::slotsOf(table, location.table, location.row, slots);
        detail::SolidElement& solid = candidate.solid;
        solid.kind = kind;
        std::vector<Vec3> placed;
        for (std::size_t slot = 0; slot < solid.slotCount(); ++slot)
        {
            const base::EntityId nid = slot < slots.size() ? slots[slot] : 0;
            const auto position = positionOf(nid);
            if (!position)
            {
                if (slot < solid.cornerCount())
                    return false;
                continue;
            }
            candidate.slotIds[slot] = nid;
            solid.points[slot] = *position;
            solid.present |= 1U << slot;
            placed.push_back(*position);
        }
        boxOf(placed, candidate);
        return true;
    }

    static void boxOf(const std::vector<Vec3>& points, Candidate& candidate)
    {
        candidate.low = candidate.high = points.front();
        for (const Vec3& point : points)
        {
            for (std::size_t axis = 0; axis < 3; ++axis)
            {
                candidate.low[axis] = std::min(candidate.low[axis], point[axis]);
                candidate.high[axis] = std::max(candidate.high[axis], point[axis]);
            }
        }
    }

    [[nodiscard]] std::optional<Hit> onShell(const Candidate& candidate, const Vec3& point) const
    {
        const detail::SurfaceProjection projection = detail::SurfaceProjection::onto(candidate.shell, point);
        if (!projection.isInside(INSIDE_TOLERANCE) || projection.distance > m_tolerance)
            return std::nullopt;
        Hit hit;
        hit.distance = projection.distance;
        hit.natural = {projection.xi, projection.eta, std::numeric_limits<double>::quiet_NaN()};
        for (std::size_t slot = 0; slot < candidate.shell.slotCount(); ++slot)
        {
            if (!candidate.shell.hasNode(slot))
                continue;
            hit.nodes.push_back(candidate.shell.nodes[slot]);
            hit.weights.push_back(projection.shape.n[slot]);
        }
        return hit;
    }

    [[nodiscard]] std::optional<Hit> inSolid(const Candidate& candidate, const Vec3& point) const
    {
        const detail::SolidLocation found = detail::locateInSolid(candidate.solid, point);
        if (found.outside > INSIDE_TOLERANCE && found.distance > m_tolerance)
            return std::nullopt;
        Hit hit;
        hit.distance = found.outside > INSIDE_TOLERANCE ? found.distance : 0.0;
        hit.natural = found.natural;
        const std::array<double, detail::MAX_SOLID_NODES> weights =
            detail::solidWeights(candidate.solid, found.natural);
        for (std::size_t slot = 0; slot < candidate.solid.slotCount(); ++slot)
        {
            if (!candidate.solid.hasNode(slot))
                continue;
            hit.nodes.push_back(candidate.slotIds[slot]);
            hit.weights.push_back(weights[slot]);
        }
        return hit;
    }

    const model::Model& m_model;
    std::span<const std::array<double, 3>> m_positions;
    std::vector<Candidate> m_candidates;
    std::vector<Cell> m_cells; // sorted
    Vec3 m_origin;
    double m_cell = 1.0;
    double m_tolerance = 0.0;
};

} // namespace

std::vector<LocatedPoint> PointLocator::locate(const model::Model& model, std::span<const Vec3> points,
                                               const std::optional<std::vector<model::CardLocation>>& elements,
                                               std::optional<double> tolerance, base::ThreadPool* pool)
{
    if (tolerance && !(*tolerance >= 0.0))
        throw base::UsageError("locate_points: the tolerance is a distance, 0 or more");
    Locator locator(model, elements, pool);
    locator.index(tolerance.value_or(DEFAULT_TOLERANCE_FRACTION * locator.medianSize()));
    std::vector<LocatedPoint> located(points.size());
    const auto work = [&](std::size_t begin, std::size_t end)
    {
        for (std::size_t index = begin; index < end; ++index)
            located[index] = locator.locate(points[index]);
    };
    if (pool != nullptr && points.size() > CHUNK_POINTS)
    {
        const std::vector<base::IndexRange> chunks = base::splitIntoChunks(points.size(), CHUNK_POINTS);
        base::parallelFor(*pool, chunks.size(),
                          [&](std::size_t chunk) { work(chunks[chunk].begin, chunks[chunk].end); });
    }
    else
        work(0, points.size());
    return located;
}

} // namespace oor::analysis
