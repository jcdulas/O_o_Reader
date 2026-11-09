#include "oor/analysis/mass/SectionShearSolver.hpp"

#include "DualNumber.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace oor::analysis
{

namespace
{

// Cells across the thinnest band of the coarse mesh; the fine mesh has twice as many.
constexpr int CELLS_PER_THICKNESS = 6;
// A cell may be this many times longer than the thinnest cell of the mesh (along the walls, where the solution varies
// slowly).
constexpr double LONG_CELL_RATIO = 2.0;
// At most this many cells in one direction of the coarse mesh: a section with walls much longer than thick gets longer
// cells instead of a mesh that cannot be solved quickly.
constexpr std::size_t MAX_CELLS = 360;
// A section whose uniform coarse mesh would have more cells than this (a thin web makes every wall's cells as short as
// its own; walls many times longer than thick) is meshed with graded cells instead (a companion package D-577); the uniform
// plans of the validation decks and the reference models have at most 1 800. In a graded plan the length of the cells
// grows by GRADING_GROWTH times the distance away from a corner or a thinner wall, and is never under GRADING_FLOOR
// times the length the walls of the band ask for.
constexpr std::size_t GRADED_PLAN_CELLS = 2500;
constexpr double GRADING_GROWTH = 0.25;
constexpr double GRADING_FLOOR = 0.125;

using Point = std::array<double, 2>; // (y, z)

// A mesh of bilinear quadrilaterals: the corners of each one go (y0, z0), (y1, z0), (y1, z1), (y0, z1) in its own
// parameters, so that the Jacobian is positive.
struct QuadMesh
{
    std::vector<Point> nodes;
    std::vector<std::array<std::uint32_t, 4>> quads;
};

// The lines of a graded grid through the sorted, distinct breakpoints: each band gets at least `minimumCells` cells
// and cells no longer than `longest`.
std::vector<double> gridLines(const std::vector<double>& breaks, int minimumCells, double longest)
{
    std::vector<double> lines{breaks.front()};
    for (std::size_t band = 1; band < breaks.size(); ++band)
    {
        const double width = breaks[band] - breaks[band - 1];
        const auto count = std::max<std::size_t>(static_cast<std::size_t>(minimumCells),
                                                 static_cast<std::size_t>(std::ceil(width / longest - 1.0e-9)));
        for (std::size_t cell = 1; cell <= count; ++cell)
            lines.push_back(breaks[band - 1] + width * static_cast<double>(cell) / static_cast<double>(count));
    }
    return lines;
}

std::vector<double> distinct(std::vector<double> values)
{
    std::sort(values.begin(), values.end());
    std::vector<double> kept;
    const double span = values.back() - values.front();
    for (const double value : values)
    {
        if (kept.empty() || value - kept.back() > 1.0e-12 * span)
            kept.push_back(value);
    }
    return kept;
}

// The cells of each band of a graded grid through the sorted, distinct breakpoints: at least `minimumCells` and cells
// no longer than `longest`.
std::vector<std::size_t> bandCounts(const std::vector<double>& breaks, int minimumCells, double longest)
{
    std::vector<std::size_t> counts;
    for (std::size_t band = 1; band < breaks.size(); ++band)
    {
        const double width = breaks[band] - breaks[band - 1];
        counts.push_back(std::max<std::size_t>(static_cast<std::size_t>(minimumCells),
                                               static_cast<std::size_t>(std::ceil(width / longest - 1.0e-9))));
    }
    return counts;
}

// The distinct edges of the rectangles along y and z.
void breaksOf(std::span<const SectionRectangle> pieces, std::vector<double>& yBreaks, std::vector<double>& zBreaks)
{
    yBreaks.clear();
    zBreaks.clear();
    for (const SectionRectangle& piece : pieces)
    {
        yBreaks.insert(yBreaks.end(), {piece.y0, piece.y1});
        zBreaks.insert(zBreaks.end(), {piece.z0, piece.z1});
    }
    yBreaks = distinct(std::move(yBreaks));
    zBreaks = distinct(std::move(zBreaks));
}

// The index of the break `value` is (within the tolerance of `distinct`).
std::uint32_t breakIndex(const std::vector<double>& breaks, double value)
{
    const double tolerance = 1.0e-12 * (breaks.back() - breaks.front());
    std::size_t best = 0;
    for (std::size_t index = 1; index < breaks.size(); ++index)
        if (std::fabs(breaks[index] - value) < std::fabs(breaks[best] - value))
            best = index;
    return std::fabs(breaks[best] - value) <= tolerance ? static_cast<std::uint32_t>(best)
                                                        : static_cast<std::uint32_t>(breaks.size());
}

// The plan of the two meshes of a union of rectangles: CELLS_PER_THICKNESS cells across the thinnest band of the coarse
// mesh, cells at most LONG_CELL_RATIO times as long along the walls (longer while the mesh would have too many); the
// fine mesh has twice as many across and cells half as long. Each edge of each piece is one of the breaks.
SectionMeshPlan uniformPlan(std::span<const SectionRectangle> pieces)
{
    std::vector<double> yBreaks;
    std::vector<double> zBreaks;
    breaksOf(pieces, yBreaks, zBreaks);
    double thinnest = std::numeric_limits<double>::max();
    for (const SectionRectangle& piece : pieces)
        thinnest = std::min({thinnest, piece.y1 - piece.y0, piece.z1 - piece.z0});
    double longest = LONG_CELL_RATIO * thinnest / static_cast<double>(CELLS_PER_THICKNESS);
    while (gridLines(yBreaks, CELLS_PER_THICKNESS, longest).size() +
                   gridLines(zBreaks, CELLS_PER_THICKNESS, longest).size() >
               2 * MAX_CELLS &&
           longest < thinnest * 1.0e6)
        longest *= 1.5;
    SectionMeshPlan plan;
    plan.coarseY = bandCounts(yBreaks, CELLS_PER_THICKNESS, longest);
    plan.coarseZ = bandCounts(zBreaks, CELLS_PER_THICKNESS, longest);
    plan.fineY = bandCounts(yBreaks, 2 * CELLS_PER_THICKNESS, longest / 2.0);
    plan.fineZ = bandCounts(zBreaks, 2 * CELLS_PER_THICKNESS, longest / 2.0);
    for (const SectionRectangle& piece : pieces)
        plan.edges.push_back({breakIndex(yBreaks, piece.y0), breakIndex(yBreaks, piece.y1),
                              breakIndex(zBreaks, piece.z0), breakIndex(zBreaks, piece.z1)});
    return plan;
}

// The cells of the coarse mesh of a plan (the pieces do not overlap: each covers the product of its bands).
std::size_t coarseCells(const SectionMeshPlan& plan)
{
    std::size_t cells = 0;
    for (const auto& edges : plan.edges)
    {
        std::size_t across = 0;
        std::size_t along = 0;
        for (std::uint32_t band = edges[0]; band < edges[1]; ++band)
            across += plan.coarseY[band];
        for (std::uint32_t band = edges[2]; band < edges[3]; ++band)
            along += plan.coarseZ[band];
        cells += across * along;
    }
    return cells;
}

// The cells of the bands of one direction of a graded plan. The cell length wanted, s(x), is the least over the
// sources of their length plus GRADING_GROWTH times the distance to them: a source is a band with its own length
// (`sizes`, infinite for none) or a break with its own length (`points`, infinite for none); the ramps into a band
// start no lower than its floor (`floors`). A band where s is its own length everywhere gets cells of one length (no
// cuts, the fine mesh as in the uniform plan); the others get the cells of s (their number the integral of 1 / s, at
// least CELLS_PER_THICKNESS), cut where the integral reaches each of its equal parts.
void gradedBands(const std::vector<double>& breaks, const std::vector<double>& sizes, const std::vector<double>& points,
                 const std::vector<double>& floors, std::vector<std::size_t>& coarse, std::vector<std::size_t>& fine,
                 std::vector<std::vector<double>>& cuts)
{
    const std::size_t bands = sizes.size();
    coarse.assign(bands, 0);
    fine.assign(bands, 0);
    cuts.assign(bands, {});
    for (std::size_t band = 0; band < bands; ++band)
    {
        const double width = breaks[band + 1] - breaks[band];
        const double own = sizes[band];
        // the length at each end of the band from the sources beside it
        double left = std::numeric_limits<double>::infinity();
        double right = std::numeric_limits<double>::infinity();
        for (std::size_t other = 0; other < band; ++other)
            left = std::min(left, sizes[other] + GRADING_GROWTH * (breaks[band] - breaks[other + 1]));
        for (std::size_t other = band + 1; other < bands; ++other)
            right = std::min(right, sizes[other] + GRADING_GROWTH * (breaks[other] - breaks[band + 1]));
        for (std::size_t point = 0; point <= band; ++point)
            left = std::min(left, points[point] + GRADING_GROWTH * (breaks[band] - breaks[point]));
        for (std::size_t point = band + 1; point < points.size(); ++point)
            right = std::min(right, points[point] + GRADING_GROWTH * (breaks[point] - breaks[band + 1]));
        left = std::max(left, floors[band]);
        right = std::max(right, floors[band]);
        if (left >= own && right >= own)
        {
            const auto count = static_cast<std::size_t>(std::ceil(width / own - 1.0e-9));
            coarse[band] = std::max<std::size_t>(CELLS_PER_THICKNESS, count);
            fine[band] = std::max<std::size_t>(2 * CELLS_PER_THICKNESS,
                                               static_cast<std::size_t>(std::ceil(2.0 * width / own - 1.0e-9)));
            continue;
        }
        const auto lengthAt = [&](double u)
        { return std::min({own, left + GRADING_GROWTH * u, right + GRADING_GROWTH * (width - u)}); };
        // the parts of the band where s is linear: between 0, the ends of the two ramps, where they cross and width
        std::vector<double> ends{0.0, width, (right - left + GRADING_GROWTH * width) / (2.0 * GRADING_GROWTH)};
        if (own < std::numeric_limits<double>::infinity())
            ends.insert(ends.end(), {(own - left) / GRADING_GROWTH, width - (own - right) / GRADING_GROWTH});
        for (double& end : ends)
            end = end == end ? std::clamp(end, 0.0, width) : 0.0;
        std::sort(ends.begin(), ends.end());
        // the integral of 1 / s at each end: log(high / low) / slope over a part where s is linear
        std::vector<double> integral{0.0};
        for (std::size_t part = 1; part < ends.size(); ++part)
        {
            const double length = ends[part] - ends[part - 1];
            const double low = lengthAt(ends[part - 1]);
            const double high = lengthAt(ends[part]);
            double added = 0.0;
            if (length > 0.0)
                added = std::fabs(high - low) <= 1.0e-12 * (low + high) ? length / low
                                                                        : length * std::log(high / low) / (high - low);
            integral.push_back(integral.back() + added);
        }
        const auto count =
            std::max<std::size_t>(CELLS_PER_THICKNESS, static_cast<std::size_t>(std::ceil(integral.back() - 1.0e-9)));
        coarse[band] = count;
        fine[band] = 2 * count;
        std::size_t part = 1;
        for (std::size_t cell = 1; cell < count; ++cell)
        {
            const double target = integral.back() * static_cast<double>(cell) / static_cast<double>(count);
            while (part + 1 < ends.size() && integral[part] < target)
                ++part;
            const double start = ends[part - 1];
            const double end = ends[part];
            const double low = lengthAt(start);
            const double rest = target - integral[part - 1];
            const double slope = end > start ? (lengthAt(end) - low) / (end - start) : 0.0;
            const double place =
                std::fabs(slope) <= 1.0e-12 ? start + rest * low : start + low * (std::exp(slope * rest) - 1.0) / slope;
            cuts[band].push_back(std::clamp(place, start, end) / width);
        }
    }
}

// The graded plan of a union of rectangles. A band runs across a wall (a piece no longer along the band's direction
// than across it) or along walls. Across, its cells are LONG_CELL_RATIO / CELLS_PER_THICKNESS of the thinnest such
// wall's thickness long, as in the uniform plan of that wall alone. Along, they are so long only at the corners of the
// section (where its boundary turns, the thinnest wall touching the corner giving the length), longer away from them
// (gradedBands); never shorter than GRADING_FLOOR of what the walls of the band ask for (those across it, else those
// along it): a flange is not cut to the scale of the thin web it carries. Longer while the mesh would have too many
// lines, as the uniform plan.
SectionMeshPlan gradedPlan(std::span<const SectionRectangle> pieces)
{
    SectionMeshPlan plan;
    std::array<std::vector<double>, 2> breaks;
    breaksOf(pieces, breaks[0], breaks[1]);
    for (const SectionRectangle& piece : pieces)
        plan.edges.push_back({breakIndex(breaks[0], piece.y0), breakIndex(breaks[0], piece.y1),
                              breakIndex(breaks[1], piece.z0), breakIndex(breaks[1], piece.z1)});
    const std::size_t yBands = breaks[0].size() - 1;
    const std::size_t zBands = breaks[1].size() - 1;
    // the thinnest wall across each band and along it, and the corners on each break
    const double none = std::numeric_limits<double>::infinity();
    std::array<std::vector<double>, 2> across;
    std::array<std::vector<double>, 2> along;
    std::array<std::vector<double>, 2> corners;
    for (std::size_t axis = 0; axis < 2; ++axis)
    {
        across[axis].assign(breaks[axis].size() - 1, none);
        along[axis].assign(breaks[axis].size() - 1, none);
        corners[axis].assign(breaks[axis].size(), none);
    }
    std::vector<std::int32_t> cover(yBands * zBands, -1); // the piece covering each cell of bands
    std::vector<double> thickness(pieces.size());
    for (std::size_t index = 0; index < pieces.size(); ++index)
    {
        const SectionRectangle& piece = pieces[index];
        const auto& edges = plan.edges[index];
        if (edges[1] > yBands || edges[3] > zBands)
            return {};
        const std::array<double, 2> extent{piece.y1 - piece.y0, piece.z1 - piece.z0};
        thickness[index] = std::min(extent[0], extent[1]);
        for (std::size_t axis = 0; axis < 2; ++axis)
        {
            auto& wanted = extent[axis] <= extent[1 - axis] ? across[axis] : along[axis];
            for (std::uint32_t band = edges[2 * axis]; band < edges[2 * axis + 1]; ++band)
                wanted[band] = std::min(wanted[band], thickness[index]);
        }
        for (std::uint32_t row = edges[0]; row < edges[1]; ++row)
            for (std::uint32_t column = edges[2]; column < edges[3]; ++column)
                cover[row * zBands + column] = static_cast<std::int32_t>(index);
    }
    // a corner: a point of the breaks where one or three of the four cells around it are covered, or two diagonal ones
    for (std::size_t row = 0; row <= yBands; ++row)
        for (std::size_t column = 0; column <= zBands; ++column)
        {
            std::array<std::int32_t, 4> around{-1, -1, -1, -1}; // (row - 1, column - 1), (row - 1, column), ...
            for (std::size_t corner = 0; corner < 4; ++corner)
            {
                const std::size_t r = row + corner / 2;
                const std::size_t c = column + corner % 2;
                if (r >= 1 && c >= 1 && r - 1 < yBands && c - 1 < zBands)
                    around[corner] = cover[(r - 1) * zBands + (c - 1)];
            }
            const int covered = (around[0] >= 0) + (around[1] >= 0) + (around[2] >= 0) + (around[3] >= 0);
            const bool diagonal = covered == 2 && (around[0] >= 0) == (around[3] >= 0);
            if (covered != 1 && covered != 3 && !diagonal)
                continue;
            double thinnest = none;
            for (const std::int32_t piece : around)
                if (piece >= 0)
                    thinnest = std::min(thinnest, thickness[static_cast<std::size_t>(piece)]);
            corners[0][row] = std::min(corners[0][row], thinnest);
            corners[1][column] = std::min(corners[1][column], thinnest);
        }
    double scale = LONG_CELL_RATIO / static_cast<double>(CELLS_PER_THICKNESS);
    for (int attempt = 0; attempt < 64; ++attempt, scale *= 1.5)
    {
        std::array<std::vector<double>, 2> sizes = across;
        std::array<std::vector<double>, 2> points = corners;
        std::array<std::vector<double>, 2> floors;
        for (std::size_t axis = 0; axis < 2; ++axis)
        {
            for (double& size : sizes[axis])
                size *= scale;
            for (double& point : points[axis])
                point *= scale;
            for (std::size_t band = 0; band < across[axis].size(); ++band)
                floors[axis].push_back(scale * GRADING_FLOOR *
                                       (across[axis][band] < none ? across[axis][band] : along[axis][band]));
        }
        gradedBands(breaks[0], sizes[0], points[0], floors[0], plan.coarseY, plan.fineY, plan.cutsY);
        gradedBands(breaks[1], sizes[1], points[1], floors[1], plan.coarseZ, plan.fineZ, plan.cutsZ);
        std::size_t lines = 2;
        for (const auto* counts : {&plan.coarseY, &plan.coarseZ})
            for (const std::size_t count : *counts)
                lines += count;
        if (lines <= 2 * MAX_CELLS)
            break;
    }
    return plan;
}

// The plan of the meshes of a union of rectangles: the uniform plan, or the graded one when the uniform plan's coarse
// mesh has more than GRADED_PLAN_CELLS cells and the graded one fewer (walls of very different thicknesses, or many
// times longer than thick: a companion package D-577). Every other section keeps the meshes of before, to the bit.
SectionMeshPlan planOfPieces(std::span<const SectionRectangle> pieces)
{
    SectionMeshPlan uniform = uniformPlan(pieces);
    const std::size_t cells = coarseCells(uniform);
    if (cells <= GRADED_PLAN_CELLS)
        return uniform;
    SectionMeshPlan graded = gradedPlan(pieces);
    return coarseCells(graded) < cells ? graded : uniform;
}

// The value of an edge of a piece: y0, y1, z0, z1.
double edgeOf(const SectionRectangle& piece, std::size_t edge)
{
    switch (edge)
    {
    case 0:
        return piece.y0;
    case 1:
        return piece.y1;
    case 2:
        return piece.z0;
    default:
        return piece.z1;
    }
}

// The lines of one direction of a mesh of the plan: for each line, its band and where it is in the band (0 on the
// break that starts the band; the last line is the last break, at 0 in a band past the last).
struct PlanLines
{
    std::vector<std::uint32_t> band;
    std::vector<double> fraction;
};

// `cuts` the lines inside the graded bands of the coarse mesh (empty: none graded); `halved` for the fine mesh, which
// cuts each coarse cell of a graded band in its middle.
PlanLines planLines(const std::vector<std::size_t>& counts, const std::vector<std::vector<double>>& cuts, bool halved)
{
    PlanLines lines;
    for (std::size_t band = 0; band < counts.size(); ++band)
    {
        if (band < cuts.size() && !cuts[band].empty())
        {
            std::vector<double> coarse{0.0};
            coarse.insert(coarse.end(), cuts[band].begin(), cuts[band].end());
            coarse.push_back(1.0);
            for (std::size_t cell = 0; cell + 1 < coarse.size(); ++cell)
            {
                lines.band.push_back(static_cast<std::uint32_t>(band));
                lines.fraction.push_back(coarse[cell]);
                if (halved)
                {
                    lines.band.push_back(static_cast<std::uint32_t>(band));
                    lines.fraction.push_back(0.5 * (coarse[cell] + coarse[cell + 1]));
                }
            }
            continue;
        }
        for (std::size_t cell = 0; cell < counts[band]; ++cell)
        {
            lines.band.push_back(static_cast<std::uint32_t>(band));
            lines.fraction.push_back(static_cast<double>(cell) / static_cast<double>(counts[band]));
        }
    }
    lines.band.push_back(static_cast<std::uint32_t>(counts.size()));
    lines.fraction.push_back(0.0);
    return lines;
}

// The topology of a mesh of a plan: the grid of the plan's bands, `yCounts` and `zCounts` cells in them, its cells in
// the bands a piece covers kept (the quadrilaterals, the lines of each node and the pieces around it). It does not
// depend on where the edges are.
struct PlanTopology
{
    std::vector<std::array<std::uint32_t, 4>> quads;
    std::vector<std::array<std::uint32_t, 2>> places; // the lines (y, z) of each node
    std::vector<std::array<std::int32_t, 4>> around;  // the pieces around each node, -1 past the last
    PlanLines ys;
    PlanLines zs;
};

std::optional<PlanTopology> planTopology(const SectionMeshPlan& plan, std::size_t level)
{
    if (plan.empty())
        return std::nullopt;
    const std::vector<std::size_t>& yCounts = level == 0 ? plan.coarseY : plan.fineY;
    const std::vector<std::size_t>& zCounts = level == 0 ? plan.coarseZ : plan.fineZ;
    // the cuts of the graded bands: none, or one list a band of as many lines as the band has coarse cells less one
    for (const auto& [cuts, counts] : {std::pair{&plan.cutsY, &plan.coarseY}, std::pair{&plan.cutsZ, &plan.coarseZ}})
    {
        if (cuts->empty())
            continue;
        if (cuts->size() != counts->size())
            return std::nullopt;
        for (std::size_t band = 0; band < cuts->size(); ++band)
            if (!(*cuts)[band].empty() && (*cuts)[band].size() + 1 != (*counts)[band])
                return std::nullopt;
    }
    const std::size_t yBands = yCounts.size();
    const std::size_t zBands = zCounts.size();
    // the piece covering each cell of bands, or -1
    std::vector<std::int32_t> cover(yBands * zBands, -1);
    for (std::size_t index = 0; index < plan.edges.size(); ++index)
    {
        const auto& edges = plan.edges[index];
        if (edges[0] > yBands || edges[1] > yBands || edges[2] > zBands || edges[3] > zBands || edges[0] >= edges[1] ||
            edges[2] >= edges[3])
            return std::nullopt;
        for (std::size_t row = edges[0]; row < edges[1]; ++row)
            for (std::size_t column = edges[2]; column < edges[3]; ++column)
                cover[row * zBands + column] = static_cast<std::int32_t>(index);
    }
    PlanTopology topology;
    topology.ys = planLines(yCounts, plan.cutsY, level != 0);
    topology.zs = planLines(zCounts, plan.cutsZ, level != 0);
    const std::size_t columns = topology.zs.band.size();
    std::vector<std::int64_t> nodeOf(topology.ys.band.size() * columns, -1);
    std::vector<std::uint32_t> pieceOf; // of each quad
    const auto nodeAt = [&](std::size_t row, std::size_t column)
    {
        std::int64_t& slot = nodeOf[row * columns + column];
        if (slot < 0)
        {
            slot = static_cast<std::int64_t>(topology.places.size());
            topology.places.push_back({static_cast<std::uint32_t>(row), static_cast<std::uint32_t>(column)});
        }
        return static_cast<std::uint32_t>(slot);
    };
    for (std::size_t row = 0; row + 1 < topology.ys.band.size(); ++row)
        for (std::size_t column = 0; column + 1 < columns; ++column)
        {
            const std::int32_t piece = cover[topology.ys.band[row] * zBands + topology.zs.band[column]];
            if (piece < 0)
                continue;
            topology.quads.push_back(
                {nodeAt(row, column), nodeAt(row + 1, column), nodeAt(row + 1, column + 1), nodeAt(row, column + 1)});
            pieceOf.push_back(static_cast<std::uint32_t>(piece));
        }
    topology.around.assign(topology.places.size(), {-1, -1, -1, -1});
    for (std::size_t quad = 0; quad < topology.quads.size(); ++quad)
        for (const std::uint32_t node : topology.quads[quad])
            for (std::int32_t& slot : topology.around[node])
            {
                if (slot == static_cast<std::int32_t>(pieceOf[quad]))
                    break;
                if (slot < 0)
                {
                    slot = static_cast<std::int32_t>(pieceOf[quad]);
                    break;
                }
            }
    return topology;
}

// The positions of the nodes of a mesh of the plan for the pieces (the pieces of the plan's section, their edges
// moved). Each node follows the edges of the pieces around it: on a break, the edge of a piece it touches there (two
// edges that met in the plan's section may have moved apart: the nodes of each piece follow their own), between two
// breaks the same fraction of the way; a break no piece around the node has an edge on takes the first edge on it.
// The positions are linear in the edges: given the derivatives of the edges, the same gives those of the nodes. False
// when the pieces are not those of the plan, when a node is on two edges that moved apart (a wall meeting another one
// in its middle) or, with `upright`, when a cell turns over.
bool placeNodes(const PlanTopology& topology, const SectionMeshPlan& plan, std::span<const SectionRectangle> pieces,
                std::vector<Point>& nodes, bool upright)
{
    if (pieces.size() != plan.edges.size())
        return false;
    const std::size_t yBands = topology.ys.band.back();
    const std::size_t zBands = topology.zs.band.back();
    std::vector<double> yBreak(yBands + 1, std::numeric_limits<double>::quiet_NaN());
    std::vector<double> zBreak(zBands + 1, std::numeric_limits<double>::quiet_NaN());
    double extent = 0.0;
    for (std::size_t index = 0; index < pieces.size(); ++index)
        for (std::size_t edge = 0; edge < 4; ++edge)
        {
            const double value = edgeOf(pieces[index], edge);
            double& first = (edge < 2 ? yBreak : zBreak)[plan.edges[index][edge]];
            if (first != first)
                first = value;
            extent = std::max(extent, std::fabs(value));
        }
    const double tolerance = 1.0e-12 * (extent > 0.0 ? extent : 1.0);
    bool conflict = false;
    // the value of break `index` along an axis (0 y, 1 z) for a node: the edge on it of a piece around the node
    const auto breakFor = [&](std::uint32_t node, std::size_t axis, std::uint32_t index)
    {
        double value = std::numeric_limits<double>::quiet_NaN();
        for (const std::int32_t piece : topology.around[node])
        {
            if (piece < 0)
                break;
            for (std::size_t side = 0; side < 2; ++side)
            {
                const std::size_t edge = 2 * axis + side;
                if (plan.edges[static_cast<std::size_t>(piece)][edge] != index)
                    continue;
                const double candidate = edgeOf(pieces[static_cast<std::size_t>(piece)], edge);
                if (value != value)
                    value = candidate;
                else if (std::fabs(candidate - value) > tolerance)
                    conflict = true;
            }
        }
        return value == value ? value : (axis == 0 ? yBreak : zBreak)[index];
    };
    nodes.resize(topology.places.size());
    for (std::uint32_t node = 0; node < topology.places.size(); ++node)
        for (std::size_t axis = 0; axis < 2; ++axis)
        {
            const PlanLines& lines = axis == 0 ? topology.ys : topology.zs;
            const std::uint32_t line = topology.places[node][axis];
            const std::uint32_t band = lines.band[line];
            const double fraction = lines.fraction[line];
            const double low = breakFor(node, axis, band);
            nodes[node][axis] = fraction == 0.0 ? low : low + fraction * (breakFor(node, axis, band + 1) - low);
        }
    if (conflict)
        return false;
    if (!upright)
        return true;
    // no cell turned over: each corner turns left
    for (const auto& quad : topology.quads)
        for (std::size_t corner = 0; corner < 4; ++corner)
        {
            const Point& previous = nodes[quad[(corner + 3) % 4]];
            const Point& here = nodes[quad[corner]];
            const Point& next = nodes[quad[(corner + 1) % 4]];
            const double cross =
                (here[0] - previous[0]) * (next[1] - here[1]) - (here[1] - previous[1]) * (next[0] - here[0]);
            if (!(cross > 0.0))
                return false;
        }
    return true;
}

// The mesh of the plan for the pieces (its topology and the positions of its nodes), or nullopt.
std::optional<QuadMesh> planMesh(const SectionMeshPlan& plan, std::span<const SectionRectangle> pieces,
                                 std::size_t level, std::optional<PlanTopology>* kept = nullptr)
{
    std::optional<PlanTopology> topology = planTopology(plan, level);
    if (!topology)
        return std::nullopt;
    QuadMesh mesh;
    if (!placeNodes(*topology, plan, pieces, mesh.nodes, true))
        return std::nullopt;
    mesh.quads = topology->quads;
    if (kept != nullptr)
        *kept = std::move(topology);
    return mesh;
}

// The mesh of the HEXA hexagon: rows across the height, each row cut into the same number of cells across the width
// it has there (a mapped mesh whose cells follow the sloping sides).
QuadMesh hexagonMesh(double inset, double width, double height, int cellsPerThickness)
{
    const std::size_t rows = 4 * static_cast<std::size_t>(cellsPerThickness);
    const std::size_t columns = 4 * static_cast<std::size_t>(cellsPerThickness);
    const double half = width / 2.0;
    QuadMesh mesh;
    for (std::size_t row = 0; row <= rows; ++row)
    {
        const double y = -height / 2.0 + height * static_cast<double>(row) / static_cast<double>(rows);
        const double reach = half - inset * std::abs(y) / (height / 2.0);
        for (std::size_t column = 0; column <= columns; ++column)
            mesh.nodes.push_back(
                {y, -reach + 2.0 * reach * static_cast<double>(column) / static_cast<double>(columns)});
    }
    // The middle row is a node row (rows is even), so the corner of each side is a node.
    const auto node = [columns](std::size_t row, std::size_t column)
    { return static_cast<std::uint32_t>(row * (columns + 1) + column); };
    for (std::size_t row = 0; row < rows; ++row)
    {
        for (std::size_t column = 0; column < columns; ++column)
            mesh.quads.push_back(
                {node(row, column), node(row + 1, column), node(row + 1, column + 1), node(row, column + 1)});
    }
    return mesh;
}

// The reverse Cuthill-McKee order of the nodes: a node's neighbours get numbers close to its own, so the profile of
// the matrix stays narrow.
// PERFORMANCE: on a thin-walled section the profile is about the thickness of a wall instead of the width of the
// section. Measured (the two meshes and the three problems): BOX 2 x 2 x 0.1 in 6 ms instead of 13.5 ms in grid order,
// I 100 x 50 x 0.5 in 21 ms instead of 450 ms, BOX 0.6 x 0.2 x 0.0025 in 34 ms instead of 9.2 s.
std::vector<std::uint32_t> reverseCuthillMcKee(const QuadMesh& mesh)
{
    const std::size_t count = mesh.nodes.size();
    // the neighbours of each node, compressed: those of node n in neighbours[offsets[n] .. offsets[n] + degree[n])
    std::vector<std::uint32_t> offsets(count + 1, 0);
    for (const auto& quad : mesh.quads)
        for (const std::uint32_t node : quad)
            offsets[node + 1] += 3;
    for (std::size_t node = 0; node < count; ++node)
        offsets[node + 1] += offsets[node];
    std::vector<std::uint32_t> neighbours(offsets.back());
    std::vector<std::uint32_t> degree(count, 0);
    for (const auto& quad : mesh.quads)
        for (const std::uint32_t first : quad)
            for (const std::uint32_t second : quad)
            {
                if (first == second)
                    continue;
                const auto begin = neighbours.begin() + offsets[first];
                const auto end = begin + degree[first];
                if (std::find(begin, end, second) == end)
                    neighbours[offsets[first] + degree[first]++] = second;
            }
    for (std::size_t node = 0; node < count; ++node)
        std::sort(neighbours.begin() + offsets[node], neighbours.begin() + offsets[node] + degree[node]);
    std::vector<std::uint32_t> order;
    order.reserve(count);
    std::vector<std::uint8_t> seen(count, 0);
    std::vector<std::uint32_t> next;
    // A breadth-first walk from `start`, the neighbours of a node visited by increasing degree; returns the last node.
    // `visited` is the queue.
    const auto walk = [&](std::uint32_t start, std::vector<std::uint32_t>& visited)
    {
        std::size_t head = visited.size();
        visited.push_back(start);
        seen[start] = 1;
        std::uint32_t last = start;
        while (head < visited.size())
        {
            const std::uint32_t current = visited[head++];
            last = current;
            next.clear();
            for (std::uint32_t index = offsets[current]; index < offsets[current] + degree[current]; ++index)
            {
                const std::uint32_t neighbour = neighbours[index];
                if (seen[neighbour] == 0)
                {
                    seen[neighbour] = 1;
                    next.push_back(neighbour);
                }
            }
            std::stable_sort(next.begin(), next.end(),
                             [&](std::uint32_t left, std::uint32_t right) { return degree[left] < degree[right]; });
            visited.insert(visited.end(), next.begin(), next.end());
        }
        return last;
    };
    std::vector<std::uint32_t> trial;
    for (std::uint32_t node = 0; node < count; ++node)
    {
        if (seen[node] != 0)
            continue;
        // A first walk finds a node far from this one; the order starts from there (a pseudo-peripheral node).
        trial.clear();
        const std::uint32_t far = walk(node, trial);
        for (const std::uint32_t visited : trial)
            seen[visited] = 0;
        walk(far, order);
    }
    std::reverse(order.begin(), order.end());
    return order;
}

class SkylineCholesky
{
public:
    explicit SkylineCholesky(std::vector<std::uint32_t> first) : m_first(std::move(first))
    {
        m_start.resize(m_first.size() + 1, 0);
        for (std::size_t row = 0; row < m_first.size(); ++row)
            m_start[row + 1] = m_start[row] + (row - m_first[row] + 1);
        m_values.assign(m_start.back(), 0.0);
    }

    // Adds `value` to the entry (row, column) of the lower triangle (column <= row, inside the profile).
    void add(std::size_t row, std::size_t column, double value) { at(row, column) += value; }

    // Makes the unknown `row` zero: its equation becomes x = 0 and it leaves the other equations.
    void fix(std::size_t row)
    {
        for (std::size_t column = m_first[row]; column < row; ++column)
            at(row, column) = 0.0;
        for (std::size_t other = row + 1; other < m_first.size(); ++other)
        {
            if (m_first[other] <= row)
                at(other, row) = 0.0;
        }
        at(row, row) = 1.0;
    }

    // L L^T in place; false when the matrix is not positive definite.
    bool factor()
    {
        for (std::size_t row = 0; row < m_first.size(); ++row)
        {
            const std::size_t rowFirst = m_first[row];
            // m_values[rowBase + k] is L(row, k) (the unsigned arithmetic wraps back to the right index).
            const std::size_t rowBase = m_start[row] - rowFirst;
            for (std::size_t column = rowFirst; column < row; ++column)
            {
                const std::size_t from = std::max(rowFirst, static_cast<std::size_t>(m_first[column]));
                const std::size_t columnBase = m_start[column] - m_first[column];
                m_values[rowBase + column] =
                    (m_values[rowBase + column] -
                     dot(&m_values[rowBase + from], &m_values[columnBase + from], column - from)) /
                    m_values[columnBase + column];
            }
            const double diagonal = m_values[rowBase + row] -
                                    dot(&m_values[rowBase + rowFirst], &m_values[rowBase + rowFirst], row - rowFirst);
            if (!(diagonal > 0.0))
                return false;
            m_values[rowBase + row] = std::sqrt(diagonal);
        }
        return true;
    }

    // Solves L L^T x = b in place (after factor()).
    void solve(std::vector<double>& values) const
    {
        const std::size_t count = m_first.size();
        for (std::size_t row = 0; row < count; ++row)
        {
            const std::size_t rowBase = m_start[row] - m_first[row];
            values[row] =
                (values[row] - dot(&m_values[rowBase + m_first[row]], &values[m_first[row]], row - m_first[row])) /
                m_values[rowBase + row];
        }
        for (std::size_t row = count; row-- > 0;)
        {
            const std::size_t rowBase = m_start[row] - m_first[row];
            values[row] /= m_values[rowBase + row];
            for (std::size_t k = m_first[row]; k < row; ++k)
                values[k] -= m_values[rowBase + k] * values[row];
        }
    }

private:
    double& at(std::size_t row, std::size_t column) { return m_values[m_start[row] + (column - m_first[row])]; }

    // The dot product of two runs of `count` values, four sums at a time (the loop vectorises).
    static double dot(const double* left, const double* right, std::size_t count) noexcept
    {
        std::array<double, 4> sums{};
        std::size_t k = 0;
        for (; k + 4 <= count; k += 4)
            for (std::size_t lane = 0; lane < 4; ++lane)
                sums[lane] += left[k + lane] * right[k + lane];
        double sum = (sums[0] + sums[1]) + (sums[2] + sums[3]);
        for (; k < count; ++k)
            sum += left[k] * right[k];
        return sum;
    }

    std::vector<std::uint32_t> m_first;
    std::vector<std::size_t> m_start;
    std::vector<double> m_values;
};

// The values of the four shape functions of a bilinear quadrilateral and their derivatives at one Gauss point, in the
// coordinates of the section.
struct GaussPoint
{
    std::array<double, 4> shape{};
    std::array<double, 4> dy{};
    std::array<double, 4> dz{};
    double y = 0.0;
    double z = 0.0;
    double weight = 0.0; // the Gauss weight (1) times the Jacobian
};

std::array<GaussPoint, 4> gaussPoints(const QuadMesh& mesh, const std::array<std::uint32_t, 4>& quad)
{
    static constexpr std::array<double, 4> XI{-1.0, 1.0, 1.0, -1.0};
    static constexpr std::array<double, 4> ETA{-1.0, -1.0, 1.0, 1.0};
    const double position = 1.0 / std::sqrt(3.0);
    std::array<GaussPoint, 4> points;
    for (std::size_t index = 0; index < 4; ++index)
    {
        const double xi = XI[index] * position;
        const double eta = ETA[index] * position;
        GaussPoint& point = points[index];
        std::array<double, 4> dXi{};
        std::array<double, 4> dEta{};
        double yXi = 0.0;
        double yEta = 0.0;
        double zXi = 0.0;
        double zEta = 0.0;
        for (std::size_t corner = 0; corner < 4; ++corner)
        {
            point.shape[corner] = 0.25 * (1.0 + XI[corner] * xi) * (1.0 + ETA[corner] * eta);
            dXi[corner] = 0.25 * XI[corner] * (1.0 + ETA[corner] * eta);
            dEta[corner] = 0.25 * ETA[corner] * (1.0 + XI[corner] * xi);
            const Point& node = mesh.nodes[quad[corner]];
            point.y += point.shape[corner] * node[0];
            point.z += point.shape[corner] * node[1];
            yXi += dXi[corner] * node[0];
            zXi += dXi[corner] * node[1];
            yEta += dEta[corner] * node[0];
            zEta += dEta[corner] * node[1];
        }
        const double jacobian = yXi * zEta - zXi * yEta;
        for (std::size_t corner = 0; corner < 4; ++corner)
        {
            point.dy[corner] = (zEta * dXi[corner] - zXi * dEta[corner]) / jacobian;
            point.dz[corner] = (-yEta * dXi[corner] + yXi * dEta[corner]) / jacobian;
        }
        point.weight = jacobian;
    }
    return points;
}

// ---- forward-mode derivatives of the integrals along the directions of the geometry ------------------------------

constexpr std::size_t DIRECTIONS = MAX_SECTION_DIRECTIONS;

// The derivatives of the positions of the nodes of a mesh along each direction: (y', z') of each node.
using NodeDerivatives = std::vector<std::array<std::array<double, 2>, DIRECTIONS>>;

using detail::addScaled;
// A number and its derivatives along N directions, over the scalar S: double for the first derivatives, Dual<N> for
// the second ones (forward over forward).
template <std::size_t N, typename S = double>
using Dual = detail::Dual<N, S>;

// The Gauss points of a quadrilateral whose nodes move: the shape functions (constant), their derivatives, the
// position and the weight with their derivatives.
template <std::size_t N, typename S = double>
struct DualGauss
{
    std::array<double, 4> shape{};
    std::array<Dual<N, S>, 4> dy{};
    std::array<Dual<N, S>, 4> dz{};
    Dual<N, S> y;
    Dual<N, S> z;
    Dual<N, S> weight;
};

// The moves of a node along each direction: (y', z').
using NodeMove = std::array<std::array<double, 2>, DIRECTIONS>;

// The coordinate `axis` of a node as the scalar S: the value, and for the second derivatives (S a Dual) its
// derivatives along the outer directions, the same moves as the inner ones.
template <std::size_t N, typename S>
S seeded(double position, const NodeMove& move, std::size_t axis)
{
    if constexpr (std::is_same_v<S, double>)
        return position;
    else
    {
        S value(position);
        for (std::size_t j = 0; j < N; ++j)
            value.d[j] = move[j][axis];
        return value;
    }
}

// The Gauss points of the quadrilateral of these corners moving along the first N directions of `moves`.
template <std::size_t N, typename S = double>
std::array<DualGauss<N, S>, 4> dualGaussPoints(const std::array<const Point*, 4>& corners,
                                               const std::array<const NodeMove*, 4>& moves)
{
    static constexpr std::array<double, 4> XI{-1.0, 1.0, 1.0, -1.0};
    static constexpr std::array<double, 4> ETA{-1.0, -1.0, 1.0, 1.0};
    const double position = 1.0 / std::sqrt(3.0);
    std::array<Dual<N, S>, 4> nodeY;
    std::array<Dual<N, S>, 4> nodeZ;
    for (std::size_t corner = 0; corner < 4; ++corner)
    {
        // the nodes are affine in the geometry: their first derivatives are constants
        nodeY[corner].v = seeded<N, S>((*corners[corner])[0], *moves[corner], 0);
        nodeZ[corner].v = seeded<N, S>((*corners[corner])[1], *moves[corner], 1);
        for (std::size_t i = 0; i < N; ++i)
        {
            nodeY[corner].d[i] = (*moves[corner])[i][0];
            nodeZ[corner].d[i] = (*moves[corner])[i][1];
        }
    }
    std::array<DualGauss<N, S>, 4> points;
    for (std::size_t index = 0; index < 4; ++index)
    {
        const double xi = XI[index] * position;
        const double eta = ETA[index] * position;
        DualGauss<N, S>& point = points[index];
        std::array<double, 4> dXi{};
        std::array<double, 4> dEta{};
        Dual<N, S> yXi;
        Dual<N, S> yEta;
        Dual<N, S> zXi;
        Dual<N, S> zEta;
        for (std::size_t corner = 0; corner < 4; ++corner)
        {
            point.shape[corner] = 0.25 * (1.0 + XI[corner] * xi) * (1.0 + ETA[corner] * eta);
            dXi[corner] = 0.25 * XI[corner] * (1.0 + ETA[corner] * eta);
            dEta[corner] = 0.25 * ETA[corner] * (1.0 + XI[corner] * xi);
            addScaled(point.y, point.shape[corner], nodeY[corner]);
            addScaled(point.z, point.shape[corner], nodeZ[corner]);
            addScaled(yXi, dXi[corner], nodeY[corner]);
            addScaled(zXi, dXi[corner], nodeZ[corner]);
            addScaled(yEta, dEta[corner], nodeY[corner]);
            addScaled(zEta, dEta[corner], nodeZ[corner]);
        }
        const Dual<N, S> jacobian = yXi * zEta - zXi * yEta;
        for (std::size_t corner = 0; corner < 4; ++corner)
        {
            point.dy[corner] = (dXi[corner] * zEta - dEta[corner] * zXi) / jacobian;
            point.dz[corner] = (dEta[corner] * yXi - dXi[corner] * yEta) / jacobian;
        }
        point.weight = jacobian;
    }
    return points;
}

// The same for a quadrilateral of a mesh.
template <std::size_t N>
std::array<DualGauss<N>, 4> dualGaussPoints(const QuadMesh& mesh, const NodeDerivatives& moves,
                                            const std::array<std::uint32_t, 4>& quad)
{
    return dualGaussPoints<N>(
        std::array<const Point*, 4>{&mesh.nodes[quad[0]], &mesh.nodes[quad[1]], &mesh.nodes[quad[2]],
                                    &mesh.nodes[quad[3]]},
        std::array<const NodeMove*, 4>{&moves[quad[0]], &moves[quad[1]], &moves[quad[2]], &moves[quad[3]]});
}

// The nodal fields the derivatives need, by the place of each node in the factorised matrix: the solutions of the
// three problems (shear y, shear z, warping) and the two adjoint solutions.
enum Field : std::uint8_t
{
    ShearY,
    ShearZ,
    Warping,
    AdjointSquare, // K^-1 M u_w: the derivative of the integral of the warping function squared
    AdjointMean,   // K^-1 m: the derivative of its integral
    FieldCount
};

// The products u_a K u_b the results need.
constexpr std::array<std::array<std::size_t, 2>, 7> PAIRS{{{ShearY, ShearY},
                                                           {ShearZ, ShearZ},
                                                           {Warping, Warping},
                                                           {ShearY, Warping},
                                                           {ShearZ, Warping},
                                                           {AdjointSquare, Warping},
                                                           {AdjointMean, Warping}}};

// The derivatives of the results along each direction, as the scalar S of the derivatives.
template <typename S>
struct StiffnessRows
{
    std::array<S, DIRECTIONS> k1{};
    std::array<S, DIRECTIONS> k2{};
    std::array<S, DIRECTIONS> j{};
    std::array<S, DIRECTIONS> shearY{};
    std::array<S, DIRECTIONS> shearZ{};
    std::array<S, DIRECTIONS> cw{};
};

// The integrals over a mesh the results are made of, as numbers D carrying their derivatives along the directions of
// the geometry (the nodal fields held).
template <typename D>
struct MeshIntegrals
{
    // the raw moments of the area
    D area;
    D firstY;
    D firstZ;
    D secondYY;
    D secondZZ;
    D secondYZ;
    // per field: the integrals of v, v y, v z, dv/dy, dv/dz, z dv/dy, y dv/dz
    std::array<std::array<D, 7>, FieldCount> integrals{};
    // the products u_a K u_b of PAIRS
    std::array<D, PAIRS.size()> products{};
    D square; // the integral of the warping function squared

    // Each number, in a fixed order.
    template <typename Visit>
    void each(const Visit& visit)
    {
        for (D* number : {&area, &firstY, &firstZ, &secondYY, &secondZZ, &secondYZ, &square})
            visit(*number);
        for (auto& field : integrals)
            for (D& number : field)
                visit(number);
        for (D& number : products)
            visit(number);
    }
};

// Adds the integrals of one quadrilateral, its Gauss points `points` and its nodal fields `nodal`, to `sums`.
template <std::size_t N, typename S>
void addQuadrilateral(const std::array<DualGauss<N, S>, 4>& points,
                      const std::array<std::array<double, 4>, FieldCount>& nodal, MeshIntegrals<Dual<N, S>>& sums)
{
    using D = Dual<N, S>;
    for (const DualGauss<N, S>& point : points)
    {
        const D& w = point.weight;
        const D yw = point.y * w;
        const D zw = point.z * w;
        sums.area = sums.area + w;
        sums.firstY = sums.firstY + yw;
        sums.firstZ = sums.firstZ + zw;
        sums.secondYY = sums.secondYY + point.y * yw;
        sums.secondZZ = sums.secondZZ + point.z * zw;
        sums.secondYZ = sums.secondYZ + point.y * zw;
        std::array<D, FieldCount> gradientY{};
        std::array<D, FieldCount> gradientZ{};
        for (std::size_t field = 0; field < FieldCount; ++field)
        {
            double value = 0.0;
            for (std::size_t a = 0; a < 4; ++a)
            {
                value += point.shape[a] * nodal[field][a];
                addScaled(gradientY[field], nodal[field][a], point.dy[a]);
                addScaled(gradientZ[field], nodal[field][a], point.dz[a]);
            }
            auto& totals = sums.integrals[field];
            addScaled(totals[0], value, w);
            addScaled(totals[1], value, yw);
            addScaled(totals[2], value, zw);
            const D gyw = gradientY[field] * w;
            const D gzw = gradientZ[field] * w;
            totals[3] = totals[3] + gyw;
            totals[4] = totals[4] + gzw;
            totals[5] = totals[5] + point.z * gyw;
            totals[6] = totals[6] + point.y * gzw;
            if (field == Warping)
                addScaled(sums.square, value * value, w);
        }
        for (std::size_t pair = 0; pair < PAIRS.size(); ++pair)
        {
            const auto [first, second] = PAIRS[pair];
            sums.products[pair] =
                sums.products[pair] + (gradientY[first] * gradientY[second] + gradientZ[first] * gradientZ[second]) * w;
        }
    }
}

// The nodal fields of a quadrilateral.
std::array<std::array<double, 4>, FieldCount> nodalOf(const std::array<std::vector<double>, FieldCount>& fields,
                                                      const std::vector<std::uint32_t>& position,
                                                      const std::array<std::uint32_t, 4>& quad)
{
    std::array<std::array<double, 4>, FieldCount> nodal{};
    for (std::size_t field = 0; field < FieldCount; ++field)
        for (std::size_t a = 0; a < 4; ++a)
            nodal[field][a] = fields[field][position[quad[a]]];
    return nodal;
}

// The Lagrangians the results are made of, by the index `compose` gives its hook: the three energies, the products
// of the warping function with y and z, the integral of its square and its integral.
enum Lagrangian : std::uint8_t
{
    EnergyY,
    EnergyZ,
    EnergyW,
    WarpingY,
    WarpingZ,
    WarpingSquared,
    WarpingMean,
    LagrangianCount
};

// The results and their derivatives from the integrals. With K u_p = f_p (the row of the fixed unknown taken out),
// the energy E_p = f_p . u_p has E_p' = 2 f_p' . u_p - u_p K' u_p; the integral of the warping function times y - yc
// is f_0 . u_w (u_0 K u_w), whose derivative is f_0' . u_w + f_w' . u_0 - u_0 K' u_w; an integral g . u_w of the
// warping function has the derivative g' . u_w + a . (f_w' - K' u_w) with K a = g (the adjoint). So the derivatives
// need, along each direction, the derivatives of integrals of fixed nodal fields over the moving mesh only: no solve
// per direction. Each is the derivative of a Lagrangian L(x, u, a), the solutions u and adjoints a held, equal to the
// result when they are the solutions and stationary in both. With second derivatives (S a Dual, a companion package) a
// Lagrangian's value is its own (the result's to rounding) and `hook(index, number)` adds to its second derivatives
// what the fields held leave out (hessianOn); without, its value is the result's.
template <std::size_t N, typename S, typename Hook>
void compose(const MeshIntegrals<Dual<N, S>>& sums, const std::array<double, 3>& energies, std::size_t count,
             const Hook& hook, StiffnessRows<S>& rows)
{
    using D = Dual<N, S>;
    const auto& integrals = sums.integrals;
    const auto& products = sums.products;
    const D centroidY = sums.firstY / sums.area;
    const D centroidZ = sums.firstZ / sums.area;
    const D inertiaY = sums.secondYY - sums.firstY * centroidY;
    const D inertiaZ = sums.secondZZ - sums.firstZ * centroidZ;
    const D inertiaYZ = sums.secondYZ - sums.firstY * centroidZ;
    // the loads dotted with a fixed field, f_p . v, as functions of the geometry
    const auto load = [&](std::size_t problem, std::size_t field)
    {
        const auto& fieldSums = integrals[field];
        if (problem == ShearY)
            return fieldSums[1] - centroidY * fieldSums[0];
        if (problem == ShearZ)
            return fieldSums[2] - centroidZ * fieldSums[0];
        return fieldSums[5] - centroidZ * fieldSums[3] - fieldSums[6] + centroidY * fieldSums[4];
    };
    // a value whose derivative is the sum of the derivatives of the terms (with the second derivatives, the sum of
    // the terms)
    const auto combined = [](double value, std::initializer_list<std::pair<double, const D*>> terms)
    {
        D result;
        if constexpr (std::is_same_v<S, double>)
        {
            result.v = value;
            for (const auto& [factor, term] : terms)
                for (std::size_t i = 0; i < N; ++i)
                    result.d[i] += factor * term->d[i];
        }
        else
            for (const auto& [factor, term] : terms)
                addScaled(result, factor, *term);
        return result;
    };
    std::array<D, 3> energy;
    std::array<D, 3> loads{};
    for (std::size_t problem = 0; problem < 3; ++problem)
    {
        loads[problem] = load(problem, problem);
        energy[problem] = combined(energies[problem], {{2.0, &loads[problem]}, {-1.0, &products[problem]}});
        hook(problem, energy[problem]);
    }
    const D shearYWarping = load(ShearY, Warping);
    const D warpingShearY = load(Warping, ShearY);
    const D shearZWarping = load(ShearZ, Warping);
    const D warpingShearZ = load(Warping, ShearZ);
    D warpingY = combined(valueOf(shearYWarping), {{1.0, &shearYWarping}, {1.0, &warpingShearY}, {-1.0, &products[3]}});
    D warpingZ = combined(valueOf(shearZWarping), {{1.0, &shearZWarping}, {1.0, &warpingShearZ}, {-1.0, &products[4]}});
    hook(WarpingY, warpingY);
    hook(WarpingZ, warpingZ);
    const D warpingSquare = load(Warping, AdjointSquare);
    const D warpingMean = load(Warping, AdjointMean);
    D squared = combined(valueOf(sums.square), {{1.0, &sums.square}, {2.0, &warpingSquare}, {-2.0, &products[5]}});
    D mean = combined(valueOf(integrals[Warping][0]),
                      {{1.0, &integrals[Warping][0]}, {1.0, &warpingMean}, {-1.0, &products[6]}});
    hook(WarpingSquared, squared);
    hook(WarpingMean, mean);
    const D k1 = inertiaY * inertiaY / (sums.area * energy[0]);
    const D k2 = inertiaZ * inertiaZ / (sums.area * energy[1]);
    const D torsion = inertiaY + inertiaZ - energy[2];
    const D determinant = inertiaY * inertiaZ - inertiaYZ * inertiaYZ;
    const D poleY = (warpingY * inertiaYZ - inertiaY * warpingZ) / determinant;
    const D poleZ = (inertiaZ * warpingY - inertiaYZ * warpingZ) / determinant;
    const D about = squared + 2.0 * (poleY * warpingZ) - 2.0 * (poleZ * warpingY) + poleY * poleY * inertiaZ -
                    2.0 * (poleY * poleZ * inertiaYZ) + poleZ * poleZ * inertiaY;
    const D cw = about - mean * mean / sums.area;
    for (std::size_t i = 0; i < count; ++i)
    {
        rows.k1[i] = k1.d[i];
        rows.k2[i] = k2.d[i];
        rows.j[i] = torsion.d[i];
        rows.shearY[i] = centroidY.d[i] + poleY.d[i];
        rows.shearZ[i] = centroidZ.d[i] + poleZ.d[i];
        rows.cw[i] = cw.d[i];
    }
}

// The derivatives of the results on one mesh in `gradient` (NaN along a direction not `valid`).
template <std::size_t N>
void gradientOn(const QuadMesh& mesh, const NodeDerivatives& moves, const std::vector<std::uint32_t>& position,
                const std::array<std::vector<double>, FieldCount>& fields, const std::array<double, 3>& energies,
                std::size_t count, const std::array<std::uint8_t, DIRECTIONS>* valid,
                SectionStiffnessGradient& gradient)
{
    MeshIntegrals<Dual<N>> sums;
    for (const auto& quad : mesh.quads)
    {
        const auto nodal = nodalOf(fields, position, quad);
        addQuadrilateral<N, double>(dualGaussPoints<N>(mesh, moves, quad), nodal, sums);
    }
    StiffnessRows<double> rows;
    compose<N, double>(sums, energies, count, [](std::size_t, const Dual<N>&) {}, rows);
    gradient.count = count;
    for (std::size_t i = 0; i < count; ++i)
    {
        const bool ok = valid == nullptr || (*valid)[i] != 0;
        const double nan = SectionStiffnessGradient::UNDEFINED;
        gradient.k1[i] = ok ? rows.k1[i] : nan;
        gradient.k2[i] = ok ? rows.k2[i] : nan;
        gradient.j[i] = ok ? rows.j[i] : nan;
        gradient.shearY[i] = ok ? rows.shearY[i] : nan;
        gradient.shearZ[i] = ok ? rows.shearZ[i] : nan;
        gradient.cw[i] = ok ? rows.cw[i] : nan;
    }
}

// ---- second derivatives -------------------------------------------------------------------------
//
// Each result Q is made of Lagrangians L(x, u, a) (compose), u a solution of K u = f and a an adjoint, L stationary in
// both. The second derivative of such a quantity is the second-order adjoint formula
//   d2 Q / dx_i dx_j = L_ij + L_iu . u_j + L_ju . u_i + u_i . L_uu . u_j,
// L_ij its second derivatives with u and a held, L_iu = r_i the derivative of its residual with u held (f_i - K_i a
// for the energies and the products, M_i u_w - K_i a for the warping squared, ...), u_j = K^-1 (f_j - K_j u) the
// derivative of the solution, and L_uu = 2 M for the warping squared (0 for the others). L_ij comes from the integrals
// of the fields held, forward over forward (each quadrilateral along the directions that move it, a few of them, so
// the cost is that of the gradient a few times over); the rest from three solves a direction and dot products of
// nodal vectors.

// The nodal vectors of the second derivatives, a direction each: the residuals r of the five fields (f_i - K_i u for
// the solutions, M_i u_w - K_i a and m_i - K_i a for the adjoints of the warping squared and of its integral), the
// derivatives of the three solutions K^-1 r, and M times that of the warping function.
struct FieldRates
{
    std::array<std::vector<std::vector<double>>, FieldCount> residual;
    std::array<std::vector<std::vector<double>>, 3> solution;
    std::vector<std::vector<double>> massWarping;
};

template <std::size_t N>
FieldRates fieldRates(const QuadMesh& mesh, const NodeDerivatives& moves, const std::vector<std::uint32_t>& position,
                      const std::array<std::vector<double>, FieldCount>& fields, const SkylineCholesky& matrix,
                      std::size_t count)
{
    using D = Dual<N>;
    const std::size_t nodeCount = mesh.nodes.size();
    D area;
    D firstY;
    D firstZ;
    for (const auto& quad : mesh.quads)
        for (const DualGauss<N>& point : dualGaussPoints<N>(mesh, moves, quad))
        {
            area = area + point.weight;
            firstY = firstY + point.y * point.weight;
            firstZ = firstZ + point.z * point.weight;
        }
    const D centroidY = firstY / area;
    const D centroidZ = firstZ / area;
    FieldRates rates;
    for (auto& field : rates.residual)
        field.assign(count, std::vector<double>(nodeCount, 0.0));
    for (const auto& quad : mesh.quads)
    {
        const auto nodal = nodalOf(fields, position, quad);
        for (const DualGauss<N>& point : dualGaussPoints<N>(mesh, moves, quad))
        {
            const D& w = point.weight;
            const D y = point.y - centroidY;
            const D z = point.z - centroidZ;
            double warping = 0.0;
            for (std::size_t a = 0; a < 4; ++a)
                warping += point.shape[a] * nodal[Warping][a];
            // the right-hand sides of the five fields at each corner, and the stiffness between the corners
            std::array<std::array<D, 4>, FieldCount> loads{};
            std::array<std::array<D, 4>, 4> stiffness{};
            for (std::size_t a = 0; a < 4; ++a)
            {
                loads[ShearY][a] = point.shape[a] * (y * w);
                loads[ShearZ][a] = point.shape[a] * (z * w);
                loads[Warping][a] = (z * point.dy[a] - y * point.dz[a]) * w;
                loads[AdjointSquare][a] = (point.shape[a] * warping) * w;
                loads[AdjointMean][a] = point.shape[a] * w;
                for (std::size_t b = a; b < 4; ++b)
                {
                    stiffness[a][b] = (point.dy[a] * point.dy[b] + point.dz[a] * point.dz[b]) * w;
                    stiffness[b][a] = stiffness[a][b];
                }
            }
            for (std::size_t a = 0; a < 4; ++a)
            {
                const std::uint32_t row = position[quad[a]];
                for (std::size_t field = 0; field < FieldCount; ++field)
                    for (std::size_t i = 0; i < count; ++i)
                    {
                        double rate = loads[field][a].d[i];
                        for (std::size_t b = 0; b < 4; ++b)
                            rate -= stiffness[a][b].d[i] * nodal[field][b];
                        rates.residual[field][i][row] += rate;
                    }
            }
        }
    }
    // the fixed unknown has no residual
    for (auto& field : rates.residual)
        for (std::vector<double>& rate : field)
            rate[0] = 0.0;
    for (std::size_t problem = 0; problem < 3; ++problem)
    {
        rates.solution[problem] = rates.residual[problem];
        for (std::vector<double>& rate : rates.solution[problem])
            matrix.solve(rate);
    }
    rates.massWarping.assign(count, std::vector<double>(nodeCount, 0.0));
    for (const auto& quad : mesh.quads)
        for (const GaussPoint& point : gaussPoints(mesh, quad))
            for (std::size_t i = 0; i < count; ++i)
            {
                double value = 0.0;
                for (std::size_t b = 0; b < 4; ++b)
                    value += point.shape[b] * rates.solution[Warping][i][position[quad[b]]];
                for (std::size_t a = 0; a < 4; ++a)
                    rates.massWarping[i][position[quad[a]]] += point.shape[a] * value * point.weight;
            }
    return rates;
}

// The integrals of one quadrilateral with their second derivatives along the `moving` directions that move its
// nodes (`count` of them, at most C), added to the integrals of the mesh along all its directions.
template <std::size_t C, std::size_t N>
void addMovingQuadrilateral(const QuadMesh& mesh, const NodeDerivatives& moves,
                            const std::array<std::uint32_t, 4>& quad, const std::array<std::size_t, DIRECTIONS>& moving,
                            std::size_t count, const std::array<std::array<double, 4>, FieldCount>& nodal,
                            MeshIntegrals<Dual<N, Dual<N>>>& sums)
{
    using L = Dual<C, Dual<C>>;
    std::array<NodeMove, 4> local{};
    for (std::size_t corner = 0; corner < 4; ++corner)
        for (std::size_t b = 0; b < count; ++b)
            local[corner][b] = moves[quad[corner]][moving[b]];
    MeshIntegrals<L> part;
    addQuadrilateral<C, Dual<C>>(
        dualGaussPoints<C, Dual<C>>(std::array<const Point*, 4>{&mesh.nodes[quad[0]], &mesh.nodes[quad[1]],
                                                                &mesh.nodes[quad[2]], &mesh.nodes[quad[3]]},
                                    std::array<const NodeMove*, 4>{&local[0], &local[1], &local[2], &local[3]}),
        nodal, part);
    // the local directions to the directions of the mesh
    std::array<Dual<N, Dual<N>>*, 7 + 7 * FieldCount + PAIRS.size()> targets{};
    std::size_t at = 0;
    sums.each([&](Dual<N, Dual<N>>& number) { targets[at++] = &number; });
    at = 0;
    part.each(
        [&](const L& number)
        {
            Dual<N, Dual<N>>& target = *targets[at++];
            target.v.v += number.v.v;
            for (std::size_t b = 0; b < count; ++b)
            {
                target.v.d[moving[b]] += number.v.d[b];
                target.d[moving[b]].v += number.d[b].v;
                for (std::size_t c = 0; c < count; ++c)
                    target.d[moving[b]].d[moving[c]] += number.d[b].d[c];
            }
        });
}

// The second derivatives of the results on one mesh, symmetrised (the two orders agree to
// rounding), NaN along a direction not `valid`.
template <std::size_t N>
void hessianOn(const QuadMesh& mesh, const NodeDerivatives& moves, const std::vector<std::uint32_t>& position,
               const std::array<std::vector<double>, FieldCount>& fields, const SkylineCholesky& matrix,
               std::size_t count, const std::array<std::uint8_t, DIRECTIONS>* valid, SectionStiffnessHessian& hessian)
{
    using G = Dual<N, Dual<N>>;
    const FieldRates rates = fieldRates<N>(mesh, moves, position, fields, matrix, count);
    // L_ij: the integrals with the fields held, each quadrilateral along the directions that move one of its nodes
    const auto sums = std::make_unique<MeshIntegrals<G>>();
    for (const auto& quad : mesh.quads)
    {
        std::array<std::size_t, DIRECTIONS> moving{};
        std::size_t movingCount = 0;
        for (std::size_t i = 0; i < count; ++i)
        {
            bool moved = false;
            for (const std::uint32_t node : quad)
                moved = moved || moves[node][i][0] != 0.0 || moves[node][i][1] != 0.0;
            if (moved)
                moving[movingCount++] = i;
        }
        const auto nodal = nodalOf(fields, position, quad);
        if (movingCount <= 2)
            addMovingQuadrilateral<2, N>(mesh, moves, quad, moving, movingCount, nodal, *sums);
        else if (movingCount <= 4)
            addMovingQuadrilateral<4, N>(mesh, moves, quad, moving, movingCount, nodal, *sums);
        else if (movingCount <= 6)
            addMovingQuadrilateral<6, N>(mesh, moves, quad, moving, movingCount, nodal, *sums);
        else if (movingCount <= 8)
            addMovingQuadrilateral<8, N>(mesh, moves, quad, moving, movingCount, nodal, *sums);
        else
            addMovingQuadrilateral<DIRECTIONS, N>(mesh, moves, quad, moving, movingCount, nodal, *sums);
    }
    // the terms of the fields that move: r_a,i . s_b,j and s_w,i . M s_w,j
    const auto dot = [](const std::vector<double>& first, const std::vector<double>& second)
    {
        double sum = 0.0;
        for (std::size_t index = 0; index < first.size(); ++index)
            sum += first[index] * second[index];
        return sum;
    };
    const auto& r = rates.residual;
    const auto& s = rates.solution;
    std::array<std::vector<double>, LagrangianCount> terms;
    for (auto& term : terms)
        term.assign(N * N, 0.0);
    for (std::size_t i = 0; i < count; ++i)
        for (std::size_t j = 0; j < count; ++j)
        {
            const std::size_t entry = i * N + j;
            for (std::size_t problem = 0; problem < 3; ++problem)
                terms[problem][entry] = dot(r[problem][i], s[problem][j]) + dot(r[problem][j], s[problem][i]);
            terms[WarpingY][entry] = dot(r[ShearY][i], s[Warping][j]) + dot(r[ShearY][j], s[Warping][i]);
            terms[WarpingZ][entry] = dot(r[ShearZ][i], s[Warping][j]) + dot(r[ShearZ][j], s[Warping][i]);
            terms[WarpingSquared][entry] =
                2.0 * (dot(r[AdjointSquare][i], s[Warping][j]) + dot(r[AdjointSquare][j], s[Warping][i]) +
                       dot(s[Warping][i], rates.massWarping[j]));
            terms[WarpingMean][entry] = dot(r[AdjointMean][i], s[Warping][j]) + dot(r[AdjointMean][j], s[Warping][i]);
        }
    const auto hook = [&](std::size_t index, G& number)
    {
        for (std::size_t i = 0; i < count; ++i)
            for (std::size_t j = 0; j < count; ++j)
                number.d[i].d[j] += terms[index][i * N + j];
    };
    const auto rows = std::make_unique<StiffnessRows<Dual<N>>>();
    compose<N, Dual<N>>(*sums, {}, count, hook, *rows);
    hessian.count = count;
    const auto fill = [&](const std::array<Dual<N>, DIRECTIONS>& row, SectionStiffnessHessian::Matrix& out)
    {
        for (std::size_t i = 0; i < count; ++i)
            for (std::size_t j = 0; j < count; ++j)
            {
                const bool ok = valid == nullptr || ((*valid)[i] != 0 && (*valid)[j] != 0);
                out[i * DIRECTIONS + j] = ok ? 0.5 * (row[i].d[j] + row[j].d[i]) : SectionStiffnessHessian::UNDEFINED;
            }
    };
    fill(rows->k1, hessian.k1);
    fill(rows->k2, hessian.k2);
    fill(rows->j, hessian.j);
    fill(rows->shearY, hessian.shearY);
    fill(rows->shearZ, hessian.shearZ);
    fill(rows->cw, hessian.cw);
}

// The three problems on one mesh; with `moves`, the derivatives of the results along their directions (`count` of
// them, those `valid` 1) in `gradient`.
SectionStiffness solveOn(const QuadMesh& mesh, const NodeDerivatives* moves = nullptr, std::size_t count = 0,
                         const std::array<std::uint8_t, DIRECTIONS>* valid = nullptr,
                         SectionStiffnessGradient* gradient = nullptr, SectionStiffnessHessian* hessian = nullptr)
{
    const std::size_t nodeCount = mesh.nodes.size();
    if (nodeCount == 0)
        return {};
    // The centroid, then everything about it.
    double area = 0.0;
    double firstY = 0.0;
    double firstZ = 0.0;
    for (const auto& quad : mesh.quads)
    {
        for (const GaussPoint& point : gaussPoints(mesh, quad))
        {
            area += point.weight;
            firstY += point.y * point.weight;
            firstZ += point.z * point.weight;
        }
    }
    if (!(area > 0.0))
        return {};
    const double centroidY = firstY / area;
    const double centroidZ = firstZ / area;

    const std::vector<std::uint32_t> order = reverseCuthillMcKee(mesh);
    std::vector<std::uint32_t> position(nodeCount);
    for (std::uint32_t index = 0; index < nodeCount; ++index)
        position[order[index]] = index;
    std::vector<std::uint32_t> first(nodeCount);
    for (std::uint32_t index = 0; index < nodeCount; ++index)
        first[index] = index;
    for (const auto& quad : mesh.quads)
    {
        std::uint32_t lowest = position[quad[0]];
        for (const std::uint32_t node : quad)
            lowest = std::min(lowest, position[node]);
        for (const std::uint32_t node : quad)
            first[position[node]] = std::min(first[position[node]], lowest);
    }
    SkylineCholesky matrix(first);
    std::array<std::vector<double>, 3> loads{std::vector<double>(nodeCount, 0.0), std::vector<double>(nodeCount, 0.0),
                                             std::vector<double>(nodeCount, 0.0)}; // shear y, shear z, warping
    double inertiaY = 0.0;                                                         // I1, the integral of (y - yc)^2
    double inertiaZ = 0.0;
    double inertiaYZ = 0.0;
    for (const auto& quad : mesh.quads)
    {
        for (const GaussPoint& point : gaussPoints(mesh, quad))
        {
            const double y = point.y - centroidY;
            const double z = point.z - centroidZ;
            inertiaY += y * y * point.weight;
            inertiaZ += z * z * point.weight;
            inertiaYZ += y * z * point.weight;
            for (std::size_t a = 0; a < 4; ++a)
            {
                const std::uint32_t row = position[quad[a]];
                loads[0][row] += point.shape[a] * y * point.weight;
                loads[1][row] += point.shape[a] * z * point.weight;
                loads[2][row] += (z * point.dy[a] - y * point.dz[a]) * point.weight;
                for (std::size_t b = 0; b < 4; ++b)
                {
                    const std::uint32_t column = position[quad[b]];
                    if (column <= row)
                        matrix.add(row, column, (point.dy[a] * point.dy[b] + point.dz[a] * point.dz[b]) * point.weight);
                }
            }
        }
    }
    // The three problems have a zero normal derivative on the whole boundary: their solutions are known up to a
    // constant, which one fixed unknown removes (the right-hand sides add up to zero, so the problems are consistent,
    // and the integrals below do not depend on the constant).
    matrix.fix(0);
    if (!matrix.factor())
        return {};
    std::array<std::vector<double>, FieldCount> solutions;
    std::array<double, 3> energies{};
    for (std::size_t problem = 0; problem < 3; ++problem)
    {
        solutions[problem] = loads[problem];
        solutions[problem][0] = 0.0;
        matrix.solve(solutions[problem]);
        double sum = 0.0;
        for (std::size_t index = 0; index < nodeCount; ++index)
            sum += loads[problem][index] * solutions[problem][index];
        energies[problem] = sum;
    }
    SectionStiffness result;
    result.k1 = inertiaY * inertiaY / (area * energies[0]);
    result.k2 = inertiaZ * inertiaZ / (area * energies[1]);
    result.j = inertiaY + inertiaZ - energies[2];
    // the warping function (by the place of each node) at the Gauss points: its products with y, z
    const std::vector<double>& solution = solutions[2];
    double warpingY = 0.0;
    double warpingZ = 0.0;
    for (const auto& quad : mesh.quads)
    {
        for (const GaussPoint& point : gaussPoints(mesh, quad))
        {
            double value = 0.0;
            for (std::size_t a = 0; a < 4; ++a)
                value += point.shape[a] * solution[position[quad[a]]];
            warpingY += value * (point.y - centroidY) * point.weight;
            warpingZ += value * (point.z - centroidZ) * point.weight;
        }
    }
    // [Iyz -Iyy; Izz -Iyz] [a; b] = [-warpingY; -warpingZ]
    const double determinant = inertiaY * inertiaZ - inertiaYZ * inertiaYZ;
    const double poleY = (warpingY * inertiaYZ - inertiaY * warpingZ) / determinant;
    const double poleZ = (inertiaZ * warpingY - inertiaYZ * warpingZ) / determinant;
    result.shearY = centroidY + poleY;
    result.shearZ = centroidZ + poleZ;
    double sum = 0.0;
    double square = 0.0;
    // with the derivatives: M u_w and m, the right-hand sides of the adjoint problems
    const bool derive = gradient != nullptr && moves != nullptr && count > 0;
    if (derive)
    {
        solutions[AdjointSquare].assign(nodeCount, 0.0);
        solutions[AdjointMean].assign(nodeCount, 0.0);
    }
    for (const auto& quad : mesh.quads)
    {
        for (const GaussPoint& point : gaussPoints(mesh, quad))
        {
            double warping = 0.0;
            for (std::size_t a = 0; a < 4; ++a)
                warping += point.shape[a] * solution[position[quad[a]]];
            const double value = warping + poleY * (point.z - centroidZ) - poleZ * (point.y - centroidY);
            sum += value * point.weight;
            square += value * value * point.weight;
            if (derive)
                for (std::size_t a = 0; a < 4; ++a)
                {
                    solutions[AdjointSquare][position[quad[a]]] += point.shape[a] * warping * point.weight;
                    solutions[AdjointMean][position[quad[a]]] += point.shape[a] * point.weight;
                }
        }
    }
    result.cw = square - sum * sum / area;
    if (!derive)
        return result;
    for (const std::size_t adjoint : {AdjointSquare, AdjointMean})
    {
        solutions[adjoint][0] = 0.0;
        matrix.solve(solutions[adjoint]);
    }
    if (count <= 4)
        gradientOn<4>(mesh, *moves, position, solutions, energies, count, valid, *gradient);
    else if (count <= 6)
        gradientOn<6>(mesh, *moves, position, solutions, energies, count, valid, *gradient);
    else if (count <= 8)
        gradientOn<8>(mesh, *moves, position, solutions, energies, count, valid, *gradient);
    else
        gradientOn<DIRECTIONS>(mesh, *moves, position, solutions, energies, count, valid, *gradient);
    if (hessian == nullptr)
        return result;
    if (count <= 4)
        hessianOn<4>(mesh, *moves, position, solutions, matrix, count, valid, *hessian);
    else if (count <= 6)
        hessianOn<6>(mesh, *moves, position, solutions, matrix, count, valid, *hessian);
    else if (count <= 8)
        hessianOn<8>(mesh, *moves, position, solutions, matrix, count, valid, *hessian);
    else
        hessianOn<DIRECTIONS>(mesh, *moves, position, solutions, matrix, count, valid, *hessian);
    return result;
}

// Richardson's extrapolation of the results of a mesh and of the mesh twice as fine, whose error falls as h^p:
// R = fine + (fine - coarse) / (2^p - 1), the factor 1 / (2^p - 1) given.
// - The uniform meshes of a union of rectangles with a re-entrant corner (the 270 degrees of material where a flange
//   meets a web): p = 4/3. Such a corner makes the warping and the shear functions singular, as r^(2/3) at a distance
//   r from it (the Laplace problem in a wedge of 3 pi / 2; Strang and Fix, An Analysis of the Finite Element Method,
//   section 8.2), and the error of the energies then falls as h^(4/3), not h^2, on meshes refined uniformly. Measured
//   on 2026-10-22 against values converged on graded meshes (Aitken's limit of meshes 2, 4 and 8 times as fine, the
//   same to 1e-5 under either exponent) for I, T, T2, Z, L, CHAN, CHAN1, BOX, BOX1, HAT and CROSS: with p = 2 the error
//   reached 2.5e-3 of J (a CROSS; 1.2e-3 on an I of one thickness), 1.1e-3 of K1, 1.5e-3 of K2, 1.9e-3 of Cw (a BOX);
//   with p = 4/3 at most 3.1e-4 of J, 5e-5 of K1, 1.4e-4 of K2, 2.2e-4 of Cw, on the same meshes.
// - A section with no re-entrant corner (a rectangle, the hexagon) and the graded meshes (cells as short as across the
//   walls at every corner): p = 2, the error of a smooth solution (with p = 4/3 the K2 of a rectangle 20 times longer
//   than thick was 5.6e-3 off; on the graded meshes p = 4/3 doubled the error of a thin web's J).
constexpr double UNIFORM_RICHARDSON = 0.657963087177503; // 1 / (2^(4/3) - 1)
constexpr double SMOOTH_RICHARDSON = 1.0 / 3.0;          // 1 / (2^2 - 1)

// True when the union of the rectangles of a plan has a re-entrant corner: a point of the breaks where three of the
// four cells of bands around it are covered (or two diagonal ones).
bool hasReentrantCorner(const SectionMeshPlan& plan)
{
    const std::size_t yBands = plan.coarseY.size();
    const std::size_t zBands = plan.coarseZ.size();
    std::vector<std::uint8_t> cover(yBands * zBands, 0);
    for (const auto& edges : plan.edges)
        for (std::uint32_t row = edges[0]; row < edges[1] && row < yBands; ++row)
            for (std::uint32_t column = edges[2]; column < edges[3] && column < zBands; ++column)
                cover[row * zBands + column] = 1;
    const auto covered = [&](std::size_t row, std::size_t column)
    { return row >= 1 && column >= 1 && row <= yBands && column <= zBands && cover[(row - 1) * zBands + column - 1]; };
    for (std::size_t row = 0; row <= yBands; ++row)
        for (std::size_t column = 0; column <= zBands; ++column)
        {
            const bool a = covered(row, column);
            const bool b = covered(row, column + 1);
            const bool c = covered(row + 1, column);
            const bool d = covered(row + 1, column + 1);
            const int count = int{a} + int{b} + int{c} + int{d};
            if (count == 3 || (count == 2 && a == d))
                return true;
        }
    return false;
}

double richardson(double first, double second, double factor)
{
    return second + (second - first) * factor;
}

// The result of the coarse and the fine mesh extrapolated (`factor`: richardson).
SectionStiffness extrapolated(const SectionStiffness& coarse, const SectionStiffness& fine, double factor)
{
    return SectionStiffness{
        richardson(coarse.k1, fine.k1, factor),         richardson(coarse.k2, fine.k2, factor),
        richardson(coarse.j, fine.j, factor),           richardson(coarse.shearY, fine.shearY, factor),
        richardson(coarse.shearZ, fine.shearZ, factor), richardson(coarse.cw, fine.cw, factor)};
}

SectionStiffnessHessian extrapolated(const SectionStiffnessHessian& coarse, const SectionStiffnessHessian& fine,
                                     double factor)
{
    SectionStiffnessHessian result;
    result.count = fine.count;
    for (std::size_t entry = 0; entry < DIRECTIONS * DIRECTIONS; ++entry)
    {
        result.k1[entry] = richardson(coarse.k1[entry], fine.k1[entry], factor);
        result.k2[entry] = richardson(coarse.k2[entry], fine.k2[entry], factor);
        result.j[entry] = richardson(coarse.j[entry], fine.j[entry], factor);
        result.shearY[entry] = richardson(coarse.shearY[entry], fine.shearY[entry], factor);
        result.shearZ[entry] = richardson(coarse.shearZ[entry], fine.shearZ[entry], factor);
        result.cw[entry] = richardson(coarse.cw[entry], fine.cw[entry], factor);
    }
    return result;
}

SectionStiffnessGradient extrapolated(const SectionStiffnessGradient& coarse, const SectionStiffnessGradient& fine,
                                      double factor)
{
    SectionStiffnessGradient result;
    result.count = fine.count;
    for (std::size_t i = 0; i < fine.count; ++i)
    {
        result.k1[i] = richardson(coarse.k1[i], fine.k1[i], factor);
        result.k2[i] = richardson(coarse.k2[i], fine.k2[i], factor);
        result.j[i] = richardson(coarse.j[i], fine.j[i], factor);
        result.shearY[i] = richardson(coarse.shearY[i], fine.shearY[i], factor);
        result.shearZ[i] = richardson(coarse.shearZ[i], fine.shearZ[i], factor);
        result.cw[i] = richardson(coarse.cw[i], fine.cw[i], factor);
    }
    return result;
}

// The derivatives of the nodes of the mesh `make` builds from a geometry moved along each direction: the central
// difference of the meshes of the geometry moved by +-step (exact: the nodes are affine in the geometry when the
// topology is fixed). A direction whose moved meshes do not have the topology of the mesh is marked not valid.
template <typename Make>
NodeDerivatives nodeMoves(const QuadMesh& mesh, std::size_t count, double step, const Make& make,
                          std::array<std::uint8_t, DIRECTIONS>& valid)
{
    NodeDerivatives moves(mesh.nodes.size());
    for (std::size_t i = 0; i < count; ++i)
    {
        std::optional<QuadMesh> plus = make(i, step);
        std::optional<QuadMesh> minus = make(i, -step);
        valid[i] = plus && minus && plus->nodes.size() == mesh.nodes.size() &&
                           minus->nodes.size() == mesh.nodes.size() && plus->quads == mesh.quads &&
                           minus->quads == mesh.quads
                       ? 1
                       : 0;
        if (valid[i] == 0)
            continue;
        for (std::size_t node = 0; node < mesh.nodes.size(); ++node)
            for (std::size_t axis = 0; axis < 2; ++axis)
                moves[node][i][axis] = (plus->nodes[node][axis] - minus->nodes[node][axis]) / (2.0 * step);
    }
    return moves;
}

} // namespace

SectionStiffness SectionShearSolver::ofRectangles(std::span<const SectionRectangle> pieces)
{
    return ofRectangles(pieces, SectionMeshPlan{}, {}, nullptr);
}

SectionMeshPlan SectionShearSolver::planOf(std::span<const SectionRectangle> pieces)
{
    return pieces.empty() ? SectionMeshPlan{} : planOfPieces(pieces);
}

SectionStiffness SectionShearSolver::ofRectangles(std::span<const SectionRectangle> pieces, const SectionMeshPlan& plan,
                                                  std::span<const std::vector<SectionRectangle>> directions,
                                                  SectionStiffnessGradient* gradient)
{
    return ofRectangles(pieces, plan, directions, gradient, nullptr);
}

SectionStiffness SectionShearSolver::ofRectangles(std::span<const SectionRectangle> pieces, const SectionMeshPlan& plan,
                                                  std::span<const std::vector<SectionRectangle>> directions,
                                                  SectionStiffnessGradient* gradient, SectionStiffnessHessian* hessian)
{
    if (pieces.empty())
        return {};
    SectionMeshPlan own;
    const SectionMeshPlan* chosen = &plan;
    std::array<std::optional<QuadMesh>, 2> meshes;
    std::array<std::optional<PlanTopology>, 2> topologies;
    for (std::size_t level = 0; level < 2 && !plan.empty(); ++level)
        meshes[level] = planMesh(plan, pieces, level, &topologies[level]);
    if (!meshes[0] || !meshes[1])
    {
        own = planOfPieces(pieces);
        chosen = &own;
        for (std::size_t level = 0; level < 2; ++level)
            meshes[level] = planMesh(own, pieces, level, &topologies[level]);
        if (!meshes[0] || !meshes[1])
            return {};
    }
    const std::size_t count = gradient != nullptr ? std::min(directions.size(), DIRECTIONS) : 0;
    std::array<SectionStiffness, 2> levels;
    std::array<SectionStiffnessGradient, 2> moved;
    std::array<std::unique_ptr<SectionStiffnessHessian>, 2> curved;
    for (std::size_t level = 0; level < 2; ++level)
    {
        const QuadMesh& mesh = *meshes[level];
        if (hessian != nullptr)
            curved[level] = std::make_unique<SectionStiffnessHessian>();
        if (count == 0)
        {
            levels[level] = solveOn(mesh);
            continue;
        }
        // the derivatives of the nodes: the positions are linear in the edges, so those of the derivatives of the
        // edges (a direction whose edges that meet move apart is not valid)
        std::array<std::uint8_t, DIRECTIONS> valid{};
        NodeDerivatives moves(mesh.nodes.size());
        std::vector<Point> slopes;
        for (std::size_t i = 0; i < count; ++i)
        {
            valid[i] = directions[i].size() == pieces.size() &&
                               placeNodes(*topologies[level], *chosen, directions[i], slopes, false)
                           ? 1
                           : 0;
            if (valid[i] != 0)
                for (std::size_t node = 0; node < slopes.size(); ++node)
                    moves[node][i] = slopes[node];
        }
        levels[level] = solveOn(mesh, &moves, count, &valid, &moved[level], curved[level].get());
    }
    // a plan with graded bands has their cuts (gradedPlan), a uniform one none
    const bool uniform = chosen->cutsY.empty() && chosen->cutsZ.empty();
    const double factor = uniform && hasReentrantCorner(*chosen) ? UNIFORM_RICHARDSON : SMOOTH_RICHARDSON;
    if (gradient != nullptr)
        *gradient = extrapolated(moved[0], moved[1], factor);
    if (hessian != nullptr && count > 0)
        *hessian = extrapolated(*curved[0], *curved[1], factor);
    return extrapolated(levels[0], levels[1], factor);
}

SectionStiffness SectionShearSolver::ofHexagon(double inset, double width, double height)
{
    return ofHexagon(inset, width, height, {}, nullptr);
}

SectionStiffness SectionShearSolver::ofHexagon(double inset, double width, double height,
                                               std::span<const std::array<double, 3>> directions,
                                               SectionStiffnessGradient* gradient)
{
    return ofHexagon(inset, width, height, directions, gradient, nullptr);
}

SectionStiffness SectionShearSolver::ofHexagon(double inset, double width, double height,
                                               std::span<const std::array<double, 3>> directions,
                                               SectionStiffnessGradient* gradient, SectionStiffnessHessian* hessian)
{
    const std::size_t count = gradient != nullptr ? std::min(directions.size(), DIRECTIONS) : 0;
    const double step = 1.0e-5 * std::max({std::fabs(inset), std::fabs(width), std::fabs(height), 1.0e-300});
    std::array<SectionStiffness, 2> levels;
    std::array<SectionStiffnessGradient, 2> moved;
    std::array<std::unique_ptr<SectionStiffnessHessian>, 2> curved;
    for (std::size_t level = 0; level < 2; ++level)
    {
        if (hessian != nullptr)
            curved[level] = std::make_unique<SectionStiffnessHessian>();
        const int cells = level == 0 ? CELLS_PER_THICKNESS : 2 * CELLS_PER_THICKNESS;
        const QuadMesh mesh = hexagonMesh(inset, width, height, cells);
        if (count == 0)
        {
            levels[level] = solveOn(mesh);
            continue;
        }
        std::array<std::uint8_t, DIRECTIONS> valid{};
        const auto make = [&](std::size_t i, double by) -> std::optional<QuadMesh>
        {
            return hexagonMesh(inset + by * directions[i][0], width + by * directions[i][1],
                               height + by * directions[i][2], cells);
        };
        const NodeDerivatives moves = nodeMoves(mesh, count, step, make, valid);
        levels[level] = solveOn(mesh, &moves, count, &valid, &moved[level], curved[level].get());
    }
    if (gradient != nullptr)
        *gradient = extrapolated(moved[0], moved[1], SMOOTH_RICHARDSON);
    if (hessian != nullptr && count > 0)
        *hessian = extrapolated(*curved[0], *curved[1], SMOOTH_RICHARDSON);
    return extrapolated(levels[0], levels[1], SMOOTH_RICHARDSON);
}

} // namespace oor::analysis
