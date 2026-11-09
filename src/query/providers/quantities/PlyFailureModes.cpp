#include "PlyJet.hpp"
#include "oor/query/providers/quantities/CompositeQuantities.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numbers>
#include <string>
#include <type_traits>

// The theories of failure modes of a ply under plane stress. Each mode is written as its publication
// writes it; the strength ratio is the factor on the stresses that brings the index of the ply to the limit.

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();
constexpr double DEGREE = std::numbers::pi / 180.0;

// The positive root of q s^2 + l s = limit, the factor of a mode quadratic and linear in the stresses; infinity when
// the mode never reaches the limit along the loading (q and l not positive).
double rootOf(double q, double l, double limit)
{
    if (std::fabs(q) < 1.0e-300)
        return l > 0.0 ? limit / l : std::numeric_limits<double>::infinity();
    const double discriminant = l * l + 4.0 * q * limit;
    if (discriminant < 0.0)
        return std::numeric_limits<double>::infinity();
    const double root = (-l + std::sqrt(discriminant)) / (2.0 * q);
    return root > 0.0 ? root : std::numeric_limits<double>::infinity();
}

// The strengths a theory reads, with the defaults of the card (a blank compressive strength is the tensile one).
struct Strengths
{
    double xt = NOT_A_NUMBER;
    double xc = NOT_A_NUMBER;
    double yt = NOT_A_NUMBER;
    double yc = NOT_A_NUMBER;
    double s = NOT_A_NUMBER;  // in-plane shear
    double st = NOT_A_NUMBER; // transverse shear of the matrix (Hashin)
    [[nodiscard]] bool complete() const noexcept { return xt > 0.0 && xc > 0.0 && yt > 0.0 && yc > 0.0 && s > 0.0; }
};

Strengths strengthsOf(const PlyMaterial& m, bool hashin)
{
    Strengths result;
    if (hashin && m.hashin)
    {
        const std::array<double, 6>& h = *m.hashin;
        result = {h[0], h[1], h[2], h[3], h[4], h[5]};
    }
    else
    {
        result.xt = m.xt;
        result.xc = m.xc > 0.0 ? m.xc : m.xt;
        result.yt = m.yt;
        result.yc = m.yc > 0.0 ? m.yc : m.yt;
        result.s = m.s;
    }
    if (!(result.xc > 0.0))
        result.xc = result.xt;
    if (!(result.yc > 0.0))
        result.yc = result.yt;
    if (!(result.st > 0.0))
        result.st = result.s;
    return result;
}

// ---- Hashin (1980), plane stress: each mode quadratic and linear in the stresses
// -----------------------------------------------------------------

PlyModes hashin(double s1, double s2, double t12, const PlyMaterial& m, double limit)
{
    PlyModes result;
    const Strengths f = strengthsOf(m, true);
    if (!f.complete())
        return result;
    const double shear = (t12 / f.s) * (t12 / f.s);
    double fiberQ = 0.0;
    double matrixQ = 0.0;
    double matrixL = 0.0;
    std::string fiberMode;
    std::string matrixMode;
    if (s1 >= 0.0)
    {
        fiberQ = (s1 / f.xt) * (s1 / f.xt) + shear;
        fiberMode = "fiberTension";
    }
    else
    {
        fiberQ = (s1 / f.xc) * (s1 / f.xc);
        fiberMode = "fiberCompression";
    }
    if (s2 >= 0.0)
    {
        matrixQ = (s2 / f.yt) * (s2 / f.yt) + shear;
        matrixMode = "matrixTension";
    }
    else
    {
        const double half = 2.0 * f.st;
        matrixQ = (s2 / half) * (s2 / half) + shear;
        matrixL = ((f.yc / half) * (f.yc / half) - 1.0) * s2 / f.yc;
        matrixMode = "matrixCompression";
    }
    result.fiber = fiberQ;
    result.matrix = matrixQ + matrixL;
    const bool fiberGoverns = result.fiber >= result.matrix;
    result.index = fiberGoverns ? result.fiber : result.matrix;
    result.mode = fiberGoverns ? fiberMode : matrixMode;
    const double ratio = std::min(rootOf(fiberQ, 0.0, limit), rootOf(matrixQ, matrixL, limit));
    result.ratio = std::isinf(ratio) ? NOT_A_NUMBER : ratio;
    return result;
}

// ---- Puck (Puck and Schurmann 2002, VDI 2014), plane stress: stress exposures, linear in the stresses
// --------------------------------------------

PlyModes puck(double s1, double s2, double t12, const PlyMaterial& m, double limit)
{
    PlyModes result;
    const Strengths f = strengthsOf(m, false);
    const double pt = m.puckTension;
    const double pc = m.puckCompression;
    if (!f.complete() || !(pt >= 0.0) || !(pc > 0.0))
        return result;
    // the fibre fracture
    result.fiber = s1 >= 0.0 ? s1 / f.xt : -s1 / f.xc;
    const std::string fiberMode = s1 >= 0.0 ? "fiberTension" : "fiberCompression";
    // the inter-fibre fracture: the resistance of the action plane to transverse compression and the shear where the
    // modes B and C meet
    const double resistance = f.s / (2.0 * pc) * (std::sqrt(1.0 + 2.0 * pc * f.yc / f.s) - 1.0);
    const double pcc = pc * resistance / f.s; // p(-) perpendicular-perpendicular
    const double shearC = f.s * std::sqrt(1.0 + 2.0 * pcc);
    const double t = std::fabs(t12);
    std::string matrixMode;
    if (s2 >= 0.0)
    {
        const double a = 1.0 - pt * f.yt / f.s;
        result.matrix = std::sqrt((t / f.s) * (t / f.s) + a * a * (s2 / f.yt) * (s2 / f.yt)) + pt * s2 / f.s;
        matrixMode = "iffA";
    }
    else if (t > 0.0 && std::fabs(s2 / t) <= resistance / shearC)
    {
        result.matrix = (std::sqrt(t * t + (pc * s2) * (pc * s2)) + pc * s2) / f.s;
        matrixMode = "iffB";
    }
    else
    {
        const double term = t / (2.0 * (1.0 + pcc) * f.s);
        result.matrix = (term * term + (s2 / f.yc) * (s2 / f.yc)) * f.yc / -s2;
        matrixMode = "iffC";
    }
    const bool fiberGoverns = result.fiber >= result.matrix;
    result.index = fiberGoverns ? result.fiber : result.matrix;
    result.mode = fiberGoverns ? fiberMode : matrixMode;
    // the exposures scale with the stresses
    result.ratio = result.index > 0.0 ? limit / result.index : NOT_A_NUMBER;
    return result;
}

// ---- LaRC03 (Davila, Camanho and Rose 2005), plane stress
// ----------------------------------------------------------------------------------------
//
// The matrix under transverse compression fails on the plane whose angle alpha gives the largest
//   f(alpha) = (<tauT> / ST)^2 + (<tauL> / SL)^2,
//   tauT = -s2 cos a (sin a - etaT cos a), tauL = cos a (|t| + etaL s2 cos a).
// f is a trigonometric polynomial of alpha of degree four (with the kinks of the positive parts): its maxima are where
// f' goes from positive to negative between two angles of a grid of PLANE_STEPS + 1 angles over [0, 90] degrees (their
// sines and cosines computed once), each refined by a Newton search kept within its bracket, and alpha = 0 when f
// decreases from it. The derivatives of the index are those of f at the angle found (the envelope theorem: f' = 0
// there, or the angle stays at the bound), the second ones with the angle's own term, H - f_sa f_as / f_aa. Every
// formula is written once, for numbers and for jets (PlyJet.hpp), so the derivatives are those of the values. The
// strength ratio is the root of I(R sigma) = limit, by a Newton search kept within a bracket (dI/dR along sigma from a
// jet), its derivatives the implicit ones of that equation.

struct LarcConstants
{
    double etaL = 0.0; // the coefficients of longitudinal and transverse influence
    double etaT = 0.0;
    double st = 0.0;   // the transverse shear strength from Yc and the fracture angle
    double phiC = 0.0; // the misalignment angle of the fibres under pure longitudinal compression
};

LarcConstants larcConstants(const Strengths& f, double fractureAngle)
{
    const double alpha = fractureAngle * DEGREE;
    LarcConstants c;
    c.etaL = -f.s * std::cos(2.0 * alpha) / (f.yc * std::cos(alpha) * std::cos(alpha));
    c.etaT = -1.0 / std::tan(2.0 * alpha);
    c.st = f.yc * std::cos(alpha) * (std::sin(alpha) + std::cos(alpha) / std::tan(2.0 * alpha));
    const double ratio = f.s / f.xc;
    const double sum = ratio + c.etaL;
    const double root = 1.0 - 4.0 * sum * ratio;
    c.phiC = root < 0.0 ? NOT_A_NUMBER : std::atan((1.0 - std::sqrt(root)) / (2.0 * sum));
    return c;
}

constexpr std::size_t PLANE_STEPS = 24; // 3.75 degrees

// The angles of the grid of the fracture planes, their cosines and sines, and those of twice them.
struct PlaneGrid
{
    std::array<double, PLANE_STEPS + 1> angle{};
    std::array<double, PLANE_STEPS + 1> cosine{};
    std::array<double, PLANE_STEPS + 1> sine{};
    std::array<double, PLANE_STEPS + 1> cosine2{};
    std::array<double, PLANE_STEPS + 1> sine2{};
};

const PlaneGrid& planeGrid()
{
    static const PlaneGrid GRID = []
    {
        PlaneGrid grid;
        for (std::size_t step = 0; step <= PLANE_STEPS; ++step)
        {
            const double angle = 0.5 * std::numbers::pi * static_cast<double>(step) / static_cast<double>(PLANE_STEPS);
            grid.angle[step] = angle;
            grid.cosine[step] = step == PLANE_STEPS ? 0.0 : std::cos(angle);
            grid.sine[step] = std::sin(angle);
            grid.cosine2[step] = std::cos(2.0 * angle);
            grid.sine2[step] = step == PLANE_STEPS ? 0.0 : std::sin(2.0 * angle);
        }
        return grid;
    }();
    return GRID;
}

// f and its first two derivatives in the angle, of a = -s2 and b = |t|, at an angle of cosine c and sine s (c2, s2:
// those of twice the angle).
struct PlaneValue
{
    double f = 0.0;
    double first = 0.0;
    double second = 0.0;
};

PlaneValue planeAt(double c, double s, double c2, double s2, double a, double b, const Strengths& f,
                   const LarcConstants& k)
{
    const double g = c * s - k.etaT * c * c; // tauT = a g
    const double dg = c2 + k.etaT * s2;
    const double ddg = -2.0 * s2 + 2.0 * k.etaT * c2;
    const double h = b * c - k.etaL * a * c * c; // tauL
    const double dh = -b * s + k.etaL * a * s2;
    const double ddh = -b * c + 2.0 * k.etaL * a * c2;
    const double tauT = a * g;
    PlaneValue value;
    if (tauT > 0.0)
    {
        const double st2 = k.st * k.st;
        value.f += tauT * tauT / st2;
        value.first += 2.0 * tauT * a * dg / st2;
        value.second += 2.0 * (a * a * dg * dg + tauT * a * ddg) / st2;
    }
    if (h > 0.0)
    {
        const double sl2 = f.s * f.s;
        value.f += h * h / sl2;
        value.first += 2.0 * h * dh / sl2;
        value.second += 2.0 * (dh * dh + h * ddh) / sl2;
    }
    return value;
}

PlaneValue planeAt(double angle, double a, double b, const Strengths& f, const LarcConstants& k)
{
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    return planeAt(c, s, c * c - s * s, 2.0 * s * c, a, b, f, k);
}

// The fracture plane of the largest index of a transverse stress `sigma` and a shear `tau`: the index, the angle and
// whether it lies inside (0, 90) degrees (f' = 0) or at alpha = 0.
struct PlaneMaximum
{
    double value = 0.0;
    double angle = 0.0;
    bool interior = false;
};

PlaneMaximum fracturePlane(double sigma, double tau, const Strengths& f, const LarcConstants& k)
{
    const double a = -sigma;
    const double b = std::fabs(tau);
    const PlaneGrid& grid = planeGrid();
    PlaneMaximum best;
    PlaneValue before = planeAt(grid.cosine[0], grid.sine[0], grid.cosine2[0], grid.sine2[0], a, b, f, k);
    if (before.f > best.value)
        best.value = before.f;
    for (std::size_t step = 1; step <= PLANE_STEPS; ++step)
    {
        const PlaneValue next =
            planeAt(grid.cosine[step], grid.sine[step], grid.cosine2[step], grid.sine2[step], a, b, f, k);
        if (before.first > 0.0 && !(next.first > 0.0))
        {
            // a maximum within the bracket: Newton on f' = 0, a bisection where its step leaves the bracket
            double low = grid.angle[step - 1];
            double high = grid.angle[step];
            double angle = 0.5 * (low + high);
            PlaneValue at = planeAt(angle, a, b, f, k);
            for (int iteration = 0; iteration < 60; ++iteration)
            {
                if (at.first > 0.0)
                    low = angle;
                else
                    high = angle;
                double following = at.second < 0.0 ? angle - at.first / at.second : NOT_A_NUMBER;
                if (!(following > low && following < high))
                    following = 0.5 * (low + high);
                const bool converged = std::fabs(following - angle) <= 1.0e-13 || high - low <= 1.0e-15;
                angle = following;
                at = planeAt(angle, a, b, f, k);
                if (converged)
                    break;
            }
            if (at.f > best.value)
                best = PlaneMaximum{at.f, angle, true};
        }
        before = next;
    }
    return best;
}

// The angle of the fracture plane as a number of the type of the stresses: a constant, or the fourth variable of a
// jet of second derivatives (whose own term the derivatives of the index take out).
template <class T>
T angleOf(double angle)
{
    if constexpr (std::is_same_v<T, PlyJet<4, true>>)
        return T::variable(angle, 3);
    else
        return T(angle);
}

// What a LaRC03 evaluation found: the indices of the two modes, which one governs, the branches of their signs, and
// whether the matrix index is that of a fracture plane inside (0, 90) degrees.
template <class T>
struct LarcModes
{
    T fiber{};
    T matrix{};
    bool fiberGoverns = true;
    bool fiberTension = true;
    bool matrixTension = true;
    bool interiorPlane = false;
    [[nodiscard]] const T& index() const { return fiberGoverns ? fiber : matrix; }
};

// The stresses in the frame of the fibre misalignment, for the fibre kinking and the matrix under high compression.
template <class T>
void misalignedStresses(const T& s1, const T& s2, const T& t12, const PlyMaterial& m, const Strengths& f,
                        const LarcConstants& c, T& transverse, T& shear)
{
    using std::cos;
    using std::sin;
    const T shear12 = magnitude(t12);
    const T phi = (shear12 + (m.g12 - f.xc) * c.phiC) / (m.g12 + s1 - s2);
    const T sn = sin(phi);
    const T cs = cos(phi);
    transverse = s1 * sn * sn + s2 * cs * cs - 2.0 * shear12 * sn * cs;
    shear = -(s1 - s2) * sn * cs + shear12 * (cs * cs - sn * sn);
}

// The fracture plane last found for a pair (sigma, |tau|): f(alpha) of (k sigma, k tau) is k^2 f(alpha) of (sigma,
// tau), so a pair proportional to it has the same plane (the search of the ratio scales the stresses).
struct PlaneCache
{
    double sigma = NOT_A_NUMBER;
    double tau = NOT_A_NUMBER;
    PlaneMaximum found;
};

template <class T>
LarcModes<T> larcModes(const T& s1, const T& s2, const T& t12, const PlyMaterial& m, const Strengths& f,
                       const LarcConstants& c, PlaneCache* cache = nullptr)
{
    using std::cos;
    using std::sin;
    const double g = m.toughnessRatio;
    LarcModes<T> result;
    const auto tensionIndex = [&](const T& transverse, const T& shear) -> T
    {
        return (1.0 - g) * transverse / f.yt + g * (transverse / f.yt) * (transverse / f.yt) +
               (shear / f.s) * (shear / f.s);
    };
    const auto misaligned = [&](T& transverse, T& shear)
    { misalignedStresses(s1, s2, t12, m, f, c, transverse, shear); };
    // the matrix index on the fracture plane of the largest index
    const auto plane = [&](const T& sigma, const T& tau) -> T
    {
        const double a = valueOf(sigma);
        const double b = std::fabs(valueOf(tau));
        PlaneMaximum found;
        if (cache != nullptr && a < 0.0 && cache->sigma < 0.0 &&
            std::fabs(a * cache->tau - b * cache->sigma) <= 1.0e-14 * std::fabs(a * cache->sigma))
            found = cache->found; // proportional to the pair before (to rounding): the same plane
        else
        {
            found = fracturePlane(a, b, f, c);
            if (cache != nullptr)
                *cache = PlaneCache{a, b, found};
        }
        result.interiorPlane = found.interior;
        const T alpha = angleOf<T>(found.angle);
        const T cs = cos(alpha);
        const T sn = sin(alpha);
        const T transverse = positivePart(-sigma * cs * (sn - c.etaT * cs));
        const T longitudinal = positivePart(cs * (magnitude(tau) + c.etaL * sigma * cs));
        return (transverse / c.st) * (transverse / c.st) + (longitudinal / f.s) * (longitudinal / f.s);
    };
    if (valueOf(s1) >= 0.0)
        result.fiber = s1 / f.xt;
    else
    {
        result.fiberTension = false;
        T transverse{};
        T shear{};
        misaligned(transverse, shear);
        result.fiber = valueOf(transverse) < 0.0 ? positivePart((magnitude(shear) + c.etaL * transverse) / f.s)
                                                 : tensionIndex(transverse, shear);
    }
    if (valueOf(s2) >= 0.0)
        result.matrix = tensionIndex(s2, t12);
    else
    {
        result.matrixTension = false;
        if (valueOf(s1) >= -f.yc)
            result.matrix = plane(s2, t12);
        else
        {
            T transverse{};
            T shear{};
            misaligned(transverse, shear);
            result.matrix = valueOf(transverse) < 0.0 ? plane(transverse, shear) : tensionIndex(transverse, shear);
        }
    }
    result.fiberGoverns = valueOf(result.fiber) >= valueOf(result.matrix);
    return result;
}

// The strengths and the constants of LaRC03 for a material; false when it lacks one.
bool larcInputs(const PlyMaterial& m, Strengths& f, LarcConstants& c)
{
    f = strengthsOf(m, false);
    if (!f.complete() || !(m.g12 > 0.0) || !(m.toughnessRatio >= 0.0) ||
        !(m.fractureAngle > 0.0 && m.fractureAngle < 90.0))
        return false;
    c = larcConstants(f, m.fractureAngle);
    return true;
}

PlyModes larc03Index(double s1, double s2, double t12, const PlyMaterial& m)
{
    PlyModes result;
    Strengths f;
    LarcConstants c;
    if (!larcInputs(m, f, c))
        return result;
    const LarcModes<double> modes = larcModes(s1, s2, t12, m, f, c);
    if (std::isnan(modes.fiber) || std::isnan(modes.matrix))
        return result;
    result.fiber = modes.fiber;
    result.matrix = modes.matrix;
    result.index = modes.index();
    result.mode = modes.fiberGoverns ? (modes.fiberTension ? "fiberTension" : "fiberKinking")
                                     : (modes.matrixTension ? "matrixTension" : "matrixCompression");
    return result;
}

// The factor R on the stresses that brings the index to `limit` (the index growing along the loading): Newton on
// I(R sigma) = limit from sqrt(limit / I), the bracket doubled until it holds the root, a bisection where a step leaves
// it. NaN when the index never reaches the limit. The index of LaRC03 jumps where a branch changes along the loading
// (s1 = -Yc, the transverse stress of the misaligned frame at zero): `atSwitch` is set when the root is such a jump.
double larcRatio(double s1, double s2, double t12, const PlyMaterial& m, const Strengths& f, const LarcConstants& c,
                 double limit, double index, bool* atSwitch = nullptr)
{
    using Jet = PlyJet<1, false>;
    PlaneCache cache;
    const auto at = [&](double scale, double& slope)
    {
        const LarcModes<Jet> modes = larcModes(Jet::variable(scale * s1, 0, s1), Jet::variable(scale * s2, 0, s2),
                                               Jet::variable(scale * t12, 0, t12), m, f, c, &cache);
        slope = modes.index().g[0];
        return modes.index().v;
    };
    double scale = index > 0.0 ? std::sqrt(limit / index) : 1.0;
    double slope = 0.0;
    double value = at(scale, slope);
    double low = 0.0;
    double high = std::numeric_limits<double>::infinity();
    int doublings = 0;
    for (int iteration = 0; iteration < 200; ++iteration)
    {
        if (std::isnan(value))
            return NOT_A_NUMBER;
        if (value >= limit)
            high = scale;
        else
            low = scale;
        double following = 0.0;
        if (std::isinf(high))
        {
            if (++doublings > 80)
                return NOT_A_NUMBER;
            following = 2.0 * scale;
        }
        else
        {
            following = slope > 0.0 ? scale - (value - limit) / slope : NOT_A_NUMBER;
            if (!(following > low && following < high))
                following = 0.5 * (low + high);
            if (std::fabs(value - limit) <= 1.0e-15 * limit || std::fabs(following - scale) <= 1.0e-15 * scale)
                break;
        }
        scale = following;
        value = at(scale, slope);
    }
    if (atSwitch != nullptr)
        *atSwitch = !(std::fabs(value - limit) <= 1.0e-9 * limit);
    return scale;
}

PlyModes larc03(double s1, double s2, double t12, const PlyMaterial& m, double limit)
{
    PlyModes result = larc03Index(s1, s2, t12, m);
    if (std::isnan(result.index))
        return result;
    Strengths f;
    LarcConstants c;
    (void)larcInputs(m, f, c);
    result.ratio = larcRatio(s1, s2, t12, m, f, c, limit, result.index);
    return result;
}

// The gradient (and with `Second`, the second derivatives) of the LaRC03 index at (s1, s2, t12): those of the
// governing mode, the angle of the fracture plane taken out by the envelope theorem.
template <bool Second>
void larcIndexDerivatives(double s1, double s2, double t12, const PlyMaterial& m, const Strengths& f,
                          const LarcConstants& c, std::array<double, 3>& gradient, std::array<double, 9>* hessian)
{
    using Jet = PlyJet<Second ? 4 : 3, Second>;
    const LarcModes<Jet> modes = larcModes(Jet::variable(s1, 0), Jet::variable(s2, 1), Jet::variable(t12, 2), m, f, c);
    const Jet& index = modes.index();
    for (std::size_t k = 0; k < 3; ++k)
        gradient[k] = index.g[k];
    if constexpr (Second)
    {
        const double angleAngle = index.second(3, 3);
        const bool reduce = !modes.fiberGoverns && modes.interiorPlane && angleAngle < 0.0;
        for (std::size_t i = 0; i < 3; ++i)
            for (std::size_t j = i; j < 3; ++j)
            {
                double value = index.second(i, j);
                if (reduce)
                    value -= index.second(i, 3) * index.second(3, j) / angleAngle;
                (*hessian)[3 * i + j] = (*hessian)[3 * j + i] = value;
            }
    }
}

} // namespace

PlyGradient larc03Derivatives(double s1, double s2, double t12, const PlyMaterial& m, double limit, PlyHessian* hessian)
{
    PlyGradient result;
    const PlyModes modes = larc03Index(s1, s2, t12, m);
    if (std::isnan(modes.index))
        return result;
    Strengths f;
    LarcConstants c;
    (void)larcInputs(m, f, c);
    std::array<double, 9> curvature{};
    if (hessian != nullptr)
    {
        larcIndexDerivatives<true>(s1, s2, t12, m, f, c, result.index, &curvature);
        hessian->index = curvature;
    }
    else
        larcIndexDerivatives<false>(s1, s2, t12, m, f, c, result.index, nullptr);
    bool atSwitch = false;
    const double r = larcRatio(s1, s2, t12, m, f, c, limit, modes.index, &atSwitch);
    if (std::isnan(r))
        return result;
    // the implicit derivatives of G(R sigma) = 0, at x = R sigma: R_j = -R G_j / D with D = G_i sigma_i, and
    // R_jk = -(v_k' H v_j + G_j R_k + G_k R_j) / D with v_j = R e_j + sigma R_j; G the index minus the limit, or at a
    // jump of the index the function of the switch (s1 + Yc, the transverse stress of the misaligned frame)
    const std::array<double, 3> sigma{s1, s2, t12};
    std::array<double, 3> gradientAt{};
    std::array<double, 9> curvatureAt{};
    if (atSwitch)
    {
        using Jet = PlyJet<3, true>;
        Jet transverse{};
        Jet shear{};
        misalignedStresses(Jet::variable(r * s1, 0), Jet::variable(r * s2, 1), Jet::variable(r * t12, 2), m, f, c,
                           transverse, shear);
        // the switch nearer to its zero: s1 = -Yc (the matrix under compression), else the misaligned transverse stress
        const bool yield = s1 < 0.0 && s2 < 0.0 && std::fabs(r * s1 + f.yc) <= 1.0e-9 * f.yc;
        gradientAt = yield ? std::array<double, 3>{1.0, 0.0, 0.0} : transverse.g;
        for (std::size_t i = 0; i < 3; ++i)
            for (std::size_t j = 0; j < 3; ++j)
                curvatureAt[3 * i + j] = yield ? 0.0 : transverse.second(i, j);
    }
    else if (hessian != nullptr)
        larcIndexDerivatives<true>(r * s1, r * s2, r * t12, m, f, c, gradientAt, &curvatureAt);
    else
        larcIndexDerivatives<false>(r * s1, r * s2, r * t12, m, f, c, gradientAt, nullptr);
    const double along = gradientAt[0] * s1 + gradientAt[1] * s2 + gradientAt[2] * t12;
    if (!(along != 0.0))
        return result;
    for (std::size_t j = 0; j < 3; ++j)
        result.ratio[j] = -r * gradientAt[j] / along;
    if (hessian == nullptr)
        return result;
    std::array<std::array<double, 3>, 3> v{};
    for (std::size_t j = 0; j < 3; ++j)
        for (std::size_t i = 0; i < 3; ++i)
            v[j][i] = (i == j ? r : 0.0) + sigma[i] * result.ratio[j];
    for (std::size_t j = 0; j < 3; ++j)
        for (std::size_t k = j; k < 3; ++k)
        {
            double quadratic = 0.0;
            for (std::size_t i = 0; i < 3; ++i)
                for (std::size_t l = 0; l < 3; ++l)
                    quadratic += curvatureAt[3 * i + l] * v[k][l] * v[j][i];
            hessian->ratio[3 * j + k] = hessian->ratio[3 * k + j] =
                -(quadratic + gradientAt[j] * result.ratio[k] + gradientAt[k] * result.ratio[j]) / along;
        }
    return result;
}

bool isModeTheory(const std::string& theory) noexcept
{
    return theory == "hashin" || theory == "puck" || theory == "larc03";
}

PlyModes plyFailureModes(const std::string& theory, double s1, double s2, double t12, const PlyMaterial& material,
                         double limit)
{
    if (std::isnan(s1) || std::isnan(s2) || std::isnan(t12))
        return {};
    if (theory == "hashin")
        return hashin(s1, s2, t12, material, limit);
    if (theory == "puck")
        return puck(s1, s2, t12, material, limit);
    if (theory == "larc03")
        return larc03(s1, s2, t12, material, limit);
    return {};
}

PlyModes plyModeIndices(const std::string& theory, double s1, double s2, double t12, const PlyMaterial& material)
{
    if (theory != "larc03")
        return plyFailureModes(theory, s1, s2, t12, material, 1.0);
    if (std::isnan(s1) || std::isnan(s2) || std::isnan(t12))
        return {};
    return larc03Index(s1, s2, t12, material);
}

double plyStrengthRatio(const std::string& theory, double s1, double s2, double t12, const PlyMaterial& material,
                        double limit)
{
    if (isModeTheory(theory))
        return plyFailureModes(theory, s1, s2, t12, material, limit).ratio;
    const PlyIndex index = plyFailureIndex(theory, s1, s2, t12, material);
    return std::isnan(index.index) ? NOT_A_NUMBER : scaleToLimit(index, limit);
}

} // namespace oor::query
