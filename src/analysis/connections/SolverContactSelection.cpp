#include "oor/analysis/connections/SolverContactSelection.hpp"

#include "oor/analysis/components/ComponentCache.hpp"
#include "oor/analysis/connections/RigidElementTerms.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace oor::analysis
{

namespace
{

// format reference BCPARA, ERROR: the smallest nonzero plate or solid dimension of the body divided by 20, and the
// thinnest shell of the body divided by 4.
constexpr double EDGE_DIVISOR = 20.0;
constexpr double THICKNESS_DIVISOR = 4.0;
// ISEARCH 2 of a BCTABLE: the lower body searches first while its characteristic length is at
// most 1.05 times the other's.
constexpr double ORDER_RATIO = 1.05;

// The grid points a rigid element or an MPC equation makes dependent: the dependent terms of the rigid elements
// (RigidElementTerms) and the first grid point of each MPC equation (format reference MPC: G1, C1 is the dependent
// degree of freedom). Sorted.
std::vector<base::EntityId> dependentGridPoints(const model::Model& model, base::ThreadPool* pool)
{
    const std::shared_ptr<const ModelEntities> entities = ComponentCache::of(model).entities(model, pool);
    std::vector<base::EntityId> dependent;
    for (std::size_t index = 0; index < entities->size(); ++index)
    {
        const ModelEntity& entity = entities->entity(index);
        if (entity.isEquation)
        {
            if (!entities->nodesOf(index).empty())
                dependent.push_back(entities->nodesOf(index).front());
        }
        else if (entity.role == ElementRole::Rigid)
        {
            for (const RigidTerm& term : RigidElementTerms::of(model, entity.card))
            {
                if (!term.isIndependent && term.node != 0)
                    dependent.push_back(term.node);
            }
        }
    }
    std::sort(dependent.begin(), dependent.end());
    dependent.erase(std::unique(dependent.begin(), dependent.end()), dependent.end());
    return dependent;
}

// The characteristic length of a body for the search order of ISEARCH 2.
double characteristicLength(const ContactSurface& surface, bool thicknessOff) noexcept
{
    const double edge = surface.smallestOuterEdgeLength() / EDGE_DIVISOR;
    return thicknessOff ? edge : std::min(edge, surface.thinnestShell() / THICKNESS_DIVISOR);
}

// The constraints of one pair while they are chosen: what is tied and what holds a tie.
class PairConstraints
{
public:
    explicit PairConstraints(std::set<base::EntityId>& dependentInModel) : m_dependentInModel(dependentInModel) {}

    // True when tying `node` to `face` keeps the constraints free of loops; then it is recorded. The face holds the
    // tie through all its grid points, the mid-side points of a quadratic face included.
    bool tryTie(base::EntityId node, const ContactFace& face)
    {
        m_facePoints.clear();
        face.appendGridPoints(m_facePoints);
        if (std::find(m_facePoints.begin(), m_facePoints.end(), node) != m_facePoints.end())
            return false; // a grid point of both bodies cannot be tied to itself
        if (m_dependentInModel.contains(node) || m_retained.contains(node))
            return false;
        for (const base::EntityId point : m_facePoints)
        {
            if (m_dependentInModel.contains(point))
                return false;
        }
        m_dependentInModel.insert(node);
        m_retained.insert(m_facePoints.begin(), m_facePoints.end());
        return true;
    }

private:
    std::set<base::EntityId>& m_dependentInModel;
    std::set<base::EntityId> m_retained;
    std::vector<base::EntityId> m_facePoints; // scratch
};

} // namespace

double SolverContactSelection::automaticError(const ContactSurface& surface) noexcept
{
    return std::min(surface.smallestEdgeLength() / EDGE_DIVISOR, surface.thinnestShell() / THICKNESS_DIVISOR);
}

std::vector<ContactPairResult> SolverContactSelection::select(const model::Model& model,
                                                              const std::vector<ContactPair>& pairs,
                                                              const ContactSurfaces& surfaces, bool errorPerPair,
                                                              bool thicknessOffInOrder, base::ThreadPool* pool)
{
    std::vector<ContactPairResult> results(pairs.size());
    if (pairs.empty())
        return results;
    // format reference BCPARA, ERROR: one value for every contact pair (ERRBAS 0, the default).
    double modelError = std::numeric_limits<double>::infinity();
    for (const ContactPair& pair : pairs)
    {
        modelError = std::min({modelError, automaticError(surfaces.surfaceOf(*pair.secondary)),
                               automaticError(surfaces.surfaceOf(*pair.primary))});
    }
    // A point already dependent (a rigid element, an MPC equation, an earlier pair) is never tied again.
    const std::vector<base::EntityId> excluded = dependentGridPoints(model, pool);
    std::set<base::EntityId> dependentInModel(excluded.begin(), excluded.end());

    for (std::size_t index = 0; index < pairs.size(); ++index)
    {
        const ContactPair& pair = pairs[index];
        ContactPairResult& result = results[index];
        const ContactSurface& secondary = surfaces.surfaceOf(*pair.secondary);
        const ContactSurface& primary = surfaces.surfaceOf(*pair.primary);
        double error = pair.parameters.errorTolerance.value.value_or(0.0);
        if (!(error > 0.0))
            error =
                pair.tableName == "ContactSet" && pair.tolerance > 0.0 ? pair.tolerance : 0.0; // MAXD of a legacy pair
        if (!(error > 0.0))
        {
            error = errorPerPair ? std::min(automaticError(secondary), automaticError(primary)) : modelError;
            if (!std::isfinite(error))
                error = 0.0;
        }
        const double bias = std::clamp(pair.parameters.toleranceBias.value.value_or(0.0), 0.0, 1.0);
        result.tolerance = error;
        // a grid point of shells meets the face of a solid by the point of its skin
        const ContactZone zone{(1.0 - bias) * error, (1.0 + bias) * error, false, true};
        const ShellSkins secondarySkins = ShellSkins::fromContactOption(pair.parameters.secondaryContactOption);
        const ShellSkins primarySkins = ShellSkins::fromContactOption(pair.parameters.primaryContactOption);

        // The order of the searches.
        const auto searchOption = static_cast<std::int32_t>(pair.parameters.searchOption.value.value_or(0.0));
        std::vector<SearchSide> order;
        const bool secondaryFirst = pair.secondary->id <= pair.primary->id;
        if (searchOption == 1)
            order = {SearchSide::Secondary};
        else if (searchOption == 2)
        {
            const double secondaryLength = characteristicLength(secondary, thicknessOffInOrder);
            const double primaryLength = characteristicLength(primary, thicknessOffInOrder);
            const double lower = secondaryFirst ? secondaryLength : primaryLength;
            const double higher = secondaryFirst ? primaryLength : secondaryLength;
            const bool lowerFirst = lower <= ORDER_RATIO * higher;
            order = {lowerFirst == secondaryFirst ? SearchSide::Secondary : SearchSide::Primary};
        }
        else
            order = secondaryFirst ? std::vector<SearchSide>{SearchSide::Secondary, SearchSide::Primary}
                                   : std::vector<SearchSide>{SearchSide::Primary, SearchSide::Secondary};

        PairConstraints constraints(dependentInModel);
        for (const SearchSide side : order)
        {
            const bool fromSecondary = side == SearchSide::Secondary;
            const ContactSurface& from = fromSecondary ? secondary : primary;
            const ContactSurface& onto = fromSecondary ? primary : secondary;
            const std::vector<ContactNodePair> found =
                ContactSurfaces::nodePairs(from, onto, zone, fromSecondary ? secondarySkins : primarySkins,
                                           fromSecondary ? primarySkins : secondarySkins);
            for (const ContactNodePair& candidate : found)
            {
                const ContactFace& face = onto.faces()[candidate.face];
                if (!constraints.tryTie(candidate.nid, face))
                    continue;
                ContactMeeting meeting;
                meeting.nid = candidate.nid;
                meeting.secondary = fromSecondary ? pair.secondary : pair.primary;
                meeting.primary = fromSecondary ? pair.primary : pair.secondary;
                meeting.face = &face;
                meeting.point = candidate.point;
                meeting.gap = candidate.gap;
                meeting.distance = candidate.distance;
                meeting.coordinates = faceCoordinatesOf(face, candidate.point);
                meeting.searchedFrom = side;
                meeting.surfaceOffset = candidate.surfaceOffset;
                result.meetings.push_back(meeting);
            }
        }
        result.meets = !result.meetings.empty();
        result.isReversed =
            result.meets && std::all_of(result.meetings.begin(), result.meetings.end(), [](const ContactMeeting& item)
                                        { return item.searchedFrom == SearchSide::Primary; });
        for (const ContactMeeting& meeting : result.meetings)
        {
            result.minDistance =
                std::isnan(result.minDistance) ? meeting.distance : std::min(result.minDistance, meeting.distance);
            result.maxDistance =
                std::isnan(result.maxDistance) ? meeting.distance : std::max(result.maxDistance, meeting.distance);
        }
    }
    return results;
}

} // namespace oor::analysis
