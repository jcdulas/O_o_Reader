#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/analysis/components/ElementRoles.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/common/NodePositions.hpp"
#include "oor/query/named/common/Topology.hpp"
#include "oor/query/providers/quantities/CutForceSum.hpp"
#include "oor/query/providers/quantities/CutSession.hpp"
#include "oor/query/providers/quantities/ElementQuantities.hpp"
#include "oor/query/providers/quantities/Resultants.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <tuple>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();
using Point = std::array<double, 3>;

Point mean(const std::vector<Point>& points)
{
    Point sum = {0.0, 0.0, 0.0};
    for (const Point& point : points)
        for (std::size_t axis = 0; axis < 3; ++axis)
            sum[axis] += point[axis];
    if (!points.empty())
        for (double& value : sum)
            value /= static_cast<double>(points.size());
    return sum;
}

// ---- the grid point forces across the cut
// -----------------------------------------------------------------------------------

// The forces the side pushes across the cut, from the grid point forces (CutForceSum): the rows of the side elements
// at the cut grid points and the cut connections counted from their multiPointConstrForce rows. Exact at equilibrium; a
// load path it cannot count is named in the row.
class CutNodePointForcesProvider final : public ResultantProvider
{
public:
    [[nodiscard]] std::string name() const override { return "nodePointForces"; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ResultantRequest& request,
                                  ResultantSet& sink) const override
    {
        if (request.method == "integration")
            return {0, "method=\"integration\" was asked for"};
        if (!request.cut)
            return {0, "the cut is not defined"};
        const std::shared_ptr<CutSession> session =
            request.session ? request.session : std::make_shared<CutSession>(context);
        const NodePointForceSet& forces = session->nodePointForces();
        if (forces.empty())
            return {0, "the result file has no grid point force table (output of GPFORCE was not requested)"};
        const model::Model& model = context.requireModel("place the grid points of the cut");
        const NodePositions& positions = session->positions(model);
        Outcome outcome;
        bool anyRow = false;
        // the connections crossing the cut, once for each MPC selection; the rows the cut sums, once for each layout of
        // the tables (the subcases of a result set mostly share one) with those connections
        std::map<const CutConnections*, std::vector<CutConnection>> crossingOf;
        using PlanKey = std::tuple<const NodePointForceLayout*, bool, const std::vector<CutConnection>*>;
        std::map<PlanKey, CutForceSum::Plan> plans;
        CutTableSum sum;
        for (const NodePointForceTable& table : forces.tables())
        {
            const CutConnections* connections = session->connections(table.set, table.subcase);
            auto found = crossingOf.find(connections);
            if (found == crossingOf.end())
                found = crossingOf
                            .emplace(connections, connections != nullptr ? connections->crossing(*request.cut)
                                                                         : std::vector<CutConnection>())
                            .first;
            const PlanKey key{table.layout.get(), table.isStatic(), &found->second};
            auto plan = plans.find(key);
            if (plan == plans.end())
                plan = plans.emplace(key, CutForceSum::plan(table, *request.cut, found->second, positions)).first;
            CutForceSum::apply(plan->second, table, request.point, request.nodeDetail, sum);
            if (!sum.hasRows)
                continue;
            anyRow = true;
            for (std::size_t step = 0; step < table.stepCount(); ++step)
            {
                if (sink.covers(table.set, table.subcase, table.designCycle, table.steps[step]))
                    continue;
                ResultantRow row;
                row.set = table.set;
                row.analysis = table.analysis;
                row.subcase = table.subcase;
                row.designCycle = table.designCycle;
                row.stepKind = table.stepKind;
                row.step = table.steps[step];
                row.eigenvalue = table.eigenvalues[step];
                row.precision = table.precision;
                row.isComplex = table.isComplex;
                row.magnitudePhase = table.magnitudePhase;
                row.value = sum.real[step];
                row.imaginary = sum.imaginary[step];
                row.count = sum.nodeCount;
                row.elementCount = sum.elementCount;
                row.complete = sum.complete;
                row.uncountedCards = sum.uncountedCards;
                row.uncountedIds = sum.uncountedIds;
                if (request.nodeDetail)
                    row.nodes = sum.nodes[step];
                if (sink.add(std::move(row)))
                    ++outcome.rows;
            }
        }
        if (outcome.rows == 0)
            outcome.reason =
                anyRow ? "every step was already given"
                       : "the grid point forces have no row of the elements of the side at the cut grid points";
        return outcome;
    }
};

// ---- the integration of the shell forces across the cut
// ------------------------------------------------------------------------

using ShellGeometry = CutIntegrationIndex::Shell;

Point cross(const Point& a, const Point& b)
{
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
double dot(const Point& a, const Point& b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
Point sub(const Point& a, const Point& b)
{
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}
Point add(const Point& a, const Point& b)
{
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}
Point scale(const Point& a, double factor)
{
    return {a[0] * factor, a[1] * factor, a[2] * factor};
}

// A piece of the cut in an element: its end points and the outward normal of the side (in the plane of the element).
struct Segment
{
    Point from;
    Point to;
    Point outward;
};

// Integrates the forces of the shell elements across the cut: the centroid forces of each shell times the length of the
// piece of the cut inside it. The elements of the side that the plane crosses, or that lie against it, give the pieces
// of a plane; for a cut along grid points the edges of the elements of the side that join two cut grid points are the
// pieces. The force of an element is taken as constant over it: an estimate, marked approximate. Only shells give a
// contribution (a rod or a bar that crosses the cut is not counted).
class CutIntegrationProvider final : public ResultantProvider
{
public:
    [[nodiscard]] std::string name() const override { return "integration"; }
    [[nodiscard]] bool approximate() const override { return true; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ResultantRequest& request,
                                  ResultantSet& sink) const override
    {
        if (request.method == "nodePointForces")
            return {0, "method=\"nodePointForces\" was asked for"};
        if (!context.hasModel())
            return {0, "no model is attached to the results (the geometry of the cut is in the model)"};
        if (!request.cut)
            return {0, "the cut is not defined"};
        const model::Model& model = *context.modelPointer();
        // the forces of the shells, the positions and the elements: once for the cuts of a call
        const std::shared_ptr<CutSession> session =
            request.session ? request.session : std::make_shared<CutSession>(context);
        const CutSession::ShellForces& shellForces = session->shellForces();
        const ElementFlatSet& forces = shellForces.set;
        if (forces.rowCount() == 0)
            return {0, "no force of shells in the result file, and no stress to integrate: " +
                           describeAttempts("the shell forces", shellForces.attempts)};
        // PERFORMANCE: the shells with their geometry and rows are indexed once for the call: a cut
        // looks at the shells that can reach it (a plane) or at its side elements (a line of grid points), not at
        // every row of every subcase
        const CutIntegrationIndex& index = session->integrationIndex(model);
        const std::set<std::int32_t> cutNodes(request.cut->nodes.begin(), request.cut->nodes.end());
        const std::set<std::int32_t> side(request.cut->sideElements.begin(), request.cut->sideElements.end());
        const auto segmentsOf = [&](const ShellGeometry& shell, bool onSide) -> std::vector<Segment>
        {
            std::vector<Segment> segments;
            const std::size_t count = shell.corners.size();
            const Point centre = mean(shell.corners);
            if (request.cut->isPlane)
            {
                const Point& normal = request.cut->planeNormal;
                std::vector<double> distance(count);
                bool below = false;
                bool above = false;
                std::size_t on = 0;
                double span = 0.0;
                for (std::size_t corner = 0; corner < count; ++corner)
                {
                    distance[corner] = dot(normal, sub(shell.corners[corner], request.cut->planePoint));
                    span = std::max(span, std::fabs(distance[corner]));
                }
                const double tolerance = 1.0e-9 * std::max(span, 1.0);
                for (const double value : distance)
                {
                    below = below || value < -tolerance;
                    above = above || value > tolerance;
                    on += std::fabs(value) <= tolerance ? 1 : 0;
                }
                std::vector<Point> hits;
                if (below && above)
                {
                    for (std::size_t corner = 0; corner < count; ++corner)
                    {
                        const std::size_t next = (corner + 1) % count;
                        if (distance[corner] * distance[next] < 0.0)
                        {
                            const double fraction = distance[corner] / (distance[corner] - distance[next]);
                            hits.push_back(add(shell.corners[corner],
                                               scale(sub(shell.corners[next], shell.corners[corner]), fraction)));
                        }
                        else if (std::fabs(distance[corner]) <= tolerance)
                        {
                            hits.push_back(shell.corners[corner]);
                        }
                    }
                }
                else if (!above && on >= 2)
                {
                    for (std::size_t corner = 0; corner < count; ++corner)
                        if (std::fabs(distance[corner]) <= tolerance)
                            hits.push_back(shell.corners[corner]);
                }
                if (hits.size() >= 2)
                    segments.push_back(Segment{hits.front(), hits.back(), normal});
                return segments;
            }
            if (!onSide)
                return segments;
            for (std::size_t corner = 0; corner < count; ++corner)
            {
                const std::size_t next = (corner + 1) % count;
                if (!cutNodes.contains(shell.cornerNodes[corner]) || !cutNodes.contains(shell.cornerNodes[next]))
                    continue;
                const Point direction = sub(shell.corners[next], shell.corners[corner]);
                Point outward = cross(direction, shell.axes[2]);
                const double norm = std::sqrt(dot(outward, outward));
                if (!(norm > 0.0))
                    continue;
                outward = scale(outward, 1.0 / norm);
                const Point middle = scale(add(shell.corners[corner], shell.corners[next]), 0.5);
                if (dot(outward, sub(middle, centre)) < 0.0)
                    outward = scale(outward, -1.0);
                segments.push_back(Segment{shell.corners[corner], shell.corners[next], outward});
            }
            return segments;
        };

        struct Total
        {
            std::array<double, 6> sum{};
            std::set<std::int32_t> elements;
            Precision precision = Precision::Double;
            std::string analysis;
            results::StepKind stepKind = results::StepKind::None;
            double step = NOT_A_NUMBER;
            double eigenvalue = NOT_A_NUMBER;
        };
        std::map<std::tuple<std::size_t, std::int32_t, std::int32_t, std::uint64_t>, Total> totals;
        // the shells the cut can take: near a plane, or the side elements of a line of grid points
        std::vector<std::uint32_t> candidates;
        if (request.cut->isPlane)
            candidates = index.nearPlane(request.cut->planeNormal, request.cut->planePoint);
        else
            for (const std::int32_t element : side)
                if (const auto found = index.byModelId.find(element); found != index.byModelId.end())
                    candidates.push_back(found->second);
        // their pieces, then their rows in the order of the force set (the sums are those of a pass over every row)
        std::vector<std::vector<Segment>> piecesOf(candidates.size());
        std::vector<std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>> taken; // flat, row, candidate
        for (std::uint32_t at = 0; at < candidates.size(); ++at)
        {
            const ShellGeometry& shell = index.shells[candidates[at]];
            if (!shell.ok)
                continue;
            // a plane takes the pieces of the elements it crosses or lies against: the side is decided by the
            // geometry
            piecesOf[at] = segmentsOf(shell, side.contains(shell.modelId));
            if (piecesOf[at].empty())
                continue;
            index.forEachRow(
                shell, [&](std::uint32_t flatIndex, std::uint32_t row) { taken.emplace_back(flatIndex, row, at); });
        }
        std::sort(taken.begin(), taken.end());
        const std::vector<ElementFlat>& flats = forces.flats();
        for (const auto& [flatIndex, row, at] : taken)
        {
            {
                const ElementFlat& flat = flats[flatIndex];
                const ShellGeometry& shell = index.shells[candidates[at]];
                const std::vector<Segment>& segments = piecesOf[at];
                const double step = flat.step.empty() ? NOT_A_NUMBER : flat.step[row];
                const auto key = std::make_tuple(flat.set, flat.subcase, flat.designCycle, stepKey(step));
                Total& total = totals[key];
                total.analysis = flat.analysis;
                total.stepKind = flat.stepKind;
                total.step = step;
                total.eigenvalue = flat.eigenvalue.empty() ? NOT_A_NUMBER : flat.eigenvalue[row];
                if (flat.precision == Precision::Double)
                    total.precision = Precision::Double;
                const auto value = [&](const char* name) { return flat.values.at(name)[row]; };
                const double nx = value("nx"), ny = value("ny"), nxy = value("nxy");
                const double mx = value("mx"), my = value("my"), mxy = value("mxy");
                double qx = value("qx"), qy = value("qy");
                if (std::isnan(qx) || std::isnan(qy))
                    qx = qy = 0.0; // the integration of the stresses gives no transverse shear
                if (std::isnan(nx) || std::isnan(ny) || std::isnan(nxy))
                    continue;
                bool used = false;
                for (const Segment& segment : segments)
                {
                    const Point piece = sub(segment.to, segment.from);
                    const double length = std::sqrt(dot(piece, piece));
                    if (!(length > 0.0))
                        continue;
                    const double ex = dot(segment.outward, shell.axes[0]);
                    const double ey = dot(segment.outward, shell.axes[1]);
                    const double tx = nx * ex + nxy * ey;
                    const double ty = nxy * ex + ny * ey;
                    const double shear = qx * ex + qy * ey;
                    // the force the rest of the structure puts on the side across this piece, then what the side puts
                    // on the cut grid points is the opposite
                    const Point force =
                        scale(add(add(scale(shell.axes[0], tx), scale(shell.axes[1], ty)), scale(shell.axes[2], shear)),
                              length);
                    const double bendingX = -(mxy * ex + my * ey);
                    const double bendingY = mx * ex + mxy * ey;
                    const Point moment =
                        scale(add(scale(shell.axes[0], bendingX), scale(shell.axes[1], bendingY)), length);
                    const Point middle = scale(add(segment.from, segment.to), 0.5);
                    const Point arm = sub(middle, request.point);
                    const Point couple = add(cross(arm, force), std::isnan(mx) ? Point{0.0, 0.0, 0.0} : moment);
                    for (std::size_t axis = 0; axis < 3; ++axis)
                    {
                        total.sum[axis] -= force[axis];
                        total.sum[3 + axis] -= couple[axis];
                    }
                    used = true;
                }
                if (used)
                    total.elements.insert(shell.modelId);
            }
        }
        // what the integration cannot count: every cut connection, and the elements of the side on the cut that are
        // not shells (but the dampers: the shell forces it reads are real, and a damper carries no static load)
        std::set<std::pair<std::string, std::int32_t>> uncounted;
        for (const CutSideElement& element : request.cut->sideElementsAtCut)
        {
            const auto location = model.locate(base::IdSpace::Element, element.id);
            if ((!location || analysis::roleOfType(location->table) != analysis::ElementRole::Shell) && !element.damper)
                uncounted.emplace(element.card, element.id);
        }
        std::map<std::pair<std::size_t, std::int32_t>, std::set<std::pair<std::string, std::int32_t>>> uncountedOf;
        const auto uncountedFor = [&](std::size_t set,
                                      std::int32_t subcase) -> const std::set<std::pair<std::string, std::int32_t>>&
        {
            const auto key = std::make_pair(set, subcase);
            auto found = uncountedOf.find(key);
            if (found != uncountedOf.end())
                return found->second;
            std::set<std::pair<std::string, std::int32_t>> all = uncounted;
            if (const CutConnections* connections = session->connections(set, subcase))
                for (const CutConnection& connection : connections->crossing(*request.cut))
                    all.emplace(connection.card, connection.id);
            return uncountedOf.emplace(key, std::move(all)).first->second;
        };
        Outcome outcome;
        for (auto& [key, total] : totals)
        {
            if (sink.covers(std::get<0>(key), std::get<1>(key), std::get<2>(key), total.step))
                continue;
            ResultantRow row;
            for (const auto& [card, id] : uncountedFor(std::get<0>(key), std::get<1>(key)))
            {
                row.uncountedCards.push_back(card);
                row.uncountedIds.push_back(id);
            }
            row.complete = row.uncountedIds.empty();
            row.set = std::get<0>(key);
            row.analysis = total.analysis;
            row.subcase = std::get<1>(key);
            row.designCycle = std::get<2>(key);
            row.stepKind = total.stepKind;
            row.step = total.step;
            row.eigenvalue = total.eigenvalue;
            row.precision = total.precision;
            row.value = total.sum;
            row.count = static_cast<std::int64_t>(cutNodes.size());
            row.elementCount = static_cast<std::int64_t>(total.elements.size());
            row.approximate = true;
            if (sink.add(std::move(row)))
                ++outcome.rows;
        }
        if (outcome.rows == 0)
            outcome.reason = "no shell of the side has a force, or the cut meets no shell";
        return outcome;
    }
};

} // namespace

const ResultantChain& cutChain()
{
    static const ResultantChain CHAIN = []
    {
        ResultantChain chain("freebody and section cut resultant");
        chain.add(std::make_unique<CutNodePointForcesProvider>());
        chain.add(std::make_unique<CutIntegrationProvider>());
        return chain;
    }();
    return CHAIN;
}

} // namespace oor::query
