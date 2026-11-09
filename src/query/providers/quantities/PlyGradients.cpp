#include "oor/query/providers/quantities/CompositeQuantities.hpp"

#include <cmath>
#include <limits>

// The derivatives of the failure index and of the strength ratio of a ply in its three stresses.
// Every theory is, around a state of stress, one smooth piece: the branch its signs choose (tension or compression),
// the component or the mode that governs. The derivatives are those of that piece, in closed form:
//   a theory of one index, I = Q + L with Q quadratic and L linear in the stresses (Hill, Hoffman, Tsai-Wu, and each
//   mode of Hashin): grad I = grad Q + grad L; the ratio R, the root of R^2 Q + R L = limit, has
//   grad R = -(R^2 grad Q + R grad L) / (2 R Q + L) (the implicit derivative of I(R sigma) = limit);
//   an index of degree one (max stress, max strain, Puck's exposures): R = limit / I, grad R = -R grad I / I.
// At a switch (a tie of two components or modes, a stress at zero where a branch changes) the derivative is that of the
// piece the value takes (the first component, the fibre before the matrix, tension at zero): one side's. LaRC03's index
// is the largest over the angle of the fracture plane: its derivatives are those at the angle found (the envelope
// theorem), by jets of the formulas of its values, and those of its ratio the implicit ones (larc03Derivatives,
// PlyFailureModes.cpp).
//
// The second derivatives are those of the same piece, in closed form:
//   an index Q + L: its Hessian is that of Q (constant on the piece); the ratio, from F(R, sigma) = R^2 Q + R L - limit
//   = 0 differentiated twice,
//   R_ij = -(2 Q R_i R_j + (2 R Q_j + L_j) R_i + (2 R Q_i + L_i) R_j + R^2 Q_ij) / (2 R Q + L);
//   an index of degree one: its Hessian (zero for max stress, max strain and Puck's fibre, that of the square root of
//   a quadratic form for Puck's exposures A and B, of t^2 / s2 for C), and the ratio limit / I,
//   R_ij = -R I_ij / I + 2 R I_i I_j / I^2.
// One triangle is computed and mirrored: the Hessians are symmetric exactly.

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();
using Vector = std::array<double, 3>;
using Matrix = std::array<double, 9>; // 3 x 3, row-major

constexpr Vector UNDEFINED{NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER};

// The symmetric matrix whose upper triangle `entry(i, j)` (i <= j) gives, mirrored.
template <class Entry>
Matrix symmetric(Entry entry)
{
    Matrix result{};
    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t j = i; j < 3; ++j)
            result[3 * i + j] = result[3 * j + i] = entry(i, j);
    return result;
}

Matrix diagonal(double a, double b, double c)
{
    return {a, 0.0, 0.0, 0.0, b, 0.0, 0.0, 0.0, c};
}

double signOf(double value)
{
    return value >= 0.0 ? 1.0 : -1.0;
}

// The derivatives of an index Q + L (Q quadratic, L linear) and of its ratio to the limit; with `hessian`, also the
// second derivatives (`hessQ` the Hessian of Q).
PlyGradient quadraticPiece(double q, double l, const Vector& gradQ, const Vector& gradL, double ratio,
                           const Matrix& hessQ = {}, PlyHessian* hessian = nullptr)
{
    PlyGradient result;
    for (std::size_t k = 0; k < 3; ++k)
        result.index[k] = gradQ[k] + gradL[k];
    if (hessian != nullptr)
        hessian->index = hessQ;
    if (std::isnan(ratio))
        return result;
    const double denominator = 2.0 * ratio * q + l;
    for (std::size_t k = 0; k < 3; ++k)
        result.ratio[k] = -(ratio * ratio * gradQ[k] + ratio * gradL[k]) / denominator;
    if (hessian != nullptr)
    {
        const Vector& r = result.ratio;
        hessian->ratio = symmetric(
            [&](std::size_t i, std::size_t j)
            {
                return -(2.0 * q * r[i] * r[j] + (2.0 * ratio * gradQ[j] + gradL[j]) * r[i] +
                         (2.0 * ratio * gradQ[i] + gradL[i]) * r[j] + ratio * ratio * hessQ[3 * i + j]) /
                       denominator;
            });
    }
    return result;
}

// The derivatives of an index of degree one in the stresses and of its ratio limit / index; with `hessian`, also the
// second derivatives (`hessI` the Hessian of the index).
PlyGradient linearPiece(const Vector& gradI, double index, double ratio, const Matrix& hessI = {},
                        PlyHessian* hessian = nullptr)
{
    PlyGradient result;
    result.index = gradI;
    if (hessian != nullptr)
    {
        // an index whose gradient is undefined has no Hessian either
        const bool defined = !std::isnan(gradI[0]) && !std::isnan(gradI[1]) && !std::isnan(gradI[2]);
        hessian->index = defined ? hessI : PlyHessian{}.index;
    }
    if (std::isnan(ratio) || !(index != 0.0))
        return result;
    for (std::size_t k = 0; k < 3; ++k)
        result.ratio[k] = -ratio * gradI[k] / index;
    if (hessian != nullptr)
    {
        const Matrix& h = hessian->index;
        hessian->ratio =
            symmetric([&](std::size_t i, std::size_t j)
                      { return -ratio * h[3 * i + j] / index + 2.0 * ratio * gradI[i] * gradI[j] / (index * index); });
    }
    return result;
}

// Hashin's modes (their Q and L, and the derivatives), as `hashin` in PlyFailureModes.cpp writes them.
struct Mode
{
    double q = 0.0;
    double l = 0.0;
    Vector gradQ{};
    Vector gradL{};
    Matrix hessQ{};
};

PlyGradient hashinGradient(double s1, double s2, double t12, const PlyMaterial& m, double limit, const PlyModes& modes,
                           PlyHessian* hessian)
{
    double xt = m.xt, xc = m.xc > 0.0 ? m.xc : m.xt, yt = m.yt, yc = m.yc > 0.0 ? m.yc : m.yt, s = m.s,
           st = NOT_A_NUMBER;
    if (m.hashin)
    {
        const std::array<double, 6>& h = *m.hashin;
        xt = h[0];
        xc = h[1];
        yt = h[2];
        yc = h[3];
        s = h[4];
        st = h[5];
    }
    if (!(xc > 0.0))
        xc = xt;
    if (!(yc > 0.0))
        yc = yt;
    if (!(st > 0.0))
        st = s;
    Mode fiber;
    Mode matrix;
    const double shear = (t12 / s) * (t12 / s);
    const double dShear = 2.0 * t12 / (s * s);
    if (s1 >= 0.0)
    {
        fiber.q = (s1 / xt) * (s1 / xt) + shear;
        fiber.gradQ = {2.0 * s1 / (xt * xt), 0.0, dShear};
        fiber.hessQ = diagonal(2.0 / (xt * xt), 0.0, 2.0 / (s * s));
    }
    else
    {
        fiber.q = (s1 / xc) * (s1 / xc);
        fiber.gradQ = {2.0 * s1 / (xc * xc), 0.0, 0.0};
        fiber.hessQ = diagonal(2.0 / (xc * xc), 0.0, 0.0);
    }
    if (s2 >= 0.0)
    {
        matrix.q = (s2 / yt) * (s2 / yt) + shear;
        matrix.gradQ = {0.0, 2.0 * s2 / (yt * yt), dShear};
        matrix.hessQ = diagonal(0.0, 2.0 / (yt * yt), 2.0 / (s * s));
    }
    else
    {
        const double half = 2.0 * st;
        matrix.q = (s2 / half) * (s2 / half) + shear;
        matrix.gradQ = {0.0, 2.0 * s2 / (half * half), dShear};
        matrix.hessQ = diagonal(0.0, 2.0 / (half * half), 2.0 / (s * s));
        const double factor = ((yc / half) * (yc / half) - 1.0) / yc;
        matrix.l = factor * s2;
        matrix.gradL = {0.0, factor, 0.0};
    }
    PlyGradient result;
    // the index: the mode `hashin` gives (the fibre on a tie)
    const Mode& byIndex = modes.fiber >= modes.matrix ? fiber : matrix;
    for (std::size_t k = 0; k < 3; ++k)
        result.index[k] = byIndex.gradQ[k] + byIndex.gradL[k];
    if (hessian != nullptr)
        hessian->index = byIndex.hessQ;
    // the ratio: the mode of the smaller root (the fibre on a tie, as std::min takes it)
    if (std::isnan(modes.ratio))
        return result;
    const auto rootOf = [&](const Mode& mode)
    {
        if (std::fabs(mode.q) < 1.0e-300)
            return mode.l > 0.0 ? limit / mode.l : std::numeric_limits<double>::infinity();
        const double discriminant = mode.l * mode.l + 4.0 * mode.q * limit;
        if (discriminant < 0.0)
            return std::numeric_limits<double>::infinity();
        const double root = (-mode.l + std::sqrt(discriminant)) / (2.0 * mode.q);
        return root > 0.0 ? root : std::numeric_limits<double>::infinity();
    };
    const Mode& byRatio = !(rootOf(matrix) < rootOf(fiber)) ? fiber : matrix;
    if (hessian == nullptr)
    {
        result.ratio = quadraticPiece(byRatio.q, byRatio.l, byRatio.gradQ, byRatio.gradL, modes.ratio).ratio;
        return result;
    }
    PlyHessian byRatioHessian;
    result.ratio =
        quadraticPiece(byRatio.q, byRatio.l, byRatio.gradQ, byRatio.gradL, modes.ratio, byRatio.hessQ, &byRatioHessian)
            .ratio;
    hessian->ratio = byRatioHessian.ratio;
    return result;
}

PlyGradient puckGradient(double s1, double s2, double t12, const PlyMaterial& m, const PlyModes& modes,
                         PlyHessian* hessian)
{
    const double xt = m.xt, xc = m.xc > 0.0 ? m.xc : m.xt, yt = m.yt, yc = m.yc > 0.0 ? m.yc : m.yt, s = m.s;
    const double pt = m.puckTension;
    const double pc = m.puckCompression;
    Vector grad = UNDEFINED;
    // the Hessian of the exposure in (s2, |t12|), its terms s2 s2, s2 t, t t (zero for the fibre)
    double hs2s2 = 0.0;
    double hs2t = 0.0;
    double htt = 0.0;
    double dt = 1.0;
    const bool fiberGoverns = modes.fiber >= modes.matrix;
    if (fiberGoverns)
        grad = s1 >= 0.0 ? Vector{1.0 / xt, 0.0, 0.0} : Vector{-1.0 / xc, 0.0, 0.0};
    else
    {
        const double t = std::fabs(t12);
        dt = signOf(t12); // the derivative of |t12| (that of t12 >= 0 at zero)
        if (modes.mode == "iffA")
        {
            const double a = 1.0 - pt * yt / s;
            const double root = std::sqrt((t / s) * (t / s) + a * a * (s2 / yt) * (s2 / yt));
            if (root > 0.0)
            {
                grad = {0.0, a * a * s2 / (yt * yt) / root + pt / s, t / (s * s) / root * dt};
                // the square root of a quadratic form: (its matrix - g g^T) / root, g the gradient of the root
                const double cube = root * root * root;
                hs2s2 = a * a / (yt * yt) * (t / s) * (t / s) / cube;
                hs2t = -(a * a * s2 / (yt * yt)) * (t / (s * s)) / cube;
                htt = (a * s2 / yt) * (a * s2 / yt) / (s * s) / cube;
            }
        }
        else if (modes.mode == "iffB")
        {
            const double root = std::sqrt(t * t + (pc * s2) * (pc * s2));
            if (root > 0.0)
            {
                grad = {0.0, (pc * pc * s2 / root + pc) / s, t / root / s * dt};
                const double cube = root * root * root;
                hs2s2 = pc * pc * t * t / cube / s;
                hs2t = -pc * pc * s2 * t / cube / s;
                htt = pc * pc * s2 * s2 / cube / s;
            }
        }
        else if (modes.mode == "iffC")
        {
            const double resistance = s / (2.0 * pc) * (std::sqrt(1.0 + 2.0 * pc * yc / s) - 1.0);
            const double pcc = pc * resistance / s;
            const double k = 1.0 / (2.0 * (1.0 + pcc) * s);
            // f = -k^2 t^2 yc / s2 - s2 / yc
            grad = {0.0, k * k * t * t * yc / (s2 * s2) - 1.0 / yc, -2.0 * k * k * t * yc / s2 * dt};
            hs2s2 = -2.0 * k * k * t * t * yc / (s2 * s2 * s2);
            hs2t = 2.0 * k * k * t * yc / (s2 * s2);
            htt = -2.0 * k * k * yc / s2;
        }
    }
    // in (s1, s2, t12): d|t12| / dt12 = dt, its square 1
    const Matrix hessI{0.0, 0.0, 0.0, 0.0, hs2s2, hs2t * dt, 0.0, hs2t * dt, htt};
    return linearPiece(grad, modes.index, modes.ratio, hessI, hessian);
}

PlyGradient derivatives(const std::string& theory, double s1, double s2, double t12, const PlyMaterial& m, double limit,
                        PlyHessian* hessian)
{
    if (std::isnan(s1) || std::isnan(s2) || std::isnan(t12))
        return {};
    PlyGradient result;
    if (theory == "larc03")
        return larc03Derivatives(s1, s2, t12, m, limit, hessian);
    if (theory == "hashin" || theory == "puck")
    {
        const PlyModes modes = plyFailureModes(theory, s1, s2, t12, m, limit);
        if (std::isnan(modes.index))
            return result;
        result = theory == "hashin" ? hashinGradient(s1, s2, t12, m, limit, modes, hessian)
                                    : puckGradient(s1, s2, t12, m, modes, hessian);
        return result;
    }
    const PlyIndex index = plyFailureIndex(theory, s1, s2, t12, m);
    if (std::isnan(index.index))
        return result;
    const double ratio = scaleToLimit(index, limit);
    if (theory == "maxStress" || theory == "maxStrain")
    {
        // the component that governs (the first of the largest), its derivative
        const bool strain = theory == "maxStrain";
        Vector da{1.0, 0.0, 0.0};
        Vector db{0.0, 1.0, 0.0};
        Vector dc{0.0, 0.0, 1.0};
        double a = s1;
        double b = s2;
        double c = t12;
        double xt = m.xt, xc = m.xc, yt = m.yt, yc = m.yc, ss = m.s;
        if (strain)
        {
            const double nu21 = m.nu12 * m.e2 / m.e1;
            a = (s1 - m.nu12 * s2) / m.e1;
            b = (s2 - nu21 * s1) / m.e2;
            c = t12 / m.g12;
            da = {1.0 / m.e1, -m.nu12 / m.e1, 0.0};
            db = {-nu21 / m.e2, 1.0 / m.e2, 0.0};
            dc = {0.0, 0.0, 1.0 / m.g12};
            if (!m.strainAllowables)
            {
                xt /= m.e1;
                xc /= m.e1;
                yt /= m.e2;
                yc /= m.e2;
                ss /= m.g12;
            }
        }
        const double ratios[3] = {a >= 0.0 ? a / xt : -a / xc, b >= 0.0 ? b / yt : -b / yc, std::fabs(c) / ss};
        const Vector gradients[3] = {
            {da[0] * (a >= 0.0 ? 1.0 / xt : -1.0 / xc), da[1] * (a >= 0.0 ? 1.0 / xt : -1.0 / xc),
             da[2] * (a >= 0.0 ? 1.0 / xt : -1.0 / xc)},
            {db[0] * (b >= 0.0 ? 1.0 / yt : -1.0 / yc), db[1] * (b >= 0.0 ? 1.0 / yt : -1.0 / yc),
             db[2] * (b >= 0.0 ? 1.0 / yt : -1.0 / yc)},
            {dc[0] * signOf(c) / ss, dc[1] * signOf(c) / ss, dc[2] * signOf(c) / ss}};
        std::size_t governing = 0;
        for (std::size_t k = 1; k < 3; ++k)
            if (ratios[k] > ratios[governing])
                governing = k;
        return linearPiece(gradients[governing], index.index, ratio, Matrix{}, hessian);
    }
    const double xc = m.xc > 0.0 ? m.xc : m.xt;
    const double yc = m.yc > 0.0 ? m.yc : m.yt;
    Vector gradQ{};
    Vector gradL{};
    Matrix hessQ{};
    if (theory == "hill")
    {
        const double x = s1 >= 0.0 ? m.xt : xc;
        const double y = s2 >= 0.0 ? m.yt : yc;
        gradQ = {(2.0 * s1 - s2) / (x * x), -s1 / (x * x) + 2.0 * s2 / (y * y), 2.0 * t12 / (m.s * m.s)};
        hessQ = {2.0 / (x * x), -1.0 / (x * x), 0.0, -1.0 / (x * x), 2.0 / (y * y), 0.0, 0.0, 0.0, 2.0 / (m.s * m.s)};
    }
    else if (theory == "hoffman")
    {
        gradQ = {(2.0 * s1 - s2) / (m.xt * xc), -s1 / (m.xt * xc) + 2.0 * s2 / (m.yt * yc), 2.0 * t12 / (m.s * m.s)};
        gradL = {(xc - m.xt) / (m.xt * xc), (yc - m.yt) / (m.yt * yc), 0.0};
        const double f11 = 1.0 / (m.xt * xc);
        hessQ = {2.0 * f11, -f11, 0.0, -f11, 2.0 / (m.yt * yc), 0.0, 0.0, 0.0, 2.0 / (m.s * m.s)};
    }
    else if (theory == "tsaiWu")
    {
        const double f11 = 1.0 / (m.xt * xc);
        const double f22 = 1.0 / (m.yt * yc);
        const double f66 = 1.0 / (m.s * m.s);
        const double f12 = m.f12 * std::sqrt(f11 * f22);
        gradQ = {2.0 * f11 * s1 + 2.0 * f12 * s2, 2.0 * f22 * s2 + 2.0 * f12 * s1, 2.0 * f66 * t12};
        gradL = {1.0 / m.xt - 1.0 / xc, 1.0 / m.yt - 1.0 / yc, 0.0};
        hessQ = {2.0 * f11, 2.0 * f12, 0.0, 2.0 * f12, 2.0 * f22, 0.0, 0.0, 0.0, 2.0 * f66};
    }
    else
        return result;
    return quadraticPiece(index.quadratic, index.linear, gradQ, gradL, ratio, hessQ, hessian);
}

} // namespace

PlyGradient plyGradient(const std::string& theory, double s1, double s2, double t12, const PlyMaterial& m, double limit)
{
    return derivatives(theory, s1, s2, t12, m, limit, nullptr);
}

PlyHessian plyHessian(const std::string& theory, double s1, double s2, double t12, const PlyMaterial& m, double limit)
{
    PlyHessian result;
    result.gradient = derivatives(theory, s1, s2, t12, m, limit, &result);
    return result;
}

} // namespace oor::query
