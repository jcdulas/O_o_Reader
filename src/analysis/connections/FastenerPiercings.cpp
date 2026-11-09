#include "oor/analysis/connections/FastenerPiercings.hpp"

#include "fasteners/AxisCrossing.hpp"
#include "fasteners/FastenerInputs.hpp"
#include "fasteners/NodePositions.hpp"
#include "fasteners/PropertyShellIndex.hpp"
#include "fasteners/ShellCandidates.hpp"
#include "fasteners/ShellElements.hpp"
#include "fasteners/SurfaceProjection.hpp"
#include "oor/analysis/adjacency/AdjacencyCache.hpp"
#include "oor/analysis/components/ComponentCache.hpp"
#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/base/threading/ParallelFor.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <optional>
#include <string>
#include <utility>

namespace oor::analysis
{

std::string_view fastenerFormName(FastenerForm form) noexcept
{
    switch (form)
    {
    case FastenerForm::Elem:
        return "Elements";
    case FastenerForm::Prop:
        return "Property";
    case FastenerForm::Elpat:
        return "ElemPatch";
    case FastenerForm::Partpat:
        return "PartPatch";
    case FastenerForm::Elemid:
        return "ElemIdPatch";
    case FastenerForm::Gridid:
        return "NodeIdPatch";
    case FastenerForm::Align:
        return "AlignedNodes";
    }
    return "?";
}

namespace
{

using detail::FastenerInput;
using detail::HostedPoint;
using detail::ShellElement;

// The natural tolerance of the first pass of a walk along surface B: wide, so that the element nearest a point is found
// even when the point projects outside every element (the walk then moves to the element its crossing points to).
constexpr double WALK_TOLERANCE = 1.0;
constexpr int MAX_WALK_STEPS = 16;

std::string number(double value)
{
    char text[32];
    std::snprintf(text, sizeof(text), "%.6g", value);
    return text;
}

std::string pointText(const base::Vec3& point)
{
    return "(" + number(point.x) + ", " + number(point.y) + ", " + number(point.z) + ")";
}

// The exact neutral field name used by messages and the field column.
std::string fieldText(const FastenerInput&, std::string_view field)
{
    return std::string(field);
}

// The neutral shell card names used in messages.
std::string shellCardsText()
{
    static constexpr std::array<std::string_view, 6> SHELL_CARDS = {
        "ShellQuad4", "ShellQuad4Drilling", "ShellTria3", "ShellTria3Drilling", "ShellQuad8", "ShellTria6"};
    std::string text;
    for (const std::string_view card : SHELL_CARDS)
    {
        if (!text.empty())
            text += ", ";
        text += std::string(card);
    }
    return text;
}

// The keyword of the card field of the surface of a side.
std::string surfaceField(const FastenerInput& input, char side)
{
    const bool isA = side == 'A';
    switch (input.form)
    {
    case FastenerForm::Prop:
    case FastenerForm::Elem:
        return fieldText(input, isA ? "patchIdA" : "patchIdB");
    case FastenerForm::Partpat:
        return fieldText(input, isA ? "propIdA" : "propIdB");
    case FastenerForm::Elpat:
    case FastenerForm::Elemid:
        return fieldText(input, isA ? "shellIdA" : "shellIdB");
    case FastenerForm::Gridid:
        return fieldText(input, isA ? "nodeA1" : "nodeB1");
    case FastenerForm::Align:
        break;
    }
    return fieldText(input, isA ? "nodeA" : "nodeB");
}

bool isPointToPatch(const FastenerInput& input) noexcept
{
    return (input.form == FastenerForm::Elemid && input.idb == 0) ||
           (input.form == FastenerForm::Gridid && input.shapeB == 0);
}

// The shells one side may pierce: those of a property (searched through its cells), or a named element with, for
// ELEM and ELPAT, the shells around it. `problem` says why the surface cannot be searched at all.
struct SideSurface
{
    const detail::PropertyShellIndex* property = nullptr;
    std::optional<ShellElement> named;
    std::vector<ShellElement> around; // by element ID
    std::string field;
    std::string text; // what the surface is, for the messages
    std::string problem;
};

// A point placed on one element of a surface: the element, the natural coordinates brought onto it and the point there,
// and how far it was found from what was projected (or from the start of the axis).
struct Placement
{
    ShellElement element;
    std::array<double, 2> natural{};
    base::Vec3 point;
    double distance = 0.0;
};

// The outcome of a search on a surface: a placement, or the message of what was not found.
struct SearchOutcome
{
    std::optional<Placement> placement;
    std::string problem;
};

Placement placeOn(const ShellElement& element, double xi, double eta, double distance)
{
    Placement placement;
    placement.element = element;
    placement.natural = detail::clampToElement(element.kind, xi, eta);
    const detail::ShapeValues values =
        detail::ShellShape::evaluate(element.kind, element.presentNodes, placement.natural[0], placement.natural[1]);
    placement.point = element.pointAt(values);
    placement.distance = distance;
    return placement;
}

double reachOf(const ShellElement& element) noexcept
{
    return PIERCING_REACH_IN_SIZES * detail::elementSize(element);
}

std::string elementText(const ShellElement& element)
{
    return element.isVirtual ? std::string("the face of grid points") : "element " + std::to_string(element.eid);
}

std::string toleranceText(double tolerance)
{
    const std::string projectionTolerance = std::string("ProjectionTolerance");
    return "within " + number(tolerance) + " of its edges in natural coordinates (" + projectionTolerance + ") and " +
           number(PIERCING_REACH_IN_SIZES) + " element sizes of the point";
}

class Piercer
{
public:
    Piercer(const detail::NodePositions& positions, const detail::ShellElements& elements,
            const detail::ShellCandidates& candidates)
        : m_positions(positions), m_elements(elements), m_candidates(candidates)
    {
    }

    // The rows of one fastener: side A, and side B unless it is a point-to-patch connection; none for ALIGN.
    [[nodiscard]] std::vector<FastenerPiercing> pierce(const FastenerInput& input) const;

private:
    [[nodiscard]] SideSurface surfaceOf(const FastenerInput& input, char side) const;
    // The element of the surface on which `point` falls, nearest to it (projection).
    [[nodiscard]] SearchOutcome footOn(const SideSurface& surface, const base::Vec3& point, double tolerance,
                                       const std::string& what) const;
    // The crossing of the line origin + t·direction with the surface nearest to origin.
    [[nodiscard]] SearchOutcome crossingOn(const SideSurface& surface, const base::Vec3& origin,
                                           const base::Vec3& direction, double tolerance) const;
    [[nodiscard]] static std::optional<HostedPoint> hostIn(const SideSurface& surface, const base::Vec3& point,
                                                           double tolerance);

    const detail::NodePositions& m_positions;
    const detail::ShellElements& m_elements;
    const detail::ShellCandidates& m_candidates;
};

SideSurface Piercer::surfaceOf(const FastenerInput& input, char side) const
{
    SideSurface surface;
    surface.field = surfaceField(input, side);
    const base::EntityId id = side == 'A' ? input.ida : input.idb;
    switch (input.form)
    {
    case FastenerForm::Prop:
    case FastenerForm::Partpat:
        surface.text = "the shells of property " + std::to_string(id);
        surface.property = m_candidates.property(id);
        if (surface.property == nullptr)
            surface.problem = "no shell element (" + shellCardsText() +
                              ") with its grid points placed has the property " + std::to_string(id) + "; correct " +
                              surface.field;
        break;
    case FastenerForm::Elem:
    case FastenerForm::Elpat:
    case FastenerForm::Elemid:
        surface.text = "shell element " + std::to_string(id);
        surface.named = m_elements.readById(id);
        if (!surface.named)
            surface.problem = "element " + std::to_string(id) + " is not a shell element (" + shellCardsText() +
                              ") of the model with its grid points placed; correct " + surface.field;
        else if (input.form != FastenerForm::Elemid)
        {
            surface.text += " and the shells around it";
            surface.around = m_candidates.ringOf(*surface.named);
        }
        break;
    case FastenerForm::Gridid:
    {
        const char shape = side == 'A' ? input.shapeA : input.shapeB;
        const bool isA = side == 'A';
        surface.text = "the face of the grid points " + fieldText(input, isA ? "nodeA1" : "nodeB1") + "-" +
                       fieldText(input, isA ? "nodeA8" : "nodeB8");
        if (shape != 0)
            surface.named = m_elements.fromGrids(side == 'A' ? input.gai : input.gbi, shape);
        if (!surface.named)
            surface.problem = surface.text +
                              " does not make a face: a corner is blank or a grid point is not defined; correct the "
                              "grid points or " +
                              fieldText(input, "patchShapes");
        break;
    }
    case FastenerForm::Align:
        surface.problem = "an ALIGN connection has no surface";
        break;
    }
    return surface;
}

std::optional<HostedPoint> Piercer::hostIn(const SideSurface& surface, const base::Vec3& point, double tolerance)
{
    // Strictly inside first, then within the tolerance: of two elements sharing an edge, the one the point is in.
    for (const double allowed : {0.0, tolerance})
    {
        if (surface.property != nullptr)
        {
            if (auto host = surface.property->nearestInside(point, allowed))
                return host;
            continue;
        }
        if (surface.named)
        {
            const detail::SurfaceProjection projection = detail::SurfaceProjection::onto(*surface.named, point);
            if (projection.isInside(allowed))
                return HostedPoint{*surface.named, projection};
        }
    }
    for (const double allowed : {0.0, tolerance})
    {
        if (auto host = detail::ShellCandidates::nearestInside(surface.around, point, allowed))
            return host;
    }
    return std::nullopt;
}

SearchOutcome Piercer::footOn(const SideSurface& surface, const base::Vec3& point, double tolerance,
                              const std::string& what) const
{
    SearchOutcome outcome;
    const std::optional<HostedPoint> host = hostIn(surface, point, tolerance);
    if (host && host->projection.distance <= reachOf(host->element))
    {
        outcome.placement =
            placeOn(host->element, host->projection.xi, host->projection.eta, host->projection.distance);
        return outcome;
    }
    outcome.problem = "the projection of " + what + " " + pointText(point) + " falls on none of " + surface.text + " " +
                      toleranceText(tolerance);
    if (host)
        outcome.problem += "; it falls on " + elementText(host->element) + " but " + number(host->projection.distance) +
                           " away, more than " + number(reachOf(host->element));
    else if (const auto nearest = hostIn(surface, point, WALK_TOLERANCE))
        outcome.problem += "; the nearest, " + elementText(nearest->element) + ", is " +
                           number(nearest->projection.distance) + " away and its projection " +
                           number(nearest->projection.outside) + " outside its edges";
    outcome.problem += "; move the location onto the shells or correct " + surface.field;
    return outcome;
}

SearchOutcome Piercer::crossingOn(const SideSurface& surface, const base::Vec3& origin, const base::Vec3& direction,
                                  double tolerance) const
{
    SearchOutcome outcome;
    // An element accepts the crossing when it lies on it (within the tolerance) and within its reach of the origin.
    const auto accepts = [&](const ShellElement& element, const detail::LineCrossing& crossing)
    {
        return detail::ShellShape::outsideDistance(element.kind, crossing.xi, crossing.eta) <= tolerance &&
               std::abs(crossing.t) <= reachOf(element);
    };
    const auto place = [&](const ShellElement& element, const detail::LineCrossing& crossing)
    { outcome.placement = placeOn(element, crossing.xi, crossing.eta, std::abs(crossing.t)); };

    if (surface.property != nullptr)
    {
        // A walk on the shells of the property: from the element nearest the origin, to the element nearest where the
        // axis crosses the surface of the current one, until the crossing falls on the current element. The element
        // of a point is the one it projects into, else within the tolerance, else the nearest within WALK_TOLERANCE:
        // the projection on the extended surfaces of the neighbours of a flat sheet is as near as the one inside.
        const auto elementNear = [&](const base::Vec3& point)
        {
            std::optional<HostedPoint> found;
            for (const double allowed : {0.0, tolerance, WALK_TOLERANCE})
            {
                found = surface.property->nearestInside(point, allowed);
                if (found)
                    break;
            }
            return found;
        };
        std::optional<HostedPoint> host = elementNear(origin);
        for (int step = 0; host && step < MAX_WALK_STEPS; ++step)
        {
            const std::optional<detail::LineCrossing> crossing = detail::crossElement(host->element, origin, direction);
            if (!crossing)
                break;
            if (accepts(host->element, *crossing))
            {
                place(host->element, *crossing);
                return outcome;
            }
            const detail::ShapeValues values = detail::ShellShape::evaluate(
                host->element.kind, host->element.presentNodes, crossing->xi, crossing->eta);
            std::optional<HostedPoint> next = elementNear(host->element.pointAt(values));
            if (!next || next->element.eid == host->element.eid)
                break; // the axis leaves the surface beside this element
            host = std::move(next);
        }
    }
    else
    {
        // The named element first, then the nearest crossing (smallest |t|) among the shells around it.
        if (surface.named)
        {
            const auto crossing = detail::crossElement(*surface.named, origin, direction);
            if (crossing && accepts(*surface.named, *crossing))
            {
                place(*surface.named, *crossing);
                return outcome;
            }
        }
        for (const ShellElement& element : surface.around)
        {
            const auto crossing = detail::crossElement(element, origin, direction);
            if (crossing && accepts(element, *crossing) &&
                (!outcome.placement || std::abs(crossing->t) < outcome.placement->distance))
                place(element, *crossing);
        }
        if (outcome.placement)
            return outcome;
    }
    outcome.problem = "the fastener axis from " + pointText(origin) + " along " + pointText(direction) +
                      " (the normal of the other side) crosses none of " + surface.text + " " +
                      toleranceText(tolerance) +
                      "; check that the two surfaces face each other, or give the grid "
                      "point of this side";
    return outcome;
}

// The row of a side placed on an element.
void fill(FastenerPiercing& row, const Placement& placement, const std::optional<base::Vec3>& givenPoint)
{
    row.isFound = true;
    row.position = givenPoint ? *givenPoint : placement.point;
    row.hasElement = !placement.element.isVirtual;
    row.element = placement.element.location;
    row.elementId = placement.element.isVirtual ? 0 : placement.element.eid;
    row.xi = placement.natural[0];
    row.eta = placement.natural[1];
    const detail::ShapeValues values = detail::ShellShape::evaluate(
        placement.element.kind, placement.element.presentNodes, placement.natural[0], placement.natural[1]);
    for (std::size_t slot = 0; slot < placement.element.slotCount(); ++slot)
    {
        if (!placement.element.hasNode(slot))
            continue;
        row.nids.push_back(placement.element.nodes[slot]);
        row.shapeValues.push_back(values.n[slot]);
    }
}

void fail(FastenerPiercing& row, std::string field, std::string problem)
{
    row.isFound = false;
    row.field = std::move(field);
    row.problem = std::move(problem);
}

std::vector<FastenerPiercing> Piercer::pierce(const FastenerInput& input) const
{
    std::vector<FastenerPiercing> rows;
    if (input.form == FastenerForm::Align)
        return rows;
    const auto field = [&](std::string_view name) { return fieldText(input, name); };
    const std::array<std::string, 2> nodeFields = {field("nodeA"), field("nodeB")};
    const std::string spotField = field("spotNode");
    const std::string stationFields = field("locationX") + ", " + field("locationY") + ", " + field("locationZ");
    const bool pointToPatch = isPointToPatch(input);
    const double tolerance = std::abs(input.projtol);
    std::array<FastenerPiercing, 2> sides;
    std::array<SideSurface, 2> surfaces;
    std::array<std::optional<base::Vec3>, 2> given; // the positions of GA and GB when the card gives them
    std::array<std::optional<Placement>, 2> placed;
    const std::size_t sideCount = pointToPatch ? 1 : 2;
    for (std::size_t s = 0; s < sideCount; ++s)
    {
        const char name = s == 0 ? 'A' : 'B';
        FastenerPiercing& row = sides[s];
        row.card = input.card;
        row.eid = input.eid;
        row.form = input.form;
        row.side = name;
        row.pointGrid = s == 0 ? input.ga : input.gb;
        surfaces[s] = surfaceOf(input, name);
        if (!surfaces[s].problem.empty())
            fail(row, surfaces[s].field, surfaces[s].problem);
        if (row.pointGrid != 0)
        {
            given[s] = m_positions.of(row.pointGrid);
            if (!given[s])
                fail(row, nodeFields[s],
                     "grid point " + std::to_string(row.pointGrid) +
                         " is not defined in the model (or has no "
                         "position); define it or correct the field");
        }
    }
    const auto usable = [&](std::size_t s) { return sides[s].problem.empty(); };

    // Rule 1: the given grid points, each on its own surface.
    for (std::size_t s = 0; s < sideCount; ++s)
    {
        if (!usable(s) || !given[s])
            continue;
        const SearchOutcome outcome =
            footOn(surfaces[s], *given[s], tolerance, nodeFields[s] + " " + std::to_string(sides[s].pointGrid));
        if (outcome.placement)
            placed[s] = outcome.placement;
        else
            fail(sides[s], nodeFields[s], outcome.problem);
    }

    // Rule 2: side A from the location, when GA is not given.
    std::optional<base::Vec3> location;
    std::string locationText;
    if (input.gs != 0)
    {
        location = m_positions.of(input.gs);
        locationText = spotField + " " + std::to_string(input.gs);
        if (!location && usable(0) && !given[0])
            fail(sides[0], spotField,
                 "grid point " + locationText +
                     " is not defined in the model (or has no position); define it or correct " + spotField);
    }
    else if (input.location)
    {
        location = input.location;
        locationText = "the point " + stationFields;
    }
    if (usable(0) && !given[0] && location)
    {
        const SearchOutcome outcome = footOn(surfaces[0], *location, tolerance, locationText);
        if (outcome.placement)
            placed[0] = outcome.placement;
        else
            fail(sides[0], surfaces[0].field, outcome.problem);
    }

    // Rule 3: the side still to place is the crossing of the axis from the other side.
    const auto crossFrom = [&](std::size_t from, std::size_t to)
    {
        const Placement& start = *placed[from];
        const base::Vec3 origin = given[from] ? *given[from] : start.point;
        const std::optional<base::Vec3> normal =
            detail::surfaceNormal(start.element, start.natural[0], start.natural[1]);
        if (!normal)
        {
            fail(sides[to], surfaces[from].field,
                 "the element pierced on side " + std::string(1, sides[from].side) +
                     " has no normal at the point (a degenerate element): the fastener axis is undefined");
            return;
        }
        const SearchOutcome outcome = crossingOn(surfaces[to], origin, *normal, tolerance);
        if (outcome.placement)
            placed[to] = outcome.placement;
        else
            fail(sides[to], surfaces[to].field, outcome.problem);
    };
    if (sideCount == 2 && usable(1) && !placed[1] && placed[0])
        crossFrom(0, 1);
    if (sideCount == 2 && usable(0) && !placed[0] && placed[1] && !location)
        crossFrom(1, 0);

    for (std::size_t s = 0; s < sideCount; ++s)
    {
        FastenerPiercing& row = sides[s];
        if (placed[s] && usable(s))
            fill(row, *placed[s], given[s]);
        else if (usable(s))
        {
            const std::size_t other = 1 - s;
            if (s == 0 && !location && !given[0] && !given[1])
                fail(row, spotField,
                     "the fastener has no location: give " + spotField + ", " + nodeFields[0] + " or " + stationFields);
            else
                fail(row, surfaces[s].field,
                     "the piercing point of side " + std::string(1, sides[other].side) +
                         " was not found, so the fastener axis that places this side is unknown (see field " +
                         sides[other].field + ")");
        }
    }
    for (std::size_t s = 0; s < sideCount; ++s)
        rows.push_back(std::move(sides[s]));
    return rows;
}

// Every side of a fastener whose positions cannot be computed: not found, with the reason.
std::vector<FastenerPiercing> unplaced(const FastenerInput& input, const std::string& reason)
{
    std::vector<FastenerPiercing> rows;
    if (input.form == FastenerForm::Align)
        return rows;
    for (const char name : {'A', 'B'})
    {
        if (name == 'B' && isPointToPatch(input))
            break;
        FastenerPiercing row;
        row.card = input.card;
        row.eid = input.eid;
        row.form = input.form;
        row.side = name;
        row.pointGrid = name == 'A' ? input.ga : input.gb;
        fail(row, fieldText(input, "spotNode"), reason);
        rows.push_back(std::move(row));
    }
    return rows;
}

// The fastener as a connection, without its sides: its card and the grid points of the card it joins itself.
PiercedFastener connectionOf(const FastenerInput& input)
{
    PiercedFastener fastener;
    fastener.card = input.card;
    fastener.eid = input.eid;
    fastener.form = input.form;
    fastener.ga = input.ga;
    fastener.gb = input.gb;
    const bool pointToPatch = isPointToPatch(input);
    if (input.ga != 0)
        fastener.ownNodes.push_back(input.ga);
    if (input.gb != 0 && !pointToPatch)
        fastener.ownNodes.push_back(input.gb);
    if (pointToPatch && input.gs != 0)
        fastener.ownNodes.push_back(input.gs); // the other end of a point-to-patch connection
    std::sort(fastener.ownNodes.begin(), fastener.ownNodes.end());
    fastener.ownNodes.erase(std::unique(fastener.ownNodes.begin(), fastener.ownNodes.end()), fastener.ownNodes.end());
    return fastener;
}

bool locationLess(const model::CardLocation& left, const model::CardLocation& right) noexcept
{
    return left.table != right.table ? left.table < right.table : left.row < right.row;
}

} // namespace

FastenerPiercings FastenerPiercings::compute(const model::Model& model, base::ThreadPool* pool)
{
    FastenerPiercings result;
    const std::vector<FastenerInput> inputs = detail::FastenerInputs::read(model);
    if (inputs.empty())
        return result;
    std::vector<std::vector<FastenerPiercing>> perFastener(inputs.size());
    BasicPositionCache* cache = nullptr;
    try
    {
        cache = &BasicPositionCache::of(model, pool);
    }
    catch (const base::OorError& error)
    {
        for (std::size_t index = 0; index < inputs.size(); ++index)
            perFastener[index] = unplaced(
                inputs[index], std::string("the positions of the grid points cannot be computed: ") + error.what());
    }
    if (cache != nullptr)
    {
        const detail::NodePositions positions(model, cache->positions());
        const detail::ShellElements elements(model, positions);
        const std::shared_ptr<const NodeToElementIndex> nodeIndex = AdjacencyCache::of(model).nodeIndex(model, pool);
        detail::ShellCandidates candidates(model, elements, *nodeIndex);
        std::vector<base::EntityId> properties;
        for (const FastenerInput& input : inputs)
        {
            if (input.form == FastenerForm::Prop || input.form == FastenerForm::Partpat)
            {
                properties.push_back(input.ida);
                properties.push_back(input.idb);
            }
        }
        candidates.prepare(properties); // before the work spreads over threads: the indexes are then read only
        const Piercer piercer(positions, elements, candidates);
        const auto work = [&](std::size_t index) { perFastener[index] = piercer.pierce(inputs[index]); };
        if (pool != nullptr && inputs.size() > 1)
            base::parallelFor(*pool, inputs.size(), work);
        else
        {
            for (std::size_t index = 0; index < inputs.size(); ++index)
                work(index);
        }
    }
    result.m_fasteners.reserve(inputs.size());
    for (std::size_t index = 0; index < inputs.size(); ++index)
    {
        PiercedFastener fastener = connectionOf(inputs[index]);
        fastener.firstSide = static_cast<std::uint32_t>(result.m_sides.size());
        fastener.sideCount = static_cast<std::uint32_t>(perFastener[index].size());
        for (FastenerPiercing& row : perFastener[index])
            result.m_sides.push_back(std::move(row));
        result.m_fasteners.push_back(std::move(fastener));
    }
    result.m_byLocation.resize(result.m_fasteners.size());
    for (std::size_t index = 0; index < result.m_fasteners.size(); ++index)
        result.m_byLocation[index] = static_cast<std::uint32_t>(index);
    std::sort(result.m_byLocation.begin(), result.m_byLocation.end(), [&](std::uint32_t left, std::uint32_t right)
              { return locationLess(result.m_fasteners[left].card, result.m_fasteners[right].card); });
    return result;
}

std::shared_ptr<const FastenerPiercings> FastenerPiercings::of(const model::Model& model, base::ThreadPool* pool)
{
    return ComponentCache::of(model).fastenerPiercings(model, pool);
}

const PiercedFastener* FastenerPiercings::find(model::CardLocation card) const noexcept
{
    const auto found = std::lower_bound(m_byLocation.begin(), m_byLocation.end(), card,
                                        [&](std::uint32_t index, const model::CardLocation& wanted)
                                        { return locationLess(m_fasteners[index].card, wanted); });
    if (found == m_byLocation.end() || !(m_fasteners[*found].card == card))
        return nullptr;
    return &m_fasteners[*found];
}

std::vector<base::EntityId> FastenerPiercings::connectionNodes(const PiercedFastener& fastener) const
{
    std::vector<base::EntityId> nodes = fastener.ownNodes;
    for (const FastenerPiercing& side : sidesOf(fastener))
        nodes.insert(nodes.end(), side.nids.begin(), side.nids.end());
    std::sort(nodes.begin(), nodes.end());
    nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
    return nodes;
}

} // namespace oor::analysis
