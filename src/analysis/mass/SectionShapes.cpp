#include "DualNumber.hpp"
#include "oor/analysis/mass/SectionFormulas.hpp"
#include "oor/analysis/mass/SectionShearSolver.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <numbers>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

// The cross-section library of PBARL and PBEAML (the standard GROUP), one function per section type. The dimensions are
// read from the format reference figures 9-106 to 9-109: every function says how it cuts its figure. Each section is
// drawn in its own (y, z) coordinates with y vertical and z horizontal, exactly as the figures show them; `Figure`
// integrates the pieces and moves everything to the centroid at the end, so the properties are about the neutral axis
// and the stress recovery points C, D, E, F are measured from it. The formulas are templates on their scalar T: double
// for the properties, or a number with its first and second derivatives (a companion package: the closed forms
// differentiated through the same operations; T = double computes what it did with doubles).

namespace oor::analysis
{

namespace
{

constexpr double PI = std::numbers::pi;

template <typename T>
using Recovery = std::array<std::array<T, 2>, 4>;

using detail::valueOf;

template <typename T>
constexpr bool IS_DOUBLE = std::is_same_v<T, double>;

// The properties of a section for a scalar that carries derivatives: the closed forms of SectionProperties (those the
// finite element formulation gives stay NaN).
template <typename T>
struct ClosedForms
{
    static constexpr double UNDEFINED = std::numeric_limits<double>::quiet_NaN();
    T area = UNDEFINED;
    T i1 = UNDEFINED;
    T i2 = UNDEFINED;
    T i12 = UNDEFINED;
    T j = UNDEFINED;
    T k1 = UNDEFINED;
    T k2 = UNDEFINED;
    Recovery<T> recovery = {
        {{UNDEFINED, UNDEFINED}, {UNDEFINED, UNDEFINED}, {UNDEFINED, UNDEFINED}, {UNDEFINED, UNDEFINED}}};
    std::array<T, 2> drawingCentroid = {UNDEFINED, UNDEFINED};
    std::array<T, 4> drawingBounds = {UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED};
    std::array<T, 2> drawingShearCentre = {UNDEFINED, UNDEFINED};
    T cw = UNDEFINED;
};

// What the formulas of a scalar return: SectionProperties for double.
template <typename T>
using PropertiesOf = std::conditional_t<IS_DOUBLE<T>, SectionProperties, ClosedForms<T>>;

// x to a whole power, as std::pow.
template <typename T>
T power(const T& value, int exponent)
{
    using std::pow;
    return pow(value, exponent);
}

// How the finite element formulation runs in the sections the thread computes: `Capture` keeps the
// rectangles of the figure and solves nothing, `Skip` solves nothing (the closed forms only), `Solve` solves on the
// meshes of `plan` with the derivatives along `directions`.
struct SolverRequest
{
    enum class Mode : std::uint8_t
    {
        Capture,
        Skip,
        Solve
    };
    Mode mode = Mode::Solve;
    std::vector<SectionRectangle> pieces;
    const SectionMeshPlan* plan = nullptr;
    std::span<const std::vector<SectionRectangle>> directions;
    std::size_t hexagonDirections = 0; // HEXA: the derivatives along DIM1, DIM2, DIM3
    bool wantGradient = false;
    SectionStiffnessGradient gradient;
    bool wantHessian = false; // with wantGradient
    std::unique_ptr<SectionStiffnessHessian> hessian;
    bool stiffnessSolved = false; // K1, K2 and J came from the solver
    bool warpingSolved = false;   // the shear centre and the warping constant
};

thread_local SolverRequest* currentRequest = nullptr;

// The request of the thread while it lives.
class RequestScope
{
public:
    explicit RequestScope(SolverRequest& request) : m_previous(currentRequest) { currentRequest = &request; }
    ~RequestScope() { currentRequest = m_previous; }
    RequestScope(const RequestScope&) = delete;
    RequestScope& operator=(const RequestScope&) = delete;
    RequestScope(RequestScope&&) = delete;
    RequestScope& operator=(RequestScope&&) = delete;

private:
    SolverRequest* m_previous;
};

// True when the figures solve nothing (Capture, Skip, and the figures of a scalar with derivatives).
template <typename T = double>
bool solverSkipped()
{
    if constexpr (!IS_DOUBLE<T>)
        return true;
    else
        return currentRequest != nullptr && currentRequest->mode != SolverRequest::Mode::Solve;
}

// The finite element formulation of a section made of rectangles, as the request of the thread asks.
SectionStiffness solveRectangles(const std::vector<SectionRectangle>& pieces)
{
    SolverRequest* request = currentRequest;
    if (request == nullptr)
        return SectionShearSolver::ofRectangles(pieces);
    static const SectionMeshPlan NO_PLAN;
    return SectionShearSolver::ofRectangles(pieces, request->plan != nullptr ? *request->plan : NO_PLAN,
                                            request->directions, request->wantGradient ? &request->gradient : nullptr,
                                            request->wantHessian ? request->hessian.get() : nullptr);
}

// The pieces of a section and their integrals about the drawing origin. Rectangles, disks and one polygon are enough
// for the whole library. Each rectangle is drawn as what it is in the beam library equations: a vertical wall (its area
// carries the shear along y), a horizontal wall (along z) or a corner square where two walls meet, which carries
// neither (the rule the solver applies under MDLPRM,TWBRBML,1). Under the solver's finite element rule the
// rectangles together are the region SectionShearSolver meshes, whatever their role.
template <typename T>
class Figure
{
public:
    explicit Figure(ShearFactorRule rule) : m_rule(rule) {}

    // The rectangle between y0 and y1 (vertical) and z0 and z1 (horizontal), a wall along y.
    void verticalWall(const T& y0, const T& y1, const T& z0, const T& z1) { piece(y0, y1, z0, z1, Role::Vertical); }
    // A wall along z.
    void horizontalWall(const T& y0, const T& y1, const T& z0, const T& z1) { piece(y0, y1, z0, z1, Role::Horizontal); }
    // A corner square.
    void corner(const T& y0, const T& y1, const T& z0, const T& z1) { piece(y0, y1, z0, z1, Role::Corner); }

    // A flange between y0 and y1 that spans z0..z1, crossed by a wall between w0 and w1: the part of the flange in the
    // wall is a corner, the rest horizontal wall.
    void flangeAcross(const T& y0, const T& y1, const T& z0, const T& z1, const T& w0, const T& w1)
    {
        const T inner0 = std::max(z0, w0);
        const T inner1 = std::min(z1, w1);
        if (inner0 > z0)
            horizontalWall(y0, y1, z0, inner0);
        if (inner1 > inner0)
            corner(y0, y1, inner0, inner1);
        if (z1 > inner1)
            horizontalWall(y0, y1, inner1, z1);
    }

    // A disk centred on the drawing origin; `sign` -1 takes it out (the hole of a tube).
    void disk(const T& radius, double sign)
    {
        if (sign > 0.0)
            bound(-radius, radius, -radius, radius);
        const T pieceArea = PI * radius * radius;
        m_area += sign * pieceArea;
        m_secondYY += sign * pieceArea * radius * radius / 4.0;
        m_secondZZ += sign * pieceArea * radius * radius / 4.0;
    }

    // A polygon given as (z, y) corners going counter-clockwise in the plane with z to the right and y up. Its shear
    // factors are given by the caller (`setShear`).
    void polygon(const std::vector<std::array<T, 2>>& corners)
    {
        const std::size_t count = corners.size();
        for (const auto& corner : corners)
            bound(corner[1], corner[1], corner[0], corner[0]);
        for (std::size_t index = 0; index < count; ++index)
        {
            const auto& first = corners[index];
            const auto& second = corners[(index + 1) % count];
            const T& z0 = first[0];
            const T& y0 = first[1];
            const T& z1 = second[0];
            const T& y1 = second[1];
            const T cross = z0 * y1 - z1 * y0;
            m_area += cross / 2.0;
            m_firstZ += (z0 + z1) * cross / 6.0;
            m_firstY += (y0 + y1) * cross / 6.0;
            m_secondYY += (y0 * y0 + y0 * y1 + y1 * y1) * cross / 12.0;
            m_secondZZ += (z0 * z0 + z0 * z1 + z1 * z1) * cross / 12.0;
            m_secondYZ += (z0 * y1 + 2.0 * z0 * y0 + 2.0 * z1 * y1 + z1 * y0) * cross / 24.0;
        }
    }

    // Shear factors (and a torsional constant, when `torsion` is given) known in closed form or computed by the caller.
    void setShear(const T& k1, const T& k2, std::optional<T> torsion = std::nullopt)
    {
        m_fixedShear = true;
        m_k1 = k1;
        m_k2 = k2;
        m_fixedTorsion = torsion;
    }

    // The shear centre (drawing coordinates) and the warping constant computed by the caller (HEXA).
    void setWarping(const T& shearY, const T& shearZ, const T& cw)
    {
        m_hasWarping = true;
        m_warping = {shearY, shearZ, cw};
    }

    // The properties about the centroid, with the recovery points (given in drawing coordinates) moved to it.
    // `torsion` is the torsional constant of the beam library formulas, used unless the rule is the solver's for a
    // section made of rectangles (then SectionShearSolver gives it).
    [[nodiscard]] PropertiesOf<T> finish(const T& torsion, const Recovery<T>& points) const
    {
        PropertiesOf<T> result;
        if (!(m_area > 0.0))
            return result;
        const T centroidY = m_firstY / m_area;
        const T centroidZ = m_firstZ / m_area;
        result.area = m_area;
        result.i1 = m_secondYY - m_area * centroidY * centroidY;
        result.i2 = m_secondZZ - m_area * centroidZ * centroidZ;
        result.i12 = m_secondYZ - m_area * centroidY * centroidZ;
        // Round-off can leave a tiny non-zero product for a symmetric section: it is exactly zero by symmetry (its
        // derivatives are kept: a dimension may break the symmetry).
        if constexpr (IS_DOUBLE<T>)
            if (std::abs(result.i12) < 1.0e-13 * std::max(result.i1, result.i2))
                result.i12 = 0.0;
        result.j = m_fixedTorsion.value_or(torsion);
        if (m_rule == ShearFactorRule::None)
        {
            // Not asked for: K stays NaN and J is the closed form or the library formula.
        }
        else if (m_fixedShear)
        {
            result.k1 = m_k1;
            result.k2 = m_k2;
        }
        else if (m_rule == ShearFactorRule::BeamLibrary)
        {
            result.k1 = m_verticalArea / m_area;
            result.k2 = m_horizontalArea / m_area;
        }
        if (currentRequest != nullptr && currentRequest->mode == SolverRequest::Mode::Capture)
            currentRequest->pieces = m_pieces;
        const bool skipped = solverSkipped<T>();
        std::optional<SectionStiffness> solved;
        if (m_rule != ShearFactorRule::None && !m_fixedShear && m_rule == ShearFactorRule::Solver && !m_pieces.empty())
        {
            if (!skipped)
            {
                solved = solveRectangles(m_pieces);
                if (currentRequest != nullptr)
                    currentRequest->stiffnessSolved = true;
            }
            result.k1 = solved ? solved->k1 : SectionProperties::UNDEFINED;
            result.k2 = solved ? solved->k2 : SectionProperties::UNDEFINED;
            result.j = solved ? solved->j : SectionProperties::UNDEFINED;
        }
        for (std::size_t point = 0; point < 4; ++point)
            result.recovery[point] = {points[point][0] - centroidY, points[point][1] - centroidZ};
        result.drawingCentroid = {centroidY, centroidZ};
        result.drawingBounds = {m_yMin, m_yMax, m_zMin, m_zMax};
        if (m_rule != ShearFactorRule::None)
        {
            // the shear centre and the warping constant: the warping function of the section
            if (m_hasWarping)
            {
                result.drawingShearCentre = {m_warping[0], m_warping[1]};
                result.cw = m_warping[2];
            }
            else if (m_pieces.empty())
            {
                // disks: a rod or a tube does not warp
                result.drawingShearCentre = {centroidY, centroidZ};
                result.cw = 0.0;
            }
            else if (!skipped)
            {
                if (!solved)
                    solved = solveRectangles(m_pieces);
                if (currentRequest != nullptr)
                    currentRequest->warpingSolved = true;
                result.drawingShearCentre = {solved->shearY, solved->shearZ};
                result.cw = solved->cw;
            }
        }
        return result;
    }

private:
    enum class Role : std::uint8_t
    {
        Vertical,
        Horizontal,
        Corner
    };

    void bound(const T& y0, const T& y1, const T& z0, const T& z1)
    {
        m_yMin = std::min(m_yMin, y0);
        m_yMax = std::max(m_yMax, y1);
        m_zMin = std::min(m_zMin, z0);
        m_zMax = std::max(m_zMax, z1);
    }

    void piece(const T& y0, const T& y1, const T& z0, const T& z1, Role role)
    {
        const T width = z1 - z0;
        const T height = y1 - y0;
        if (!(width > 0.0) || !(height > 0.0))
            return;
        bound(y0, y1, z0, z1);
        const T pieceArea = width * height;
        m_area += pieceArea;
        m_firstY += width * (y1 * y1 - y0 * y0) / 2.0;
        m_firstZ += height * (z1 * z1 - z0 * z0) / 2.0;
        m_secondYY += width * (y1 * y1 * y1 - y0 * y0 * y0) / 3.0;
        m_secondZZ += height * (z1 * z1 * z1 - z0 * z0 * z0) / 3.0;
        m_secondYZ += (y1 * y1 - y0 * y0) / 2.0 * (z1 * z1 - z0 * z0) / 2.0;
        if (role == Role::Vertical)
            m_verticalArea += pieceArea;
        else if (role == Role::Horizontal)
            m_horizontalArea += pieceArea;
        m_pieces.push_back(SectionRectangle{valueOf(y0), valueOf(y1), valueOf(z0), valueOf(z1)});
    }

    ShearFactorRule m_rule;
    T m_area = 0.0;
    T m_firstY = 0.0; // integral of y
    T m_firstZ = 0.0;
    T m_secondYY = 0.0; // integral of y squared
    T m_secondZZ = 0.0;
    T m_secondYZ = 0.0;
    T m_verticalArea = 0.0;
    T m_horizontalArea = 0.0;
    std::vector<SectionRectangle> m_pieces; // the values (the rectangles the solver meshes)
    bool m_fixedShear = false;
    T m_k1 = SectionProperties::UNDEFINED;
    T m_k2 = SectionProperties::UNDEFINED;
    std::optional<T> m_fixedTorsion;
    bool m_hasWarping = false;
    std::array<T, 3> m_warping = {0.0, 0.0, 0.0};
    T m_yMin = std::numeric_limits<double>::infinity();
    T m_yMax = -std::numeric_limits<double>::infinity();
    T m_zMin = std::numeric_limits<double>::infinity();
    T m_zMax = -std::numeric_limits<double>::infinity();
};

template <typename T>
[[nodiscard]] bool allPositive(std::span<const T> dimensions) noexcept
{
    return std::all_of(dimensions.begin(), dimensions.end(), [](const T& value) { return value > 0.0; });
}

// The thin-walled approximation of the torsional constant of a rectangle of length b and thickness t: b t^3 / 3.
template <typename T>
[[nodiscard]] T thin(const T& length, const T& thickness)
{
    return length * thickness * thickness * thickness / 3.0;
}

// The torsional constant of a solid rectangle by the series approximation an element guide gives:
// b t^3 (1/3 - 0.21 (t / b) (1 - t^4 / (12 b^4))), b the longer side and t the shorter.
template <typename T>
[[nodiscard]] T solidRectangle(const T& first, const T& second)
{
    const T longer = std::max(first, second);
    const T shorter = std::min(first, second);
    const T ratio = shorter / longer;
    return longer * shorter * shorter * shorter * (1.0 / 3.0 - 0.21 * ratio * (1.0 - power(ratio, 4) / 12.0));
}

// Bredt's thin-walled closed section: 4 Am^2 / (sum of the length over the thickness of each wall).
template <typename T>
[[nodiscard]] T closedCell(const T& enclosedArea, const T& perimeterOverThickness)
{
    return 4.0 * enclosedArea * enclosedArea / perimeterOverThickness;
}

// The shear factor of a tube under the solver's rule: Cowper's formula with no Poisson effect (the energy-consistent
// factor of the Saint-Venant flexure solution, which the finite element formulation computes), m the inner radius over
// the outer one: 6/7 for a rod (m = 0), 1/2 for a thin tube. It matches the factors the solver gives a ROD and a TUBE
// to 4e-4.
template <typename T>
[[nodiscard]] T tubeShearFactor(const T& ratio)
{
    const T square = (1.0 + ratio * ratio) * (1.0 + ratio * ratio);
    return 6.0 * square / (7.0 * square + 20.0 * ratio * ratio);
}

// ROD: DIM1 is the radius. C, D, E, F are the top, right, bottom and left of the circle.
template <typename T>
PropertiesOf<T> rodSection(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 1 || !allPositive(d))
        return {};
    Figure<T> figure(rule);
    figure.disk(d[0], 1.0);
    const T shear = rule == ShearFactorRule::BeamLibrary ? 0.9 : tubeShearFactor(0.0);
    figure.setShear(shear, shear);
    return figure.finish(PI * power(d[0], 4) / 2.0, {{{d[0], 0.0}, {0.0, d[0]}, {-d[0], 0.0}, {0.0, -d[0]}}});
}

// TUBE: DIM1 the outer radius, DIM2 the inner radius.
template <typename T>
PropertiesOf<T> tubeSection(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 2 || !allPositive(d) || d[1] >= d[0])
        return {};
    Figure<T> figure(rule);
    figure.disk(d[0], 1.0);
    figure.disk(d[1], -1.0);
    const T shear = rule == ShearFactorRule::BeamLibrary ? 0.5 : tubeShearFactor(d[1] / d[0]);
    figure.setShear(shear, shear);
    return figure.finish(PI * (power(d[0], 4) - power(d[1], 4)) / 2.0,
                         {{{d[0], 0.0}, {0.0, d[0]}, {-d[0], 0.0}, {0.0, -d[0]}}});
}

// TUBE2: DIM1 the outer radius, DIM2 the wall thickness.
template <typename T>
PropertiesOf<T> tube2Section(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 2 || !allPositive(d) || d[1] >= d[0])
        return {};
    const std::array<T, 2> both = {d[0], d[0] - d[1]};
    return tubeSection<T>(both, rule);
}

// L: DIM1 the width of the horizontal leg, DIM2 the height of the vertical leg, DIM3 the thickness of the horizontal
// leg, DIM4 the thickness of the vertical leg. The vertical leg is on the left; E is the outer corner, D the end of the
// horizontal leg, F and C the two corners at the top of the vertical leg.
template <typename T>
PropertiesOf<T> lSection(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 4 || !allPositive(d) || d[2] >= d[1] || d[3] >= d[0])
        return {};
    Figure<T> figure(rule);
    figure.corner(0.0, d[2], 0.0, d[3]);
    figure.horizontalWall(0.0, d[2], d[3], d[0]);
    figure.verticalWall(d[2], d[1], 0.0, d[3]);
    const T torsion = thin(d[0], d[2]) + thin(d[1] - d[2], d[3]);
    return figure.finish(torsion, {{{d[1], d[3]}, {0.0, d[0]}, {0.0, 0.0}, {d[1], 0.0}}});
}

// I: DIM1 the height, DIM2 and DIM3 the widths of the bottom and top flanges, DIM4 the web thickness, DIM5 and DIM6 the
// thicknesses of the bottom and top flanges. C is the top right, D the bottom right, E the bottom left, F the top left.
template <typename T>
PropertiesOf<T> iSection(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 6 || !allPositive(d) || d[4] + d[5] >= d[0])
        return {};
    const T h = d[0];
    const T web = d[3] / 2.0;
    Figure<T> figure(rule);
    figure.flangeAcross(0.0, d[4], -d[1] / 2.0, d[1] / 2.0, -web, web);
    figure.verticalWall(d[4], h - d[5], -web, web);
    figure.flangeAcross(h - d[5], h, -d[2] / 2.0, d[2] / 2.0, -web, web);
    const T torsion = thin(d[1], d[4]) + thin(d[2], d[5]) + thin(h - d[4] - d[5], d[3]);
    return figure.finish(torsion, {{{h, d[2] / 2.0}, {0.0, d[1] / 2.0}, {0.0, -d[1] / 2.0}, {h, -d[2] / 2.0}}});
}

// CHAN: DIM1 the width over the flanges, DIM2 the height, DIM3 the web thickness, DIM4 the flange thickness. The web is
// on the left and the flanges point right. C is the top right, D the bottom right, E the bottom left, F the top left.
template <typename T>
PropertiesOf<T> chanSection(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 4 || !allPositive(d) || d[3] * 2.0 >= d[1] || d[2] >= d[0])
        return {};
    const T h = d[1];
    Figure<T> figure(rule);
    figure.corner(0.0, d[3], 0.0, d[2]);
    figure.verticalWall(d[3], h - d[3], 0.0, d[2]);
    figure.corner(h - d[3], h, 0.0, d[2]);
    figure.horizontalWall(h - d[3], h, d[2], d[0]);
    figure.horizontalWall(0.0, d[3], d[2], d[0]);
    const T torsion = thin(h, d[2]) + 2.0 * thin(d[0] - d[2], d[3]);
    return figure.finish(torsion, {{{h, d[0]}, {0.0, d[0]}, {0.0, 0.0}, {h, 0.0}}});
}

// T: DIM1 the flange width, DIM2 the overall height, DIM3 the flange thickness, DIM4 the web thickness. The flange is
// on top. C is the middle of the top, D its right end, F its left end, E the bottom of the web.
template <typename T>
PropertiesOf<T> tSection(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 4 || !allPositive(d) || d[2] >= d[1] || d[3] >= d[0])
        return {};
    const T h = d[1];
    Figure<T> figure(rule);
    figure.flangeAcross(h - d[2], h, -d[0] / 2.0, d[0] / 2.0, -d[3] / 2.0, d[3] / 2.0);
    figure.verticalWall(0.0, h - d[2], -d[3] / 2.0, d[3] / 2.0);
    const T torsion = thin(d[0], d[2]) + thin(h - d[2], d[3]);
    return figure.finish(torsion, {{{h, 0.0}, {h, d[0] / 2.0}, {0.0, 0.0}, {h, -d[0] / 2.0}}});
}

// BOX: DIM1 the width, DIM2 the height, DIM3 the thickness of the top and bottom walls, DIM4 the thickness of the side
// walls. C is the top right, D the bottom right, E the bottom left, F the top left. The torsional constant of the beam
// library is Bredt's.
template <typename T>
PropertiesOf<T> boxSection(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 4 || !allPositive(d) || d[3] * 2.0 >= d[0] || d[2] * 2.0 >= d[1])
        return {};
    const T w = d[0];
    const T h = d[1];
    Figure<T> figure(rule);
    for (const T& y0 : {T(0.0), h - d[2]})
    {
        figure.corner(y0, y0 + d[2], 0.0, d[3]);
        figure.horizontalWall(y0, y0 + d[2], d[3], w - d[3]);
        figure.corner(y0, y0 + d[2], w - d[3], w);
    }
    figure.verticalWall(d[2], h - d[2], 0.0, d[3]);
    figure.verticalWall(d[2], h - d[2], w - d[3], w);
    const T torsion =
        2.0 * d[2] * d[3] * power(w - d[3], 2) * power(h - d[2], 2) / (w * d[3] + h * d[2] - d[3] * d[3] - d[2] * d[2]);
    return figure.finish(torsion, {{{h, w}, {0.0, w}, {0.0, 0.0}, {h, 0.0}}});
}

// BAR: DIM1 the width, DIM2 the height. C is the top right, D the bottom right, E the bottom left, F the top left. The
// shear factors are 5/6 under both rules: the shear problem of a rectangle depends on one coordinate only, and 5/6 is
// its exact solution. The torsional constant is the series of a solid rectangle under both rules.
template <typename T>
PropertiesOf<T> barSection(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 2 || !allPositive(d))
        return {};
    Figure<T> figure(rule);
    figure.verticalWall(0.0, d[1], 0.0, d[0]);
    figure.setShear(5.0 / 6.0, 5.0 / 6.0, solidRectangle(d[0], d[1]));
    return figure.finish(solidRectangle(d[0], d[1]), {{{d[1], d[0]}, {0.0, d[0]}, {0.0, 0.0}, {d[1], 0.0}}});
}

// CROSS: the stem is DIM2 wide and DIM3 high; the arms have a thickness DIM4 and reach DIM1 / 2 beyond the stem on each
// side, so DIM1 is the total length of the arms. C and E are the top and bottom of the stem, D and F the ends of the
// arms. Under the beam library rule the band of the arms across the stem is a corner.
template <typename T>
PropertiesOf<T> crossSection(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 4 || !allPositive(d) || d[3] >= d[2])
        return {};
    const T stem = d[1];
    const T arm = d[0] / 2.0;
    Figure<T> figure(rule);
    figure.verticalWall(d[3] / 2.0, d[2] / 2.0, -stem / 2.0, stem / 2.0);
    figure.corner(-d[3] / 2.0, d[3] / 2.0, -stem / 2.0, stem / 2.0);
    figure.verticalWall(-d[2] / 2.0, -d[3] / 2.0, -stem / 2.0, stem / 2.0);
    figure.horizontalWall(-d[3] / 2.0, d[3] / 2.0, stem / 2.0, stem / 2.0 + arm);
    figure.horizontalWall(-d[3] / 2.0, d[3] / 2.0, -stem / 2.0 - arm, -stem / 2.0);
    const T torsion = thin(d[2], stem) + thin(d[0], d[3]);
    return figure.finish(torsion,
                         {{{d[2] / 2.0, 0.0}, {0.0, stem / 2.0 + arm}, {-d[2] / 2.0, 0.0}, {0.0, -stem / 2.0 - arm}}});
}

// H: two vertical flanges and a crossbar. DIM1 is the gap between the flanges, DIM2 the thickness of the two flanges
// together (each is DIM2 / 2), DIM3 their height, DIM4 the thickness of the crossbar. C is the top right, D the bottom
// right, E the bottom left, F the top left. Under the beam library rule the band of the crossbar across each flange is
// a corner.
template <typename T>
PropertiesOf<T> hSection(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 4 || !allPositive(d) || d[3] >= d[2])
        return {};
    const T inner = d[0] / 2.0;
    const T outer = (d[0] + d[1]) / 2.0;
    Figure<T> figure(rule);
    for (const auto& [z0, z1] : {std::pair{-outer, -inner}, std::pair{inner, outer}})
    {
        figure.verticalWall(d[3] / 2.0, d[2] / 2.0, z0, z1);
        figure.corner(-d[3] / 2.0, d[3] / 2.0, z0, z1);
        figure.verticalWall(-d[2] / 2.0, -d[3] / 2.0, z0, z1);
    }
    figure.horizontalWall(-d[3] / 2.0, d[3] / 2.0, -inner, inner);
    const T torsion = 2.0 * thin(d[2], d[1] / 2.0) + thin(d[0], d[3]);
    return figure.finish(torsion,
                         {{{d[2] / 2.0, outer}, {-d[2] / 2.0, outer}, {-d[2] / 2.0, -outer}, {d[2] / 2.0, -outer}}});
}

// T1: a vertical flange DIM1 high and DIM3 thick, with a stem DIM2 long and DIM4 thick pointing left from the middle of
// the flange. C is the middle of the outer face of the flange, D and F its lower and upper ends, E the end of the stem.
template <typename T>
PropertiesOf<T> t1Section(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 4 || !allPositive(d) || d[3] >= d[0])
        return {};
    Figure<T> figure(rule);
    figure.verticalWall(d[3] / 2.0, d[0] / 2.0, 0.0, d[2]);
    figure.corner(-d[3] / 2.0, d[3] / 2.0, 0.0, d[2]);
    figure.verticalWall(-d[0] / 2.0, -d[3] / 2.0, 0.0, d[2]);
    figure.horizontalWall(-d[3] / 2.0, d[3] / 2.0, -d[1], 0.0);
    const T torsion = thin(d[0], d[2]) + thin(d[1], d[3]);
    return figure.finish(torsion, {{{0.0, d[2]}, {-d[0] / 2.0, d[2]}, {0.0, -d[1]}, {d[0] / 2.0, d[2]}}});
}

// I1: flanges DIM1 + DIM2 wide, a web DIM2 thick and DIM3 high between them, DIM4 the overall height (each flange is
// (DIM4 - DIM3) / 2 thick). C is the top right, D the bottom right, E the bottom left, F the top left.
template <typename T>
PropertiesOf<T> i1Section(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 4 || !allPositive(d) || d[2] >= d[3])
        return {};
    const T flange = (d[3] - d[2]) / 2.0;
    const T half = (d[0] + d[1]) / 2.0;
    Figure<T> figure(rule);
    figure.flangeAcross(0.0, flange, -half, half, -d[1] / 2.0, d[1] / 2.0);
    figure.verticalWall(flange, flange + d[2], -d[1] / 2.0, d[1] / 2.0);
    figure.flangeAcross(flange + d[2], d[3], -half, half, -d[1] / 2.0, d[1] / 2.0);
    const T torsion = 2.0 * thin(d[0] + d[1], flange) + thin(d[2], d[1]);
    return figure.finish(torsion, {{{d[3], half}, {0.0, half}, {0.0, -half}, {d[3], -half}}});
}

// CHAN1: a web DIM2 thick and DIM4 high on the left, flanges DIM1 long beyond it and (DIM4 - DIM3) / 2 thick. C is the
// top right, D the bottom right, E the bottom left, F the top left.
template <typename T>
PropertiesOf<T> chan1Section(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 4 || !allPositive(d) || d[2] >= d[3])
        return {};
    const T flange = (d[3] - d[2]) / 2.0;
    Figure<T> figure(rule);
    figure.corner(0.0, flange, 0.0, d[1]);
    figure.verticalWall(flange, d[3] - flange, 0.0, d[1]);
    figure.corner(d[3] - flange, d[3], 0.0, d[1]);
    figure.horizontalWall(d[3] - flange, d[3], d[1], d[1] + d[0]);
    figure.horizontalWall(0.0, flange, d[1], d[1] + d[0]);
    const T torsion = thin(d[3], d[1]) + 2.0 * thin(d[0], flange);
    return figure.finish(torsion, {{{d[3], d[0] + d[1]}, {0.0, d[0] + d[1]}, {0.0, 0.0}, {d[3], 0.0}}});
}

// Z: a web DIM2 thick and DIM4 high, a top flange reaching DIM1 to the left of it and a bottom flange DIM1 to the
// right, each (DIM4 - DIM3) / 2 thick. C is the top right of the web, D the end of the bottom flange, E the bottom left
// of the web, F the end of the top flange.
template <typename T>
PropertiesOf<T> zSection(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 4 || !allPositive(d) || d[2] >= d[3])
        return {};
    const T flange = (d[3] - d[2]) / 2.0;
    Figure<T> figure(rule);
    figure.corner(0.0, flange, 0.0, d[1]);
    figure.verticalWall(flange, d[3] - flange, 0.0, d[1]);
    figure.corner(d[3] - flange, d[3], 0.0, d[1]);
    figure.horizontalWall(d[3] - flange, d[3], -d[0], 0.0);
    figure.horizontalWall(0.0, flange, d[1], d[1] + d[0]);
    const T torsion = thin(d[3], d[1]) + 2.0 * thin(d[0], flange);
    return figure.finish(torsion, {{{d[3], d[1]}, {0.0, d[1] + d[0]}, {0.0, 0.0}, {d[3], -d[0]}}});
}

// CHAN2: a U standing on its base: DIM1 the thickness of the legs, DIM2 the thickness of the base, DIM3 the overall
// height, DIM4 the overall width. C is the top right, D the bottom right, E the bottom left, F the top left.
template <typename T>
PropertiesOf<T> chan2Section(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 4 || !allPositive(d) || d[1] >= d[2] || d[0] * 2.0 >= d[3])
        return {};
    Figure<T> figure(rule);
    figure.corner(0.0, d[1], 0.0, d[0]);
    figure.horizontalWall(0.0, d[1], d[0], d[3] - d[0]);
    figure.corner(0.0, d[1], d[3] - d[0], d[3]);
    figure.verticalWall(d[1], d[2], 0.0, d[0]);
    figure.verticalWall(d[1], d[2], d[3] - d[0], d[3]);
    const T torsion = thin(d[3], d[1]) + 2.0 * thin(d[2] - d[1], d[0]);
    return figure.finish(torsion, {{{d[2], d[3]}, {0.0, d[3]}, {0.0, 0.0}, {d[2], 0.0}}});
}

// T2: an upside-down T: a flange DIM1 wide and DIM3 thick at the bottom, a web DIM4 wide up to the overall height DIM2.
// C and F are the top right and left of the web, D and E the bottom right and left of the flange.
template <typename T>
PropertiesOf<T> t2Section(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 4 || !allPositive(d) || d[2] >= d[1] || d[3] >= d[0])
        return {};
    Figure<T> figure(rule);
    figure.flangeAcross(0.0, d[2], -d[0] / 2.0, d[0] / 2.0, -d[3] / 2.0, d[3] / 2.0);
    figure.verticalWall(d[2], d[1], -d[3] / 2.0, d[3] / 2.0);
    const T torsion = thin(d[0], d[2]) + thin(d[1] - d[2], d[3]);
    return figure.finish(torsion, {{{d[1], d[3] / 2.0}, {0.0, d[0] / 2.0}, {0.0, -d[0] / 2.0}, {d[1], -d[3] / 2.0}}});
}

// BOX1: a box whose four walls have their own thickness: DIM1 the width, DIM2 the height, DIM3 the top wall, DIM4 the
// bottom wall, DIM5 the right wall, DIM6 the left wall. C is the top right, D the bottom right, E the bottom left, F
// the top left. The torsional constant of the beam library is Bredt's, on the mid-lines of the walls.
template <typename T>
PropertiesOf<T> box1Section(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 6 || !allPositive(d) || d[2] + d[3] >= d[1] || d[4] + d[5] >= d[0])
        return {};
    const T w = d[0];
    const T h = d[1];
    Figure<T> figure(rule);
    for (const auto& [y0, y1] : {std::pair<T, T>{h - d[2], h}, std::pair<T, T>{0.0, d[3]}})
    {
        figure.corner(y0, y1, 0.0, d[5]);
        figure.horizontalWall(y0, y1, d[5], w - d[4]);
        figure.corner(y0, y1, w - d[4], w);
    }
    figure.verticalWall(d[3], h - d[2], w - d[4], w);
    figure.verticalWall(d[3], h - d[2], 0.0, d[5]);
    const T midWidth = w - (d[4] + d[5]) / 2.0;
    const T midHeight = h - (d[2] + d[3]) / 2.0;
    const T torsion =
        closedCell(midWidth * midHeight, midWidth / d[2] + midWidth / d[3] + midHeight / d[4] + midHeight / d[5]);
    return figure.finish(torsion, {{{h, w}, {0.0, w}, {0.0, 0.0}, {h, 0.0}}});
}

// The finite element formulation of the HEXA hexagon, as the request of the thread asks (doubles only).
void solveHexagon(Figure<double>& figure, std::span<const double> d, ShearFactorRule rule,
                  std::optional<double>& solvedTorsion)
{
    SolverRequest* request = currentRequest;
    static constexpr std::array<std::array<double, 3>, 3> UNIT{{{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}};
    const std::size_t directions =
        request != nullptr && request->wantGradient ? std::min<std::size_t>(request->hexagonDirections, 3) : 0;
    const SectionStiffness solved =
        directions > 0 ? SectionShearSolver::ofHexagon(
                             d[0], d[1], d[2], std::span<const std::array<double, 3>>(UNIT.data(), directions),
                             &request->gradient, request->wantHessian ? request->hessian.get() : nullptr)
                       : SectionShearSolver::ofHexagon(d[0], d[1], d[2]);
    if (request != nullptr)
    {
        request->warpingSolved = true;
        request->stiffnessSolved = rule == ShearFactorRule::Solver;
    }
    figure.setWarping(solved.shearY, solved.shearZ, solved.cw);
    if (rule == ShearFactorRule::Solver)
    {
        figure.setShear(solved.k1, solved.k2, solved.j);
        solvedTorsion = solved.j;
    }
    else
        figure.setShear(5.0 / 6.0, 5.0 / 6.0);
}

// HEXA: a hexagon with a flat top and bottom: DIM1 the horizontal inset of the sloping sides, DIM2 the overall width,
// DIM3 the height. C and D are the middle of the top and the bottom, E and F the points on the right and the left. The
// section is solid: under the beam library rule the shear factors are 5/6 and the torsional constant is A^4 / (40 Ip),
// the approximation for a compact section; under the solver's rule SectionShearSolver gives the three.
template <typename T>
PropertiesOf<T> hexaSection(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 3 || !allPositive(d) || d[0] * 2.0 >= d[1])
        return {};
    const T half = d[1] / 2.0;
    const T top = half - d[0];
    Figure<T> figure(rule);
    figure.polygon(
        {{half, 0.0}, {top, d[2] / 2.0}, {-top, d[2] / 2.0}, {-half, 0.0}, {-top, -d[2] / 2.0}, {top, -d[2] / 2.0}});
    std::optional<T> solvedTorsion;
    if (rule != ShearFactorRule::None && solverSkipped<T>())
    {
        // the closed forms only: what the solver gives is NaN
        const T none = SectionProperties::UNDEFINED;
        figure.setWarping(none, none, none);
        if (rule == ShearFactorRule::Solver)
        {
            figure.setShear(none, none, none);
            solvedTorsion = none;
        }
        else
            figure.setShear(5.0 / 6.0, 5.0 / 6.0);
    }
    else if (rule != ShearFactorRule::None)
    {
        if constexpr (IS_DOUBLE<T>)
            solveHexagon(figure, d, rule, solvedTorsion);
    }
    else
        figure.setShear(5.0 / 6.0, 5.0 / 6.0);
    PropertiesOf<T> result = figure.finish(0.0, {{{d[2] / 2.0, 0.0}, {-d[2] / 2.0, 0.0}, {0.0, half}, {0.0, -half}}});
    result.j = solvedTorsion.value_or(power(result.area, 4) / (40.0 * (result.i1 + result.i2)));
    return result;
}

// HAT: a crown DIM3 wide (over the outer faces of the legs) on two legs, brims DIM4 long on the outside of the legs,
// all of one thickness DIM2, DIM1 high overall. C and F are the top right and left of the crown, D and E the ends of
// the brims. The torsional constant of the beam library is the thin-walled open section sum.
template <typename T>
PropertiesOf<T> hatSection(std::span<const T> d, ShearFactorRule rule)
{
    const T thickness = d.size() == 4 ? d[1] : T(0.0);
    if (d.size() != 4 || !allPositive(d) || d[2] <= 2.0 * thickness || d[0] <= 2.0 * thickness)
        return {};
    const T half = d[2] / 2.0;
    Figure<T> figure(rule);
    // The crown, its two ends over the legs being corners.
    figure.corner(d[0] - thickness, d[0], -half, -half + thickness);
    figure.horizontalWall(d[0] - thickness, d[0], -half + thickness, half - thickness);
    figure.corner(d[0] - thickness, d[0], half - thickness, half);
    figure.verticalWall(thickness, d[0] - thickness, -half, -half + thickness);
    figure.verticalWall(thickness, d[0] - thickness, half - thickness, half);
    // The brims, the foot of each leg being a corner.
    figure.horizontalWall(0.0, thickness, -half - d[3], -half);
    figure.corner(0.0, thickness, -half, -half + thickness);
    figure.corner(0.0, thickness, half - thickness, half);
    figure.horizontalWall(0.0, thickness, half, half + d[3]);
    const T torsion =
        thin(d[2], thickness) + 2.0 * thin(d[0] - 2.0 * thickness, thickness) + 2.0 * thin(d[3] + thickness, thickness);
    return figure.finish(torsion, {{{d[0], half}, {0.0, half + d[3]}, {0.0, -half - d[3]}, {d[0], -half}}});
}

// HAT1: a hat on a base plate: DIM1 the width of the plate, DIM2 the overall height, DIM3 the width of the crown over
// the outer faces of the legs, DIM4 the thickness of the hat, DIM5 the thickness of the plate. C and D are the ends of
// the plate (left, right), E and F the left and right of the crown. The torsional constant of the beam library is
// Bredt's for the closed cell the hat makes with the plate, plus the thin-walled sum for the brims and the overhanging
// plate.
template <typename T>
PropertiesOf<T> hat1Section(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 5 || !allPositive(d) || d[2] <= 2.0 * d[3] || d[0] < d[2] || d[4] + 2.0 * d[3] >= d[1])
        return {};
    const T half = d[2] / 2.0;
    const T plate = d[0] / 2.0;
    const T t = d[3];
    Figure<T> figure(rule);
    // The plate and the brims on it: under each leg the plate and the foot of the leg are corners.
    for (const auto& [y0, y1] : {std::pair<T, T>{0.0, d[4]}, std::pair<T, T>{d[4], d[4] + t}})
    {
        figure.horizontalWall(y0, y1, -plate, -half);
        figure.corner(y0, y1, -half, -half + t);
        figure.corner(y0, y1, half - t, half);
        figure.horizontalWall(y0, y1, half, plate);
    }
    figure.horizontalWall(0.0, d[4], -half + t, half - t);
    figure.verticalWall(d[4] + t, d[1] - t, -half, -half + t);
    figure.verticalWall(d[4] + t, d[1] - t, half - t, half);
    // The crown, its two ends over the legs being corners.
    figure.corner(d[1] - t, d[1], -half, -half + t);
    figure.horizontalWall(d[1] - t, d[1], -half + t, half - t);
    figure.corner(d[1] - t, d[1], half - t, half);
    const T cellWidth = d[2] - t;
    const T cellHeight = d[1] - t / 2.0 - d[4] / 2.0;
    const T cell = closedCell(cellWidth * cellHeight, cellWidth / t + cellWidth / d[4] + 2.0 * cellHeight / t);
    const T overhang = (d[0] - d[2]) / 2.0;
    const T torsion = cell + 2.0 * thin(overhang, t) + 2.0 * thin(overhang, d[4]);
    return figure.finish(torsion, {{{0.0, -plate}, {0.0, plate}, {d[1], -half}, {d[1], half}}});
}

// DBOX: two cells side by side. DIM1 the overall width, DIM2 the overall height, DIM3 the distance from the left face
// to the middle of the central web, DIM4, DIM5, DIM6 the thickness of the left wall, the web, the right wall, DIM7 and
// DIM8 the thickness of the top and bottom of the left cell, DIM9 and DIM10 those of the right cell. C is the bottom
// left, D the bottom right, E the top right, F the top left. The torsional constant of the beam library is the
// two-cell thin-walled solution.
template <typename T>
PropertiesOf<T> dboxSection(std::span<const T> d, ShearFactorRule rule)
{
    if (d.size() != 10 || !allPositive(d))
        return {};
    const T width = d[0];
    const T height = d[1];
    const T webLeft = d[2] - d[4] / 2.0;
    const T webRight = d[2] + d[4] / 2.0;
    if (!(webLeft > d[3]) || !(webRight < width - d[5]) || d[6] + d[7] >= height || d[8] + d[9] >= height)
        return {};
    Figure<T> figure(rule);
    // Each vertical wall is cut where the top and bottom walls meet it: those squares are corners. The web meets the
    // walls of both cells: its corners are as thick as the thicker of the two.
    const auto wall = [&](const T& z0, const T& z1, const T& bottom, const T& top)
    {
        figure.corner(0.0, bottom, z0, z1);
        figure.verticalWall(bottom, height - top, z0, z1);
        figure.corner(height - top, height, z0, z1);
    };
    wall(0.0, d[3], d[7], d[6]);
    wall(webLeft, webRight, std::max(d[7], d[9]), std::max(d[6], d[8]));
    wall(width - d[5], width, d[9], d[8]);
    figure.horizontalWall(height - d[6], height, d[3], webLeft);
    figure.horizontalWall(0.0, d[7], d[3], webLeft);
    figure.horizontalWall(height - d[8], height, webRight, width - d[5]);
    figure.horizontalWall(0.0, d[9], webRight, width - d[5]);
    // The mid-lines of the walls bound the two cells. The central web is as long as the mean of the two cells' heights.
    const T leftWidth = d[2] - d[3] / 2.0;
    const T rightWidth = width - d[5] / 2.0 - d[2];
    const T leftHeight = height - d[6] / 2.0 - d[7] / 2.0;
    const T rightHeight = height - d[8] / 2.0 - d[9] / 2.0;
    const T webLength = (leftHeight + rightHeight) / 2.0;
    const T leftArea = leftWidth * leftHeight;
    const T rightArea = rightWidth * rightHeight;
    const T leftRatio = leftWidth / d[6] + leftWidth / d[7] + leftHeight / d[3] + webLength / d[4];
    const T rightRatio = rightWidth / d[8] + rightWidth / d[9] + rightHeight / d[5] + webLength / d[4];
    const T shared = webLength / d[4];
    // Equal rate of twist in both cells: [leftRatio -shared; -shared rightRatio] q = 2 [leftArea; rightArea].
    const T determinant = leftRatio * rightRatio - shared * shared;
    const T leftFlow = 2.0 * (leftArea * rightRatio + rightArea * shared) / determinant;
    const T rightFlow = 2.0 * (rightArea * leftRatio + leftArea * shared) / determinant;
    const T torsion = 2.0 * (leftArea * leftFlow + rightArea * rightFlow);
    return figure.finish(torsion, {{{0.0, 0.0}, {0.0, width}, {height, width}, {height, 0.0}}});
}

// The section of the library of this type.
template <typename T>
PropertiesOf<T> librarySection(std::string_view type, std::span<const T> dimensions, ShearFactorRule rule)
{
    if (type == "Rod")
        return rodSection<T>(dimensions, rule);
    if (type == "Tube")
        return tubeSection<T>(dimensions, rule);
    if (type == "Tube2")
        return tube2Section<T>(dimensions, rule);
    if (type == "LSection")
        return lSection<T>(dimensions, rule);
    if (type == "ISection")
        return iSection<T>(dimensions, rule);
    if (type == "Channel")
        return chanSection<T>(dimensions, rule);
    if (type == "TSection")
        return tSection<T>(dimensions, rule);
    if (type == "Box")
        return boxSection<T>(dimensions, rule);
    if (type == "Bar")
        return barSection<T>(dimensions, rule);
    if (type == "Cross")
        return crossSection<T>(dimensions, rule);
    if (type == "HSection")
        return hSection<T>(dimensions, rule);
    if (type == "TSection1")
        return t1Section<T>(dimensions, rule);
    if (type == "ISection1")
        return i1Section<T>(dimensions, rule);
    if (type == "Channel1")
        return chan1Section<T>(dimensions, rule);
    if (type == "ZSection")
        return zSection<T>(dimensions, rule);
    if (type == "Channel2")
        return chan2Section<T>(dimensions, rule);
    if (type == "TSection2")
        return t2Section<T>(dimensions, rule);
    if (type == "Box1")
        return box1Section<T>(dimensions, rule);
    if (type == "Hexagon")
        return hexaSection<T>(dimensions, rule);
    if (type == "Hat")
        return hatSection<T>(dimensions, rule);
    if (type == "Hat1")
        return hat1Section<T>(dimensions, rule);
    if (type == "DoubleBoxSection")
        return dboxSection<T>(dimensions, rule);
    return {};
}

// The properties of a section as numbers, in the order of the fields of SectionPropertiesGradient.
constexpr std::size_t FIELD_COUNT = 7 + 8 + 2 + 4 + 2 + 1;

std::array<double, FIELD_COUNT> fieldsOf(const SectionProperties& properties)
{
    std::array<double, FIELD_COUNT> fields{};
    std::size_t at = 0;
    for (const double value :
         {properties.area, properties.i1, properties.i2, properties.i12, properties.j, properties.k1, properties.k2})
        fields[at++] = value;
    for (const auto& point : properties.recovery)
        for (const double value : point)
            fields[at++] = value;
    for (const double value : properties.drawingCentroid)
        fields[at++] = value;
    for (const double value : properties.drawingBounds)
        fields[at++] = value;
    for (const double value : properties.drawingShearCentre)
        fields[at++] = value;
    fields[at] = properties.cw;
    return fields;
}

// The same for a scalar with derivatives.
template <typename T>
std::array<T, FIELD_COUNT> fieldsOf(const ClosedForms<T>& properties)
{
    std::array<T, FIELD_COUNT> fields{};
    std::size_t at = 0;
    for (const T* value : {&properties.area, &properties.i1, &properties.i2, &properties.i12, &properties.j,
                           &properties.k1, &properties.k2})
        fields[at++] = *value;
    for (const auto& point : properties.recovery)
        for (const T& value : point)
            fields[at++] = value;
    for (const T& value : properties.drawingCentroid)
        fields[at++] = value;
    for (const T& value : properties.drawingBounds)
        fields[at++] = value;
    for (const T& value : properties.drawingShearCentre)
        fields[at++] = value;
    fields[at] = properties.cw;
    return fields;
}

// The rows of a gradient (or the matrices of a Hessian), in the same order.
template <typename Holder>
auto rowsOf(Holder& gradient)
{
    using Row = std::remove_reference_t<decltype(gradient.area)>;
    std::array<Row*, FIELD_COUNT> rows{};
    std::size_t at = 0;
    for (Row* row :
         {&gradient.area, &gradient.i1, &gradient.i2, &gradient.i12, &gradient.j, &gradient.k1, &gradient.k2})
        rows[at++] = row;
    for (auto& point : gradient.recovery)
        for (auto& row : point)
            rows[at++] = &row;
    for (auto& row : gradient.drawingCentroid)
        rows[at++] = &row;
    for (auto& row : gradient.drawingBounds)
        rows[at++] = &row;
    for (auto& row : gradient.drawingShearCentre)
        rows[at++] = &row;
    rows[at] = &gradient.cw;
    return rows;
}

// The rectangles of the figure of a section, nothing solved.
std::vector<SectionRectangle> piecesOf(std::string_view type, std::span<const double> dimensions)
{
    SolverRequest request;
    request.mode = SolverRequest::Mode::Capture;
    const RequestScope scope(request);
    (void)librarySection(type, dimensions, ShearFactorRule::Solver);
    return std::move(request.pieces);
}

// The second derivatives of the closed forms with respect to the first `count` dimensions: forward
// over forward through the formulas, every pair of dimensions in one evaluation; symmetrised (the two orders agree to
// rounding).
template <std::size_t N>
void closedHessian(std::string_view type, std::span<const double> dimensions, ShearFactorRule rule, std::size_t count,
                   SectionPropertiesHessian& hessian)
{
    using Number = detail::Hessian<N>;
    std::vector<Number> seeded(dimensions.size());
    for (std::size_t index = 0; index < dimensions.size(); ++index)
    {
        seeded[index] = Number(dimensions[index]);
        if (index < count)
        {
            seeded[index].d[index].v = 1.0;
            seeded[index].v.d[index] = 1.0;
        }
    }
    const auto fields = fieldsOf(librarySection<Number>(type, seeded, rule));
    const auto rows = rowsOf(hessian);
    for (std::size_t field = 0; field < FIELD_COUNT; ++field)
        for (std::size_t i = 0; i < count; ++i)
            for (std::size_t j = 0; j < count; ++j)
                (*rows[field])[i * MAX_SECTION_DIRECTIONS + j] =
                    0.5 * (fields[field].d[i].d[j] + fields[field].d[j].d[i]);
}

} // namespace

SectionProperties SectionFormulas::standard(std::string_view type, std::span<const double> dimensions,
                                            ShearFactorRule rule)
{
    return librarySection(type, dimensions, rule);
}

SectionProperties SectionFormulas::standard(std::string_view type, std::span<const double> dimensions,
                                            ShearFactorRule rule, std::span<const double> reference,
                                            SectionPropertiesGradient* gradient)
{
    return standard(type, dimensions, rule, reference, gradient, nullptr);
}

SectionProperties SectionFormulas::standard(std::string_view type, std::span<const double> dimensions,
                                            ShearFactorRule rule, std::span<const double> reference,
                                            SectionPropertiesGradient* gradient, SectionPropertiesHessian* hessian)
{
    // the second derivatives come with the first ones
    SectionPropertiesGradient ownGradient;
    if (hessian != nullptr && gradient == nullptr)
        gradient = &ownGradient;
    if (gradient == nullptr && (reference.empty() || rule == ShearFactorRule::None))
        return librarySection(type, dimensions, rule);
    const std::size_t count = std::min(dimensions.size(), MAX_SECTION_DIRECTIONS);
    // the plan of the meshes: the reference's, else the section's own
    const std::vector<SectionRectangle> base = piecesOf(type, dimensions);
    const SectionMeshPlan plan =
        SectionShearSolver::planOf(reference.empty() ? std::span<const SectionRectangle>(base)
                                                     : std::span<const SectionRectangle>(piecesOf(type, reference)));
    // the derivatives of the rectangles along each dimension (they are affine in the dimensions)
    std::vector<std::vector<SectionRectangle>> directions;
    if (gradient != nullptr && !base.empty())
    {
        for (std::size_t index = 0; index < count; ++index)
        {
            const double step = 1.0e-5 * std::fabs(dimensions[index]);
            std::vector<double> moved(dimensions.begin(), dimensions.end());
            moved[index] = dimensions[index] + step;
            const std::vector<SectionRectangle> plus = piecesOf(type, moved);
            moved[index] = dimensions[index] - step;
            const std::vector<SectionRectangle> minus = piecesOf(type, moved);
            std::vector<SectionRectangle> direction;
            if (step > 0.0 && plus.size() == base.size() && minus.size() == base.size())
                for (std::size_t piece = 0; piece < base.size(); ++piece)
                    direction.push_back({(plus[piece].y0 - minus[piece].y0) / (2.0 * step),
                                         (plus[piece].y1 - minus[piece].y1) / (2.0 * step),
                                         (plus[piece].z0 - minus[piece].z0) / (2.0 * step),
                                         (plus[piece].z1 - minus[piece].z1) / (2.0 * step)});
            directions.push_back(std::move(direction));
        }
    }
    SolverRequest request;
    request.plan = &plan;
    request.directions = directions;
    request.hexagonDirections = count;
    request.wantGradient = gradient != nullptr;
    request.wantHessian = hessian != nullptr;
    if (request.wantHessian)
        request.hessian = std::make_unique<SectionStiffnessHessian>();
    SectionProperties result;
    {
        const RequestScope scope(request);
        result = librarySection(type, dimensions, rule);
    }
    if (gradient == nullptr)
        return result;
    // the closed forms by central differences, the solver's properties from its derivatives
    *gradient = SectionPropertiesGradient{};
    gradient->count = count;
    const auto rows = rowsOf(*gradient);
    SolverRequest skip;
    skip.mode = SolverRequest::Mode::Skip;
    const RequestScope scope(skip);
    for (std::size_t index = 0; index < count; ++index)
    {
        const double step = 1.0e-3 * std::fabs(dimensions[index]);
        std::array<std::array<double, FIELD_COUNT>, 4> values{};
        static constexpr std::array<double, 4> AT{-2.0, -1.0, 1.0, 2.0};
        static constexpr std::array<double, 4> WEIGHT{1.0, -8.0, 8.0, -1.0};
        std::vector<double> moved(dimensions.begin(), dimensions.end());
        for (std::size_t point = 0; point < 4; ++point)
        {
            moved[index] = dimensions[index] + AT[point] * step;
            values[point] = fieldsOf(librarySection<double>(type, moved, rule));
        }
        for (std::size_t field = 0; field < FIELD_COUNT; ++field)
        {
            double sum = 0.0;
            for (std::size_t point = 0; point < 4; ++point)
                sum += WEIGHT[point] * values[point][field];
            (*rows[field])[index] = step > 0.0 ? sum / (12.0 * step) : SectionProperties::UNDEFINED;
        }
        const SectionStiffnessGradient& solved = request.gradient;
        const bool known = index < solved.count;
        const double none = SectionProperties::UNDEFINED;
        if (request.stiffnessSolved)
        {
            gradient->k1[index] = known ? solved.k1[index] : none;
            gradient->k2[index] = known ? solved.k2[index] : none;
            gradient->j[index] = known ? solved.j[index] : none;
        }
        if (request.warpingSolved)
        {
            gradient->drawingShearCentre[0][index] = known ? solved.shearY[index] : none;
            gradient->drawingShearCentre[1][index] = known ? solved.shearZ[index] : none;
            gradient->cw[index] = known ? solved.cw[index] : none;
        }
    }
    // a property that is NaN has a NaN derivative
    const auto fields = fieldsOf(result);
    for (std::size_t field = 0; field < FIELD_COUNT; ++field)
        if (fields[field] != fields[field])
            rows[field]->fill(SectionProperties::UNDEFINED);
    if (hessian == nullptr)
        return result;
    // the second derivatives: the closed forms through the formulas, the solver's properties from its own
    *hessian = SectionPropertiesHessian{};
    hessian->count = count;
    if (count <= 4)
        closedHessian<4>(type, dimensions, rule, count, *hessian);
    else if (count <= 6)
        closedHessian<6>(type, dimensions, rule, count, *hessian);
    else if (count <= 8)
        closedHessian<8>(type, dimensions, rule, count, *hessian);
    else
        closedHessian<MAX_SECTION_DIRECTIONS>(type, dimensions, rule, count, *hessian);
    const SectionStiffnessHessian& solved = *request.hessian;
    const auto copy = [&](const SectionStiffnessHessian::Matrix& from, SectionPropertiesHessian::Matrix& to)
    {
        for (std::size_t i = 0; i < count; ++i)
            for (std::size_t j = 0; j < count; ++j)
                to[i * MAX_SECTION_DIRECTIONS + j] = i < solved.count && j < solved.count
                                                         ? from[i * MAX_SECTION_DIRECTIONS + j]
                                                         : SectionProperties::UNDEFINED;
    };
    if (request.stiffnessSolved)
    {
        copy(solved.k1, hessian->k1);
        copy(solved.k2, hessian->k2);
        copy(solved.j, hessian->j);
    }
    if (request.warpingSolved)
    {
        copy(solved.shearY, hessian->drawingShearCentre[0]);
        copy(solved.shearZ, hessian->drawingShearCentre[1]);
        copy(solved.cw, hessian->cw);
    }
    const auto matrices = rowsOf(*hessian);
    for (std::size_t field = 0; field < FIELD_COUNT; ++field)
        if (fields[field] != fields[field])
            matrices[field]->fill(SectionProperties::UNDEFINED);
    return result;
}

// ---- the outlines of the library sections
// -----------------------------------------------------------------------------------------------

namespace
{

using Ring = std::vector<std::array<double, 2>>; // (y, z) points

// A ring from (z, y) corners, counter-clockwise in the plane with z to the right and y up for an outline, clockwise
// for a hole.
Ring ringZY(std::initializer_list<std::array<double, 2>> cornersZY)
{
    Ring ring;
    for (const auto& corner : cornersZY)
        ring.push_back({corner[1], corner[0]});
    return ring;
}

// A rectangle z0..z1, y0..y1: counter-clockwise, or clockwise for a hole.
Ring rectangle(double z0, double z1, double y0, double y1, bool hole)
{
    return hole ? ringZY({{z0, y0}, {z0, y1}, {z1, y1}, {z1, y0}}) : ringZY({{z0, y0}, {z1, y0}, {z1, y1}, {z0, y1}});
}

// A circle of radius r about the drawing origin, `CIRCLE_POINTS` points.
Ring circle(double radius, bool hole)
{
    Ring ring;
    for (std::size_t index = 0; index < SectionOutline::CIRCLE_POINTS; ++index)
    {
        const double angle = 2.0 * PI * static_cast<double>(index) / static_cast<double>(SectionOutline::CIRCLE_POINTS);
        const double z = radius * std::cos(hole ? -angle : angle);
        const double y = radius * std::sin(hole ? -angle : angle);
        ring.push_back({y, z});
    }
    return ring;
}

} // namespace

SectionOutline SectionFormulas::outline(std::string_view type, std::span<const double> d)
{
    SectionOutline outline;
    // the same checks as the properties: an impossible section has no outline
    if (!SectionFormulas::standard(type, d, ShearFactorRule::None).hasArea())
        return outline;
    auto& rings = outline.rings;
    auto& holes = outline.holes;
    const auto add = [&](Ring ring, bool hole)
    {
        rings.push_back(std::move(ring));
        holes.push_back(hole ? 1 : 0);
    };
    if (type == "Rod")
        add(circle(d[0], false), false);
    else if (type == "Tube" || type == "Tube2")
    {
        add(circle(d[0], false), false);
        add(circle(type == "Tube" ? d[1] : d[0] - d[1], true), true);
    }
    else if (type == "LSection")
        add(ringZY({{0, 0}, {d[0], 0}, {d[0], d[2]}, {d[3], d[2]}, {d[3], d[1]}, {0, d[1]}}), false);
    else if (type == "ISection")
    {
        const double h = d[0];
        const double w = d[3] / 2.0;
        add(ringZY({{-d[1] / 2, 0},
                    {d[1] / 2, 0},
                    {d[1] / 2, d[4]},
                    {w, d[4]},
                    {w, h - d[5]},
                    {d[2] / 2, h - d[5]},
                    {d[2] / 2, h},
                    {-d[2] / 2, h},
                    {-d[2] / 2, h - d[5]},
                    {-w, h - d[5]},
                    {-w, d[4]},
                    {-d[1] / 2, d[4]}}),
            false);
    }
    else if (type == "Channel" || type == "Channel1")
    {
        // CHAN: DIM1 the width over the flanges, DIM2 the height, DIM3 the web, DIM4 the flanges; CHAN1: flanges DIM1
        // beyond a web DIM2 thick and DIM4 high, each (DIM4 - DIM3) / 2 thick
        const bool first = type == "Channel";
        const double width = first ? d[0] : d[0] + d[1];
        const double h = first ? d[1] : d[3];
        const double web = first ? d[2] : d[1];
        const double flange = first ? d[3] : (d[3] - d[2]) / 2.0;
        add(ringZY({{0, 0},
                    {width, 0},
                    {width, flange},
                    {web, flange},
                    {web, h - flange},
                    {width, h - flange},
                    {width, h},
                    {0, h}}),
            false);
    }
    else if (type == "TSection")
    {
        const double h = d[1];
        add(ringZY({{-d[3] / 2, 0},
                    {d[3] / 2, 0},
                    {d[3] / 2, h - d[2]},
                    {d[0] / 2, h - d[2]},
                    {d[0] / 2, h},
                    {-d[0] / 2, h},
                    {-d[0] / 2, h - d[2]},
                    {-d[3] / 2, h - d[2]}}),
            false);
    }
    else if (type == "Box")
    {
        add(rectangle(0.0, d[0], 0.0, d[1], false), false);
        add(rectangle(d[3], d[0] - d[3], d[2], d[1] - d[2], true), true);
    }
    else if (type == "Bar")
        add(rectangle(0.0, d[0], 0.0, d[1], false), false);
    else if (type == "Cross")
    {
        const double s = d[1] / 2.0;
        const double a = s + d[0] / 2.0;
        const double half = d[2] / 2.0;
        const double t = d[3] / 2.0;
        add(ringZY({{-s, -half},
                    {s, -half},
                    {s, -t},
                    {a, -t},
                    {a, t},
                    {s, t},
                    {s, half},
                    {-s, half},
                    {-s, t},
                    {-a, t},
                    {-a, -t},
                    {-s, -t}}),
            false);
    }
    else if (type == "HSection")
    {
        const double i = d[0] / 2.0;
        const double o = (d[0] + d[1]) / 2.0;
        const double half = d[2] / 2.0;
        const double t = d[3] / 2.0;
        add(ringZY({{-o, -half},
                    {-i, -half},
                    {-i, -t},
                    {i, -t},
                    {i, -half},
                    {o, -half},
                    {o, half},
                    {i, half},
                    {i, t},
                    {-i, t},
                    {-i, half},
                    {-o, half}}),
            false);
    }
    else if (type == "TSection1")
    {
        const double f = d[0] / 2.0;
        const double t = d[3] / 2.0;
        add(ringZY({{0, -f}, {d[2], -f}, {d[2], f}, {0, f}, {0, t}, {-d[1], t}, {-d[1], -t}, {0, -t}}), false);
    }
    else if (type == "ISection1")
    {
        const double f = (d[3] - d[2]) / 2.0;
        const double half = (d[0] + d[1]) / 2.0;
        const double w = d[1] / 2.0;
        const double h = d[3];
        add(ringZY({{-half, 0},
                    {half, 0},
                    {half, f},
                    {w, f},
                    {w, h - f},
                    {half, h - f},
                    {half, h},
                    {-half, h},
                    {-half, h - f},
                    {-w, h - f},
                    {-w, f},
                    {-half, f}}),
            false);
    }
    else if (type == "ZSection")
    {
        const double f = (d[3] - d[2]) / 2.0;
        add(ringZY({{0, 0},
                    {d[1] + d[0], 0},
                    {d[1] + d[0], f},
                    {d[1], f},
                    {d[1], d[3]},
                    {-d[0], d[3]},
                    {-d[0], d[3] - f},
                    {0, d[3] - f}}),
            false);
    }
    else if (type == "Channel2")
        add(ringZY({{0, 0},
                    {d[3], 0},
                    {d[3], d[2]},
                    {d[3] - d[0], d[2]},
                    {d[3] - d[0], d[1]},
                    {d[0], d[1]},
                    {d[0], d[2]},
                    {0, d[2]}}),
            false);
    else if (type == "TSection2")
        add(ringZY({{-d[0] / 2, 0},
                    {d[0] / 2, 0},
                    {d[0] / 2, d[2]},
                    {d[3] / 2, d[2]},
                    {d[3] / 2, d[1]},
                    {-d[3] / 2, d[1]},
                    {-d[3] / 2, d[2]},
                    {-d[0] / 2, d[2]}}),
            false);
    else if (type == "Box1")
    {
        add(rectangle(0.0, d[0], 0.0, d[1], false), false);
        add(rectangle(d[5], d[0] - d[4], d[3], d[1] - d[2], true), true);
    }
    else if (type == "Hexagon")
    {
        const double half = d[1] / 2.0;
        const double top = half - d[0];
        const double h = d[2] / 2.0;
        add(ringZY({{half, 0}, {top, h}, {-top, h}, {-half, 0}, {-top, -h}, {top, -h}}), false);
    }
    else if (type == "Hat")
    {
        const double t = d[1];
        const double half = d[2] / 2.0;
        const double h = d[0];
        add(ringZY({{-half - d[3], 0},
                    {-half + t, 0},
                    {-half + t, h - t},
                    {half - t, h - t},
                    {half - t, 0},
                    {half + d[3], 0},
                    {half + d[3], t},
                    {half, t},
                    {half, h},
                    {-half, h},
                    {-half, t},
                    {-half - d[3], t}}),
            false);
    }
    else if (type == "Hat1")
    {
        const double half = d[2] / 2.0;
        const double plate = d[0] / 2.0;
        const double t = d[3];
        const double base = d[4];
        add(ringZY({{-plate, 0},
                    {plate, 0},
                    {plate, base + t},
                    {half, base + t},
                    {half, d[1]},
                    {-half, d[1]},
                    {-half, base + t},
                    {-plate, base + t}}),
            false);
        add(rectangle(-half + t, half - t, base, d[1] - t, true), true);
    }
    else if (type == "DoubleBoxSection")
    {
        const double width = d[0];
        const double height = d[1];
        add(rectangle(0.0, width, 0.0, height, false), false);
        add(rectangle(d[3], d[2] - d[4] / 2.0, d[7], height - d[6], true), true);
        add(rectangle(d[2] + d[4] / 2.0, width - d[5], d[9], height - d[8], true), true);
    }
    return outline;
}

} // namespace oor::analysis
