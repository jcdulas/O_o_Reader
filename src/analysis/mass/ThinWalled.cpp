#include "oor/analysis/mass/ThinWalled.hpp"

#include "DualNumber.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <numbers>
#include <stdexcept>
#include <string>

namespace oor::analysis
{

namespace
{

using Point = std::array<double, 2>; // (y, z)
using detail::isZero;
using detail::valueOf;

// The cross product a x b of two vectors of the section plane, about the x axis: a_y b_z - a_z b_y.
template <typename T>
T cross(const std::array<T, 2>& a, const std::array<T, 2>& b)
{
    return a[0] * b[1] - a[1] * b[0];
}

// A loop of the graph of the walls: each wall with the sense it is run in (+1 from its first point to its second).
struct Loop
{
    std::vector<std::pair<std::size_t, double>> walls;
};

// The graph of the walls: a spanning tree from point 0 (the wall and the sense that reach each point, in the order of
// a breadth-first walk) and one loop per wall out of the tree (the wall, then the path of the tree back).
struct WallGraph
{
    std::vector<std::size_t> order;    // the points in the order of the walk
    std::vector<std::int64_t> viaWall; // the wall that reaches each point (-1 for the root)
    std::vector<double> viaSense;      // +1 when that wall runs from its first point to its second towards the point
    std::vector<std::size_t> parent;
    std::vector<std::size_t> depth;
    std::vector<Loop> loops;
};

WallGraph graphOf(std::size_t pointCount, std::span<const std::array<std::int64_t, 2>> walls)
{
    WallGraph graph;
    std::vector<std::vector<std::pair<std::size_t, std::size_t>>> next(pointCount); // (wall, other point)
    for (std::size_t wall = 0; wall < walls.size(); ++wall)
    {
        const auto first = static_cast<std::size_t>(walls[wall][0]);
        const auto second = static_cast<std::size_t>(walls[wall][1]);
        next[first].emplace_back(wall, second);
        next[second].emplace_back(wall, first);
    }
    graph.viaWall.assign(pointCount, -1);
    graph.viaSense.assign(pointCount, 0.0);
    graph.parent.assign(pointCount, 0);
    graph.depth.assign(pointCount, 0);
    std::vector<std::uint8_t> seen(pointCount, 0);
    std::vector<std::uint8_t> inTree(walls.size(), 0);
    std::deque<std::size_t> queue{0};
    seen[0] = 1;
    while (!queue.empty())
    {
        const std::size_t point = queue.front();
        queue.pop_front();
        graph.order.push_back(point);
        for (const auto& [wall, other] : next[point])
        {
            if (seen[other] != 0)
                continue;
            seen[other] = 1;
            inTree[wall] = 1;
            graph.viaWall[other] = static_cast<std::int64_t>(wall);
            graph.viaSense[other] = static_cast<std::size_t>(walls[wall][1]) == other ? 1.0 : -1.0;
            graph.parent[other] = point;
            graph.depth[other] = graph.depth[point] + 1;
            queue.push_back(other);
        }
    }
    if (graph.order.size() != pointCount)
        throw std::invalid_argument("the walls do not connect every point of the section");
    for (std::size_t wall = 0; wall < walls.size(); ++wall)
    {
        if (inTree[wall] != 0)
            continue;
        // the wall from its first point a to its second b, then from b back to a along the tree
        Loop loop;
        loop.walls.emplace_back(wall, 1.0);
        auto a = static_cast<std::size_t>(walls[wall][0]);
        auto b = static_cast<std::size_t>(walls[wall][1]);
        std::vector<std::pair<std::size_t, double>> fromA; // a up to the common ancestor, run towards a
        std::vector<std::pair<std::size_t, double>> fromB; // b up to it, run away from b
        while (a != b)
        {
            if (graph.depth[b] >= graph.depth[a])
            {
                // b -> parent(b): against the sense that reaches b
                fromB.emplace_back(static_cast<std::size_t>(graph.viaWall[b]), -graph.viaSense[b]);
                b = graph.parent[b];
            }
            else
            {
                // parent(a) -> a: the sense that reaches a
                fromA.emplace_back(static_cast<std::size_t>(graph.viaWall[a]), graph.viaSense[a]);
                a = graph.parent[a];
            }
        }
        loop.walls.insert(loop.walls.end(), fromB.begin(), fromB.end());
        loop.walls.insert(loop.walls.end(), fromA.rbegin(), fromA.rend());
        graph.loops.push_back(std::move(loop));
    }
    return graph;
}

// Solves `matrix` x = each column of `right` (n x n, n x k, row-major) by Gaussian elimination with partial pivoting
// (on the values of a number with derivatives, which go through the same operations); the solutions replace `right`.
// False when the matrix is singular.
template <typename T>
bool solveDense(std::vector<T> matrix, std::vector<T>& right, std::size_t n, std::size_t k)
{
    double scale = 0.0;
    for (const T& value : matrix)
        scale = std::max(scale, std::fabs(valueOf(value)));
    for (std::size_t column = 0; column < n; ++column)
    {
        std::size_t pivot = column;
        for (std::size_t row = column + 1; row < n; ++row)
            if (std::fabs(valueOf(matrix[row * n + column])) > std::fabs(valueOf(matrix[pivot * n + column])))
                pivot = row;
        if (!(std::fabs(valueOf(matrix[pivot * n + column])) > 1.0e-13 * scale))
            return false;
        if (pivot != column)
        {
            for (std::size_t index = 0; index < n; ++index)
                std::swap(matrix[pivot * n + index], matrix[column * n + index]);
            for (std::size_t index = 0; index < k; ++index)
                std::swap(right[pivot * k + index], right[column * k + index]);
        }
        const T diagonal = matrix[column * n + column];
        for (std::size_t row = column + 1; row < n; ++row)
        {
            const T factor = matrix[row * n + column] / diagonal;
            if (isZero(factor))
                continue;
            for (std::size_t index = column; index < n; ++index)
                matrix[row * n + index] -= factor * matrix[column * n + index];
            for (std::size_t index = 0; index < k; ++index)
                right[row * k + index] -= factor * right[column * k + index];
        }
    }
    for (std::size_t row = n; row-- > 0;)
        for (std::size_t index = 0; index < k; ++index)
        {
            T sum = right[row * k + index];
            for (std::size_t column = row + 1; column < n; ++column)
                sum -= matrix[row * n + column] * right[column * k + index];
            right[row * k + index] = sum / matrix[row * n + row];
        }
    return true;
}

void checkIndexes(std::span<const std::array<std::int64_t, 2>> walls, std::size_t pointCount, const char* what)
{
    for (const auto& wall : walls)
        for (const std::int64_t index : wall)
            if (index < 0 || static_cast<std::size_t>(index) >= pointCount)
                throw std::invalid_argument(std::string("a wall names the ") + what + " " + std::to_string(index) +
                                            ", which does not exist");
}

// The integral over a wall of f g, f and g linear along it (their values at its ends), per unit of n t: L / 6 (2 f1 g1
// + f1 g2 + f2 g1 + 2 f2 g2).
template <typename T>
T linearProduct(const T& length, const T& f1, const T& f2, const T& g1, const T& g2)
{
    return length / 6.0 * (2.0 * f1 * g1 + f1 * g2 + f2 * g1 + 2.0 * f2 * g2);
}

} // namespace

namespace
{

// The properties of thinWallSection for a scalar T: double, or a number with derivatives with respect to the points
// and the thicknesses (the values the same operations as with doubles).
template <typename T>
struct ThinWallValues
{
    T area{};
    std::array<T, 2> centroid{};
    T i1{};
    T i2{};
    T i12{};
    T iMax{};
    T iMin{};
    T principalAngle{};
    T j{};
    T cw{};
    std::array<T, 2> shearCentre{};
    std::size_t cells = 0;
};

template <typename T>
ThinWallValues<T> sectionOf(std::span<const std::array<T, 2>> points,
                            std::span<const std::array<std::int64_t, 2>> segments, std::span<const T> thickness,
                            std::span<const double> ratio)
{
    using std::atan2;
    using std::hypot;
    using std::pow;
    using P = std::array<T, 2>;
    if (points.empty() || segments.empty())
        throw std::invalid_argument("a thin-walled section needs points and walls");
    if (thickness.size() != segments.size() || (!ratio.empty() && ratio.size() != segments.size()))
        throw std::invalid_argument("a thin-walled section has one thickness (and one modulus ratio) per wall");
    checkIndexes(segments, points.size(), "point");
    const std::size_t count = segments.size();
    std::vector<T> length(count);
    std::vector<T> weight(count); // n t
    for (std::size_t wall = 0; wall < count; ++wall)
    {
        const P& a = points[static_cast<std::size_t>(segments[wall][0])];
        const P& b = points[static_cast<std::size_t>(segments[wall][1])];
        length[wall] = hypot(b[0] - a[0], b[1] - a[1]);
        const double n = ratio.empty() ? 1.0 : ratio[wall];
        if (!(length[wall] > 0.0) || !(thickness[wall] > 0.0) || !(n > 0.0))
            throw std::invalid_argument("the wall " + std::to_string(wall) +
                                        " has no length, or a thickness or modulus ratio not above zero");
        weight[wall] = n * thickness[wall];
    }
    const WallGraph graph = graphOf(points.size(), segments);
    ThinWallValues<T> result;
    result.cells = graph.loops.size();

    // area and centroid
    T area = 0.0;
    T firstY = 0.0;
    T firstZ = 0.0;
    for (std::size_t wall = 0; wall < count; ++wall)
    {
        const P& a = points[static_cast<std::size_t>(segments[wall][0])];
        const P& b = points[static_cast<std::size_t>(segments[wall][1])];
        const T part = weight[wall] * length[wall];
        area += part;
        firstY += part * (a[0] + b[0]) / 2.0;
        firstZ += part * (a[1] + b[1]) / 2.0;
    }
    const P centroid{firstY / area, firstZ / area};
    std::vector<P> local(points.size());
    for (std::size_t point = 0; point < points.size(); ++point)
        local[point] = {points[point][0] - centroid[0], points[point][1] - centroid[1]};
    const auto ends = [&](std::size_t wall) -> std::pair<const P&, const P&>
    {
        return {local[static_cast<std::size_t>(segments[wall][0])], local[static_cast<std::size_t>(segments[wall][1])]};
    };

    // second moments: on the mid-line (the warping), and of the rectangles (reported)
    T lineYY = 0.0;
    T lineZZ = 0.0;
    T lineYZ = 0.0;
    T i1 = 0.0;
    T i2 = 0.0;
    T i12 = 0.0;
    for (std::size_t wall = 0; wall < count; ++wall)
    {
        const auto [a, b] = ends(wall);
        const T w = weight[wall];
        const T yy = w * linearProduct(length[wall], a[0], b[0], a[0], b[0]);
        const T zz = w * linearProduct(length[wall], a[1], b[1], a[1], b[1]);
        const T yz = w * linearProduct(length[wall], a[0], b[0], a[1], b[1]);
        lineYY += yy;
        lineZZ += zz;
        lineYZ += yz;
        // the second moments of the rectangle across its thickness: (n L t^3 / 12) along the normal of the wall
        const T across = (ratio.empty() ? 1.0 : ratio[wall]) * length[wall] * pow(thickness[wall], 3) / 12.0;
        const T ty = (b[0] - a[0]) / length[wall];
        const T tz = (b[1] - a[1]) / length[wall];
        i1 += yy + across * tz * tz;
        i2 += zz + across * ty * ty;
        i12 += yz - across * ty * tz;
    }
    result.area = area;
    result.centroid = centroid;
    result.i1 = i1;
    result.i2 = i2;
    result.i12 = i12;
    const T mean = (i1 + i2) / 2.0;
    const T radius = hypot((i1 - i2) / 2.0, i12);
    result.iMax = mean + radius;
    result.iMin = mean - radius;
    result.principalAngle = 0.5 * atan2(2.0 * i12, i1 - i2) * 180.0 / std::numbers::pi;

    // torsion: the loop flows of a unit G theta' (Bredt and Batho), then the flow of each wall
    const std::size_t loops = graph.loops.size();
    std::vector<T> wallFlow(count, 0.0);
    std::vector<std::uint8_t> inLoop(count, 0);
    T closed = 0.0;
    if (loops > 0)
    {
        std::vector<T> flexibility(loops * loops, 0.0);
        std::vector<T> twiceArea(loops, 0.0);
        std::vector<std::vector<double>> sense(loops, std::vector<double>(count, 0.0));
        for (std::size_t loop = 0; loop < loops; ++loop)
            for (const auto& [wall, wallSense] : graph.loops[loop].walls)
            {
                sense[loop][wall] += wallSense;
                inLoop[wall] = 1;
                const auto [a, b] = ends(wall);
                twiceArea[loop] += wallSense * cross(a, P{b[0] - a[0], b[1] - a[1]});
            }
        for (std::size_t first = 0; first < loops; ++first)
            for (std::size_t second = 0; second < loops; ++second)
                for (std::size_t wall = 0; wall < count; ++wall)
                    flexibility[first * loops + second] +=
                        sense[first][wall] * sense[second][wall] * length[wall] / weight[wall];
        std::vector<T> loopFlow = twiceArea;
        if (!solveDense(flexibility, loopFlow, loops, 1))
            throw std::invalid_argument("the cells of the section enclose no area");
        for (std::size_t loop = 0; loop < loops; ++loop)
        {
            closed += twiceArea[loop] * loopFlow[loop];
            for (std::size_t wall = 0; wall < count; ++wall)
                wallFlow[wall] += sense[loop][wall] * loopFlow[loop];
        }
    }
    T open = 0.0;
    for (std::size_t wall = 0; wall < count; ++wall)
        if (inLoop[wall] == 0)
            open += (ratio.empty() ? 1.0 : ratio[wall]) * length[wall] * pow(thickness[wall], 3) / 3.0;
    result.j = closed + open;

    // the warping function about the centroid at every point, along the tree: dw/ds = q / (n t) - r
    std::vector<T> omega(points.size(), 0.0);
    for (const std::size_t point : graph.order)
    {
        if (graph.viaWall[point] < 0)
            continue;
        const auto wall = static_cast<std::size_t>(graph.viaWall[point]);
        const auto [a, b] = ends(wall);
        const T forward = wallFlow[wall] * length[wall] / weight[wall] - cross(a, P{b[0] - a[0], b[1] - a[1]});
        omega[point] = omega[graph.parent[point]] + graph.viaSense[point] * forward;
    }
    T omegaY = 0.0;
    T omegaZ = 0.0;
    for (std::size_t wall = 0; wall < count; ++wall)
    {
        const auto [a, b] = ends(wall);
        const T& wa = omega[static_cast<std::size_t>(segments[wall][0])];
        const T& wb = omega[static_cast<std::size_t>(segments[wall][1])];
        omegaY += weight[wall] * linearProduct(length[wall], wa, wb, a[0], b[0]);
        omegaZ += weight[wall] * linearProduct(length[wall], wa, wb, a[1], b[1]);
    }
    // the pole (a, b) = (y, z) from the centroid: w_P = w + a z - b y has no product with y nor z
    //   [Iyz -Iyy; Izz -Iyz] [a; b] = [-omegaY; -omegaZ]
    T syy = lineYY;
    T szz = lineZZ;
    T syz = lineYZ;
    if (!(std::fabs(valueOf(syy * szz - syz * syz)) > 1.0e-12 * std::max(valueOf(syy * szz), 1.0e-300)))
    {
        // the walls on one line: their mid-line has no second moment across it, the rectangles have
        syy = i1;
        szz = i2;
        syz = i12;
    }
    const T determinant = -syz * syz + syy * szz;
    const T poleY = (-omegaY * -syz - -syy * -omegaZ) / determinant;
    const T poleZ = (syz * -omegaZ - szz * -omegaY) / determinant;
    result.shearCentre = {centroid[0] + poleY, centroid[1] + poleZ};
    // cw: the warping function about the shear centre, its mean removed
    std::vector<T> pole(points.size());
    for (std::size_t point = 0; point < points.size(); ++point)
        pole[point] = omega[point] + poleY * local[point][1] - poleZ * local[point][0];
    T sum = 0.0;
    for (std::size_t wall = 0; wall < count; ++wall)
    {
        const T& wa = pole[static_cast<std::size_t>(segments[wall][0])];
        const T& wb = pole[static_cast<std::size_t>(segments[wall][1])];
        sum += weight[wall] * length[wall] * (wa + wb) / 2.0;
    }
    const T meanOmega = sum / area;
    T cw = 0.0;
    for (std::size_t wall = 0; wall < count; ++wall)
    {
        const T wa = pole[static_cast<std::size_t>(segments[wall][0])] - meanOmega;
        const T wb = pole[static_cast<std::size_t>(segments[wall][1])] - meanOmega;
        cw += weight[wall] * linearProduct(length[wall], wa, wb, wa, wb);
    }
    result.cw = cw;
    return result;
}

// The 13 fields of a section's values, in the order of ThinWallHessian.
template <typename T>
std::array<T, ThinWallHessian::FIELDS> fieldsOf(const ThinWallValues<T>& r)
{
    return {r.area, r.centroid[0],    r.centroid[1],   r.i1, r.i2, r.i12, r.iMax, r.iMin, r.principalAngle, r.j,
            r.cw,   r.shearCentre[0], r.shearCentre[1]};
}

} // namespace

ThinWallProperties thinWallSection(std::span<const Point> points, std::span<const std::array<std::int64_t, 2>> segments,
                                   std::span<const double> thickness, std::span<const double> ratio)
{
    const ThinWallValues<double> values = sectionOf<double>(points, segments, thickness, ratio);
    ThinWallProperties result;
    result.area = values.area;
    result.centroid = values.centroid;
    result.i1 = values.i1;
    result.i2 = values.i2;
    result.i12 = values.i12;
    result.iMax = values.iMax;
    result.iMin = values.iMin;
    result.principalAngle = values.principalAngle;
    result.j = values.j;
    result.cw = values.cw;
    result.shearCentre = values.shearCentre;
    result.cells = values.cells;
    return result;
}

ThinWallHessian thinWallSectionHessian(std::span<const Point> points,
                                       std::span<const std::array<std::int64_t, 2>> segments,
                                       std::span<const double> thickness, std::span<const double> ratio)
{
    // forward over forward by blocks of CHUNK inputs: an evaluation seeds the inputs of one block as the inner
    // directions and those of another (at or after it) as the outer ones, and gives that block of the Hessians
    constexpr std::size_t CHUNK = 8;
    using Number = detail::Hessian<CHUNK>;
    const std::size_t inputs = 2 * points.size() + thickness.size();
    ThinWallHessian result;
    result.inputs = inputs;
    for (auto& field : result.fields)
        field.assign(inputs * inputs, 0.0);
    const std::size_t blocks = (inputs + CHUNK - 1) / CHUNK;
    std::vector<std::array<Number, 2>> seededPoints(points.size());
    std::vector<Number> seededThickness(thickness.size());
    for (std::size_t inner = 0; inner < blocks; ++inner)
        for (std::size_t outer = inner; outer < blocks; ++outer)
        {
            const auto seed = [&](double value, std::size_t input)
            {
                Number number(value);
                if (input >= inner * CHUNK && input < (inner + 1) * CHUNK)
                    number.d[input - inner * CHUNK].v = 1.0;
                if (input >= outer * CHUNK && input < (outer + 1) * CHUNK)
                    number.v.d[input - outer * CHUNK] = 1.0;
                return number;
            };
            for (std::size_t point = 0; point < points.size(); ++point)
                for (std::size_t axis = 0; axis < 2; ++axis)
                    seededPoints[point][axis] = seed(points[point][axis], 2 * point + axis);
            for (std::size_t wall = 0; wall < thickness.size(); ++wall)
                seededThickness[wall] = seed(thickness[wall], 2 * points.size() + wall);
            const auto fields = fieldsOf(sectionOf<Number>(seededPoints, segments, seededThickness, ratio));
            for (std::size_t field = 0; field < ThinWallHessian::FIELDS; ++field)
                for (std::size_t i = 0; i < CHUNK && inner * CHUNK + i < inputs; ++i)
                    for (std::size_t j = 0; j < CHUNK && outer * CHUNK + j < inputs; ++j)
                    {
                        const std::size_t row = inner * CHUNK + i;
                        const std::size_t column = outer * CHUNK + j;
                        if (column < row)
                            continue; // the upper triangle, mirrored
                        const double value = fields[field].d[i].d[j];
                        result.fields[field][row * inputs + column] = value;
                        result.fields[field][column * inputs + row] = value;
                    }
        }
    return result;
}

ShearFlowSection::ShearFlowSection(std::span<const Point> booms, std::span<const double> areas,
                                   std::span<const std::array<std::int64_t, 2>> walls,
                                   std::span<const double> thickness,
                                   const std::vector<std::vector<std::int64_t>>& cells, Point reference)
{
    const std::size_t pointCount = booms.size();
    const std::size_t count = walls.size();
    if (areas.size() != pointCount || thickness.size() != count || pointCount == 0 || count == 0)
        throw std::invalid_argument("shear flows: one area per boom and one thickness per wall");
    checkIndexes(walls, pointCount, "boom");
    std::vector<double> length(count);
    for (std::size_t wall = 0; wall < count; ++wall)
    {
        const Point& a = booms[static_cast<std::size_t>(walls[wall][0])];
        const Point& b = booms[static_cast<std::size_t>(walls[wall][1])];
        length[wall] = std::hypot(b[0] - a[0], b[1] - a[1]);
        if (!(length[wall] > 0.0) || !(thickness[wall] > 0.0))
            throw std::invalid_argument("shear flows: the wall " + std::to_string(wall) +
                                        " has no length or a thickness not above zero");
    }
    for (const double area : areas)
        if (!(area >= 0.0))
            throw std::invalid_argument("shear flows: a boom area is below zero");
    const WallGraph graph = graphOf(pointCount, walls);
    // the loops: those given, else the independent loops of the graph
    std::vector<Loop> loops;
    if (cells.empty())
        loops = graph.loops;
    else
    {
        if (cells.size() != graph.loops.size())
            throw std::invalid_argument("shear flows: the walls make " + std::to_string(graph.loops.size()) +
                                        " cells and " + std::to_string(cells.size()) + " are given");
        for (const std::vector<std::int64_t>& cell : cells)
        {
            Loop loop;
            for (std::size_t index = 0; index < cell.size(); ++index)
            {
                const std::int64_t from = cell[index];
                const std::int64_t to = cell[(index + 1) % cell.size()];
                std::int64_t found = -1;
                double sense = 0.0;
                for (std::size_t wall = 0; wall < count && found < 0; ++wall)
                {
                    if (walls[wall][0] == from && walls[wall][1] == to)
                        sense = 1.0;
                    else if (walls[wall][0] == to && walls[wall][1] == from)
                        sense = -1.0;
                    if (sense != 0.0)
                        found = static_cast<std::int64_t>(wall);
                }
                if (found < 0 || cell.size() < 3)
                    throw std::invalid_argument("shear flows: a cell is not a closed chain of walls (no wall from " +
                                                std::to_string(from) + " to " + std::to_string(to) + ")");
                loop.walls.emplace_back(static_cast<std::size_t>(found), sense);
            }
            loops.push_back(std::move(loop));
        }
    }
    m_walls = count;
    m_cells = loops.size();

    // the centroid and second moments of the booms
    double total = 0.0;
    double firstY = 0.0;
    double firstZ = 0.0;
    for (std::size_t boom = 0; boom < pointCount; ++boom)
    {
        total += areas[boom];
        firstY += areas[boom] * booms[boom][0];
        firstZ += areas[boom] * booms[boom][1];
    }
    if (!(total > 0.0))
        throw std::invalid_argument("shear flows: the booms have no area");
    m_centroid = {firstY / total, firstZ / total};
    double iyy = 0.0;
    double izz = 0.0;
    double iyz = 0.0;
    for (std::size_t boom = 0; boom < pointCount; ++boom)
    {
        const double y = booms[boom][0] - m_centroid[0];
        const double z = booms[boom][1] - m_centroid[1];
        iyy += areas[boom] * y * y;
        izz += areas[boom] * z * z;
        iyz += areas[boom] * y * z;
    }
    const double determinant = iyy * izz - iyz * iyz;
    if (!(determinant > 1.0e-12 * std::max(iyy * izz, 1.0e-300)))
        throw std::invalid_argument("shear flows: the booms lie on one line (no second moment across it)");

    if (loops.empty())
    {
        solveOpen(booms, areas, walls, thickness, length, reference, {iyy, izz, iyz});
        return;
    }

    // unknowns: the flow of each wall, then G theta'; equations: the booms but the first, the cells, the moment
    const std::size_t size = count + 1;
    std::vector<double> matrix(size * size, 0.0);
    std::vector<double> right(size * 3, 0.0); // unit vy, vz, torque
    std::size_t row = 0;
    for (std::size_t boom = 1; boom < pointCount; ++boom, ++row)
    {
        for (std::size_t wall = 0; wall < count; ++wall)
        {
            if (static_cast<std::size_t>(walls[wall][0]) == boom)
                matrix[row * size + wall] += 1.0; // the flow leaves the boom
            if (static_cast<std::size_t>(walls[wall][1]) == boom)
                matrix[row * size + wall] -= 1.0;
        }
        // the change of the flow across a boom balances the change of its direct load along x (Megson): q_out - q_in =
        // -(vz Iyy - vy Iyz) / D B z - (vy Izz - vz Iyz) / D B y
        const double y = booms[boom][0] - m_centroid[0];
        const double z = booms[boom][1] - m_centroid[1];
        right[row * 3 + 0] = -areas[boom] * (izz * y - iyz * z) / determinant;
        right[row * 3 + 1] = -areas[boom] * (iyy * z - iyz * y) / determinant;
    }
    for (const Loop& loop : loops)
    {
        double twiceArea = 0.0;
        for (const auto& [wall, sense] : loop.walls)
        {
            const Point& a = booms[static_cast<std::size_t>(walls[wall][0])];
            const Point& b = booms[static_cast<std::size_t>(walls[wall][1])];
            matrix[row * size + wall] += sense * length[wall] / thickness[wall];
            twiceArea += sense * cross(a, Point{b[0] - a[0], b[1] - a[1]});
        }
        if (!(std::fabs(twiceArea) > 0.0))
            throw std::invalid_argument("shear flows: a cell encloses no area");
        matrix[row * size + count] = -twiceArea;
        ++row;
    }
    for (std::size_t wall = 0; wall < count; ++wall)
    {
        const Point& a = booms[static_cast<std::size_t>(walls[wall][0])];
        const Point& b = booms[static_cast<std::size_t>(walls[wall][1])];
        matrix[row * size + wall] =
            cross(Point{a[0] - reference[0], a[1] - reference[1]}, Point{b[0] - a[0], b[1] - a[1]});
    }
    right[row * 3 + 2] = 1.0;
    if (++row != size)
        throw std::invalid_argument("shear flows: the walls and cells do not make a determinate section");
    if (!solveDense(matrix, right, size, 3))
        throw std::invalid_argument(
            "shear flows: the section is singular (a cell of no flexibility, or booms that "
            "carry no direct stress where walls branch)");
    for (std::size_t unit = 0; unit < 3; ++unit)
    {
        m_unit[unit].resize(size);
        for (std::size_t index = 0; index < size; ++index)
            m_unit[unit][index] = right[index * 3 + unit];
    }
    // the shear centre: a shear force there gives no twist. vy at (ys, zs) has the moment -(zs - zr) vy about the
    // reference, vz the moment (ys - yr) vz: twist_vy - (zs - zr) twist_T = 0, twist_vz + (ys - yr) twist_T = 0.
    const double twistTorque = m_unit[2][count];
    m_shearCentre = {reference[0] - m_unit[1][count] / twistTorque, reference[1] + m_unit[0][count] / twistTorque};
}

void ShearFlowSection::solveOpen(std::span<const Point> booms, std::span<const double> areas,
                                 std::span<const std::array<std::int64_t, 2>> walls, std::span<const double> thickness,
                                 const std::vector<double>& length, Point reference,
                                 const std::array<double, 3>& moments)
{
    // An open section: the walls are a tree of the booms (connected, no loop: one wall fewer than
    // the booms). The equilibrium of every boom but the first gives the flow of every wall, zero at the free end of
    // each branch (Megson's open-section flows); these flows of a shear force pass through the shear centre, so their
    // moment about the reference gives it. The moment of the loads about the shear centre is carried by the
    // Saint-Venant torsion of the walls alone: G theta' = T / J, J = sum L t^3 / 3, its stresses through the thickness,
    // no flow along the walls.
    const auto [iyy, izz, iyz] = moments;
    const double determinant = iyy * izz - iyz * iyz;
    const std::size_t pointCount = booms.size();
    const std::size_t count = walls.size();
    std::vector<double> matrix(count * count, 0.0);
    std::vector<double> right(count * 2, 0.0); // unit vy, vz
    for (std::size_t boom = 1; boom < pointCount; ++boom)
    {
        const std::size_t row = boom - 1;
        for (std::size_t wall = 0; wall < count; ++wall)
        {
            if (static_cast<std::size_t>(walls[wall][0]) == boom)
                matrix[row * count + wall] += 1.0; // the flow leaves the boom
            if (static_cast<std::size_t>(walls[wall][1]) == boom)
                matrix[row * count + wall] -= 1.0;
        }
        const double y = booms[boom][0] - m_centroid[0];
        const double z = booms[boom][1] - m_centroid[1];
        right[row * 2 + 0] = -areas[boom] * (izz * y - iyz * z) / determinant;
        right[row * 2 + 1] = -areas[boom] * (iyy * z - iyz * y) / determinant;
    }
    if (!solveDense(matrix, right, count, 2))
        throw std::invalid_argument("shear flows: the open section is singular");
    // the moments of the unit flows about the reference: vy through (ys, zs) has the moment -(zs - zr), vz (ys - yr)
    std::array<double, 2> moment{0.0, 0.0};
    double torsion = 0.0;
    for (std::size_t wall = 0; wall < count; ++wall)
    {
        const Point& a = booms[static_cast<std::size_t>(walls[wall][0])];
        const Point& b = booms[static_cast<std::size_t>(walls[wall][1])];
        const double arm = cross(Point{a[0] - reference[0], a[1] - reference[1]}, Point{b[0] - a[0], b[1] - a[1]});
        moment[0] += right[wall * 2 + 0] * arm;
        moment[1] += right[wall * 2 + 1] * arm;
        torsion += length[wall] * thickness[wall] * thickness[wall] * thickness[wall] / 3.0;
    }
    m_shearCentre = {reference[0] + moment[1], reference[1] - moment[0]};
    // the loads act through the reference: about the shear centre a unit vy has the moment zs - zr, a unit vz the
    // moment yr - ys, the torque its own
    const std::array<double, 3> aboutCentre{m_shearCentre[1] - reference[1], reference[0] - m_shearCentre[0], 1.0};
    for (std::size_t unit = 0; unit < 3; ++unit)
    {
        m_unit[unit].assign(count + 1, 0.0);
        if (unit < 2)
            for (std::size_t wall = 0; wall < count; ++wall)
                m_unit[unit][wall] = right[wall * 2 + unit];
        m_unit[unit][count] = aboutCentre[unit] / torsion;
    }
}

void ShearFlowSection::solve(double vy, double vz, double torque, double* flows, double& twist) const
{
    for (std::size_t wall = 0; wall < m_walls; ++wall)
        flows[wall] = vy * m_unit[0][wall] + vz * m_unit[1][wall] + torque * m_unit[2][wall];
    twist = vy * m_unit[0][m_walls] + vz * m_unit[1][m_walls] + torque * m_unit[2][m_walls];
}

} // namespace oor::analysis
