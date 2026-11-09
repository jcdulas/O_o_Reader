#include "oor/analysis/connections/ContactSearch.hpp"

#include "oor/analysis/components/ComponentCache.hpp"
#include "oor/analysis/connections/SolverContactSelection.hpp"
#include "oor/base/threading/ParallelFor.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace oor::analysis
{

namespace
{

struct Box
{
    base::Vec3 low{std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(),
                   std::numeric_limits<double>::infinity()};
    base::Vec3 high{-std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(),
                    -std::numeric_limits<double>::infinity()};
};

Box boxOf(const ContactSurface& surface)
{
    Box box;
    const auto extend = [&](const base::Vec3& point)
    {
        for (std::size_t axis = 0; axis < 3; ++axis)
        {
            box.low[axis] = std::min(box.low[axis], point[axis]);
            box.high[axis] = std::max(box.high[axis], point[axis]);
        }
    };
    for (const base::Vec3& point : surface.points())
        extend(point);
    for (const ContactFace& face : surface.faces())
    {
        for (std::size_t corner = 0; corner < face.cornerCount; ++corner)
            extend(face.corners[corner]);
    }
    return box;
}

bool boxesMeet(const Box& first, const Box& second, double gap) noexcept
{
    for (std::size_t axis = 0; axis < 3; ++axis)
    {
        if (first.low[axis] > second.high[axis] + gap || second.low[axis] > first.high[axis] + gap)
            return false;
    }
    return true;
}

// The largest distance by which the shells of a surface reach out of its grid points: what the boxes of two surfaces
// must be widened by for their skins to be compared.
double reachOf(const ContactSurface& surface) noexcept
{
    double reach = 0.0;
    for (const ContactFace& face : surface.faces())
        reach = std::max(reach, std::abs(face.offset) + face.halfThickness);
    return reach;
}

// The components' rule for one pair (ContactSearch).
ContactPairResult searchForComponents(const ContactPair& pair, const ContactSurfaces& surfaces,
                                      std::optional<double> tolerance)
{
    ContactPairResult entry;
    const ContactSurface& secondary = surfaces.surfaceOf(*pair.secondary);
    const ContactSurface& primary = surfaces.surfaceOf(*pair.primary);
    if (pair.tolerance > 0.0)
        entry.tolerance = pair.tolerance;
    else if (tolerance)
        entry.tolerance = *tolerance;
    else
    {
        const double edge = secondary.faces().empty() ? primary.medianEdgeLength() : secondary.medianEdgeLength();
        entry.tolerance = ContactSearch::DEFAULT_TOLERANCE_FRACTION * edge;
    }
    if (!boxesMeet(boxOf(secondary), boxOf(primary), entry.tolerance + reachOf(secondary) + reachOf(primary)))
        return entry;
    const ContactZone zone{entry.tolerance, entry.tolerance, true};
    const ShellSkins secondarySkins = ShellSkins::fromContactOption(pair.parameters.secondaryContactOption);
    const ShellSkins primarySkins = ShellSkins::fromContactOption(pair.parameters.primaryContactOption);
    std::vector<ContactNodePair> nodePairs =
        ContactSurfaces::nodePairs(secondary, primary, zone, secondarySkins, primarySkins);
    const ContactSurface* hostSurface = &primary;
    const ContactBody* nodeBody = pair.secondary;
    const ContactBody* hostBody = pair.primary;
    SearchSide side = SearchSide::Secondary;
    if (nodePairs.empty() && pair.searchesBothWays)
    {
        nodePairs = ContactSurfaces::nodePairs(primary, secondary, zone, primarySkins, secondarySkins);
        entry.isReversed = !nodePairs.empty();
        hostSurface = &secondary;
        std::swap(nodeBody, hostBody);
        side = SearchSide::Primary;
    }
    entry.meets = !nodePairs.empty();
    for (const ContactNodePair& nodePair : nodePairs)
    {
        ContactMeeting meeting;
        meeting.nid = nodePair.nid;
        meeting.secondary = nodeBody;
        meeting.primary = hostBody;
        meeting.face = &hostSurface->faces()[nodePair.face];
        meeting.point = nodePair.point;
        meeting.gap = nodePair.gap;
        meeting.distance = nodePair.distance;
        meeting.coordinates = faceCoordinatesOf(*meeting.face, meeting.point);
        meeting.searchedFrom = side;
        entry.meetings.push_back(meeting);
        entry.minDistance =
            entry.meetings.size() == 1 ? meeting.distance : std::min(entry.minDistance, meeting.distance);
        entry.maxDistance =
            entry.meetings.size() == 1 ? meeting.distance : std::max(entry.maxDistance, meeting.distance);
    }
    return entry;
}

} // namespace

std::string_view searchSideName(SearchSide side) noexcept
{
    return side == SearchSide::Primary ? "primary" : "secondary";
}

std::unique_ptr<const ContactSearch> ContactSearch::compute(const model::Model& model, std::optional<double> tolerance,
                                                            ContactRule rule, base::ThreadPool* pool)
{
    std::unique_ptr<ContactSearch> result(new ContactSearch());
    result->m_rule = rule;
    result->m_bodies = ContactBodies::read(model);
    // The pairs point into m_bodies, which stays where it is from here on.
    const ContactPairs pairs = ContactPairs::read(model, result->m_bodies);
    result->m_fromTables = pairs.fromTables();
    if (pairs.pairs().empty())
        return result;

    std::vector<const ContactBody*> used;
    for (const ContactPair& pair : pairs.pairs())
    {
        used.push_back(pair.secondary);
        used.push_back(pair.primary);
    }
    result->m_surfaces = std::make_unique<ContactSurfaces>(ContactSurfaces::build(model, used, pool));
    const ContactSurfaces& surfaces = *result->m_surfaces;

    const std::vector<ContactPair>& candidates = pairs.pairs();
    std::vector<ContactPairResult> found;
    if (rule == ContactRule::Solver)
    {
        // The pairs depend on one another (a grid point is tied once): one after the other.
        found = SolverContactSelection::select(model, candidates, surfaces, pairs.errorPerPair(),
                                               pairs.thicknessOffInOrder(), pool);
    }
    else
    {
        found.resize(candidates.size());
        const auto work = [&](std::size_t index)
        { found[index] = searchForComponents(candidates[index], surfaces, tolerance); };
        if (pool != nullptr && candidates.size() > 1)
            base::parallelFor(*pool, candidates.size(), work);
        else
        {
            for (std::size_t index = 0; index < candidates.size(); ++index)
                work(index);
        }
    }

    for (std::size_t index = 0; index < candidates.size(); ++index)
    {
        // Without tables every two bodies are candidates: only those that meet are pairs.
        if (!result->m_fromTables && !found[index].meets)
            continue;
        result->m_pairs.push_back(candidates[index]);
        result->m_results.push_back(std::move(found[index]));
    }
    return result;
}

std::shared_ptr<const ContactSearch> ContactSearch::of(const model::Model& model, std::optional<double> tolerance,
                                                       ContactRule rule, base::ThreadPool* pool)
{
    return ComponentCache::of(model).contacts(model, tolerance, rule, pool);
}

} // namespace oor::analysis
