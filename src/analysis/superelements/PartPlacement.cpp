#include "oor/analysis/superelements/PartPlacement.hpp"

#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/errors/OorError.hpp"
#include "oor/model/cards/nodes/geomPoint/GeomPointTable.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/superelements/superelementLocation/SuperelementLocationTable.hpp"
#include "oor/model/cards/superelements/superelementMirrorPlane/SuperelementMirrorPlaneTable.hpp"

#include <algorithm>
#include <array>
#include <mutex>

namespace oor::analysis
{

namespace
{

// Below this length a direction is taken as no direction: the three points of a SELOC or SEMPLN are on a line.
constexpr double DEGENERATE_LENGTH = 1.0e-12;

// An orthonormal frame by three points: the origin at the first, x towards the second, z normal to the plane of the
// three (PA1 on PB1, PA1-PA2 along PB1-PB2, the planes of the three points together).
struct Frame
{
    base::Vec3 origin;
    std::array<base::Vec3, 3> axes;
};

std::optional<Frame> frameOf(const std::array<base::Vec3, 3>& points)
{
    const base::Vec3 along = points[1] - points[0];
    const base::Vec3 normal = base::cross(along, points[2] - points[0]);
    if (base::norm(along) < DEGENERATE_LENGTH || base::norm(normal) < DEGENERATE_LENGTH)
        return std::nullopt;
    const base::Vec3 e1 = base::normalized(along);
    const base::Vec3 e3 = base::normalized(normal);
    return Frame{points[0], {e1, base::cross(e3, e1), e3}};
}

std::string describe(const SuperelementEntry& entry)
{
    return "superelement " + std::to_string(entry.seid);
}

} // namespace

const base::Vec3* PlacedPoints::find(base::EntityId nid) const noexcept
{
    const auto found = std::lower_bound(nids.begin(), nids.end(), nid);
    if (found == nids.end() || *found != nid)
        return nullptr;
    return &positions[static_cast<std::size_t>(found - nids.begin())];
}

PlacedPoints PartPlacement::gridPoints(const model::Model& model)
{
    PlacedPoints points;
    const model::NodeTable* grids = model.findTable<model::NodeTable>();
    if (grids == nullptr)
        return points;
    const BasicPositionCache& cache = BasicPositionCache::of(model);
    std::vector<std::uint32_t> rows;
    for (std::uint32_t row = 0; row < grids->rows().rowCount(); ++row)
    {
        if (grids->rows().isAlive(row))
            rows.push_back(row);
    }
    std::sort(rows.begin(), rows.end(), [grids](std::uint32_t left, std::uint32_t right)
              { return grids->idAtRow(left) < grids->idAtRow(right); });
    points.nids.reserve(rows.size());
    points.positions.reserve(rows.size());
    for (const std::uint32_t row : rows)
    {
        points.nids.push_back(grids->idAtRow(row));
        points.positions.push_back(cache.position(row));
    }
    return points;
}

std::optional<base::Vec3> PartPlacement::pointPosition(const model::Model& model, base::EntityId nid)
{
    const std::optional<model::CardLocation> found = model.locate(base::IdSpace::Node, nid);
    if (!found)
        return std::nullopt;
    if (found->table == model::typeIndexOf<model::NodeTable>())
        return BasicPositionCache::of(model).position(found->row);
    if (found->table != model::typeIndexOf<model::GeomPointTable>())
        return std::nullopt;
    const model::GeomPointCard point = model.findTable<model::GeomPointTable>()->card(found->row);
    if (point.positionFrameId == 0)
        return point.xyz;
    BasicPositionCache& cache = BasicPositionCache::of(model);
    const std::lock_guard lock(cache.mutex());
    return cache.transform().toBasic(point.positionFrameId, point.xyz);
}

PlacedPoints PartPlacement::place(const SuperelementCatalogue& catalogue, const SuperelementEntry& entry,
                                  std::vector<std::string>& problems)
{
    const model::Model& residual = catalogue.residual();
    const model::Model& source = *entry.idModel;
    PlacedPoints points;
    try
    {
        points = gridPoints(source);
    }
    catch (const base::OorError& error)
    {
        problems.push_back(describe(entry) + ": the positions of its grid points cannot be computed: " + error.what());
        return points;
    }
    std::optional<model::SuperelementMirrorPlaneCard> mirror;
    forEachCard<model::SuperelementMirrorPlaneTable>(
        residual,
        [&](const model::SuperelementMirrorPlaneCard& card, model::CardLocation)
        {
            if (card.superelementId == entry.seid)
                mirror = card;
        });
    std::optional<model::SuperelementLocationCard> move;
    forEachCard<model::SuperelementLocationTable>(residual,
                                                  [&](const model::SuperelementLocationCard& card, model::CardLocation)
                                                  {
                                                      if (card.superelementId == entry.seid)
                                                          move = card;
                                                  });

    const std::string mirrorCard = std::string("SuperelementMirrorPlane");
    const std::string locationCard = std::string("SuperelementLocation");
    // The mirror of a point through the plane of the SEMPLN; the identity when there is none or it cannot be used.
    std::optional<std::pair<base::Vec3, base::Vec3>> plane; // a point of the plane and its unit normal
    if (mirror)
    {
        std::array<base::Vec3, 3> corners{};
        bool complete = true;
        for (std::size_t index = 0; index < 3; ++index)
        {
            const std::optional<base::Vec3> at = pointPosition(residual, mirror->points[index]);
            complete = complete && at.has_value();
            corners[index] = at.value_or(base::Vec3{});
        }
        const base::Vec3 normal = base::cross(corners[1] - corners[0], corners[2] - corners[0]);
        if (!complete)
            problems.push_back(describe(entry) + ": a point of its " + mirrorCard + " is not a " + std::string("Node") +
                               " or " + std::string("GeomPoint") + " of the main Bulk Data");
        else if (base::norm(normal) < DEGENERATE_LENGTH)
            problems.push_back(describe(entry) + ": the three points of its " + mirrorCard + " are on a line");
        else
            plane = std::pair{corners[0], base::normalized(normal)};
    }
    const auto reflect = [&plane](const base::Vec3& point)
    {
        if (!plane)
            return point;
        return point - 2.0 * base::dot(point - plane->first, plane->second) * plane->second;
    };
    for (base::Vec3& position : points.positions)
        position = reflect(position);

    if (move)
    {
        std::array<base::Vec3, 3> fromPoints{};
        std::array<base::Vec3, 3> toPoints{};
        bool complete = true;
        for (std::size_t index = 0; index < 3; ++index)
        {
            const std::optional<base::Vec3> from = pointPosition(source, move->pointsA[index]);
            const std::optional<base::Vec3> to = pointPosition(residual, move->pointsB[index]);
            complete = complete && from.has_value() && to.has_value();
            fromPoints[index] = reflect(from.value_or(base::Vec3{}));
            toPoints[index] = to.value_or(base::Vec3{});
        }
        const std::optional<Frame> from = complete ? frameOf(fromPoints) : std::nullopt;
        const std::optional<Frame> to = complete ? frameOf(toPoints) : std::nullopt;
        if (!complete)
        {
            const auto fields = [&](std::string_view first, std::string_view last)
            { return std::string(first) + "-" + std::string(last); };
            problems.push_back(describe(entry) + ": a point of its " + locationCard + " does not exist (" +
                               fields("pointA1", "pointA3") + " in the superelement, " + fields("pointB1", "pointB3") +
                               " in the main Bulk Data)");
        }
        else if (!from || !to)
            problems.push_back(describe(entry) + ": three points of its " + locationCard + " are on a line");
        else
        {
            for (base::Vec3& position : points.positions)
            {
                const base::Vec3 offset = position - from->origin;
                position = to->origin;
                for (std::size_t axis = 0; axis < 3; ++axis)
                    position += base::dot(offset, from->axes[axis]) * to->axes[axis];
            }
        }
    }
    return points;
}

} // namespace oor::analysis
